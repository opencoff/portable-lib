/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * xorfilter.c - Better than Bloom & Cuckoo Filters
 *
 * An independent implementation of:
 *  "Xor Filters: Faster and Smaller Than Bloom and Cuckoo Filters"
 *  https://arxiv.org/abs/1912.08258
 *
 * Copyright (c) 2019 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Notes:
 * =====
 *   S     A Set of elements for which we construct a filter to
 *         answer membership queries
 *   n     Number of elements in S
 *   B    An array of k-bit values; the value of "k" determines how
 *        accurate the filter is (False Positive rate); k=8 or k=16.
 *   c    Size of array; c = ceil(1.23 * n) + 32
 *
 *   h    A hash function that takes the input key and returns a
 *        uniformly distributed 64-bit quantity. In our
 *        implementation, h is one round of Zilong Tan's superfast
 *        hash.
 *
 *   h0, h1, h2  3 Hash functions that map elements in S to:
 *               [0, c/3), [c/3, 2c/3), [2c/3, c) respectively.
 *
 *               In our implementation:
 *               h0 = h
 *               h1 = mix(h)
 *               h2 = mix(mix(h))
 */
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include "fast/vect.h"
#include "utils/xorfilter.h"
#include "xorfilt_internal.h"

extern void arc4random_buf(void *, size_t);

/*
 * Index into the linear Fingerprint array.
 * These are derived from three hashes h0, h1, h2
 */
struct fpidx {
    uint32_t i, j, k;
};
typedef struct fpidx fpidx;

static uint64_t
rand64()
{
    uint64_t x;
    arc4random_buf(&x, sizeof x);
    return x;
}

// Compression function from fasthash
static inline uint64_t
mix(uint64_t h)
{
    h ^= h >> 23;
    h *= 0x2127599bf4325c37ULL;
    h ^= h >> 47;
    return h;
}

/*
 * fasthash64() - but tuned for exactly _one_ round and
 * one 64-bit word.
 *
 * Borrowed from Zilong Tan's superfast hash.
 * Copyright (C) 2012 Zilong Tan (eric.zltan@gmail.com)
 */
static uint64_t
hashkey(uint64_t v, uint64_t salt)
{
    const uint64_t m = 0x880355f21e6d1965ULL;
    uint64_t h       = (8 * m);

    h ^= mix(v);
    h *= m;

    return mix(h) ^ salt;
}

static inline uint32_t
__geth0(uint64_t h, uint32_t size)
{
    return h % size;
}

static inline uint32_t
__geth1(uint64_t h, uint32_t size)
{
    return mix(h) % size;
}

static inline uint32_t
__geth2(uint64_t h, uint32_t size)
{
    return mix(mix(h)) % size;
}

/*
 * We compute all 3 indices to access the fingerprints in the large
 * linear fp8/fp16 array.
 */
static inline fpidx
hash3(uint64_t h, uint32_t size)
{
    fpidx z = {
        .i = __geth0(h, size),
        .j = __geth1(h, size) + size,
        .k = __geth2(h, size) + (2 * size),
    };

    return z;
};

// Given a hashed quantity, return its 8 bit fingerprint
static inline uint8_t
__xfp8(uint64_t h)
{
    return 0xff & (h ^ (h >> 32));
}

// Given a hashed quantity, return its 16 bit fingerprint
static inline uint16_t
__xfp16(uint64_t h)
{
    return 0xffff & (h ^ (h >> 32));
}

// hash-mask and # of times we saw it
struct xorset
{
    uint64_t mask;
    uint64_t n;
};
typedef struct xorset xorset;

// reverse map of hashmask to the index we saw it
struct keyidx
{
    uint64_t hash;
    uint64_t idx;
};
typedef struct keyidx keyidx;

VECT_TYPEDEF(keyvect, keyidx);


/*
 * Update xorset 's' with new entry from index 'i'
 */
static void
__update_q(keyvect *q, xorset *xs, uint32_t i, uint64_t h)
{
    xorset *x = &xs[i];

    x->mask ^= h;
    if (--x->n == 1) {
        keyidx ki = {
            .hash = x->mask,
            .idx  = i,
        };
        VECT_PUSH_BACK(q, ki);
    }
}

static keyvect
xorfilter_init(Xorfilter *x, uint64_t *keys, size_t n)
{
    size_t size = xorfilter_calc_size(n);

    // always a multiple of 3
    size_t   cap   = size * 3;
    uint32_t tries = 0;
    uint64_t seed  = 0;

    xorset *H = NEWA(xorset, cap);
    keyvect q;
    keyvect stack;

    VECT_INIT(&q, cap);
    VECT_INIT(&stack, n);

    while (1) {
        memset(H, 0, cap * sizeof(H[0]));
        seed = rand64();

        for (size_t i = 0; i < n; i++) {
            uint64_t h = hashkey(keys[i], seed);
            fpidx    z = hash3(h, size);

            H[z.i].mask ^= h;
            H[z.i].n++;

            H[z.j].mask ^= h;
            H[z.j].n++;

            H[z.k].mask ^= h;
            H[z.k].n++;
        }

        VECT_RESET(&q);
        for (size_t i = 0; i < cap; i++) {
            xorset *y = &H[i];
            if (y->n == 1) {
                keyidx ki = {
                    .hash = y->mask,
                    .idx  = i,
                };

                VECT_PUSH_BACK(&q, ki);
            }
        }

        VECT_RESET(&stack);
        while (VECT_LEN(&q) > 0) {
            keyidx ki = VECT_POP_BACK(&q);

            if (H[ki.idx].n != 1) continue;

            // sole element in H[i]
            VECT_PUSH_BACK(&stack, ki);
            fpidx z = hash3(ki.hash, size);

            __update_q(&q, H, z.i, ki.hash);
            __update_q(&q, H, z.j, ki.hash);
            __update_q(&q, H, z.k, ki.hash);
        }

        if (VECT_LEN(&stack) == n) break;

        if (++tries > 1000000) {
            VECT_RESET(&stack);
            goto fini;
        }

    }

    x->seed = seed;
    x->size = size;
    x->n    = n;

fini:
    VECT_FINI(&q);
    DEL(H);
    return stack;
}



Xorfilter *
Xorfilter_new8(uint64_t *keys, size_t n)
{
    Xorfilter *x  = NEWZ(Xorfilter);
    keyvect stack = xorfilter_init(x, keys, n);

    if (VECT_LEN(&stack) == 0) {
        DEL(x);
        return 0;
    }

    x->fp8 = NEWZA(uint8_t, VECT_LEN(&stack) * 3);
    while (VECT_LEN(&stack) > 0) {
        keyidx  ki = VECT_POP_BACK(&stack);
        uint8_t fp = __xfp8(ki.hash);
        fpidx    z = hash3(ki.hash, x->size);

        x->fp8[ki.idx] = fp ^ x->fp8[z.i] ^ x->fp8[z.j] ^ x->fp8[z.k];
    }

    VECT_FINI(&stack);
    return x;
}

Xorfilter *
Xorfilter_new16(uint64_t *keys, size_t n)
{
    Xorfilter *x  = NEWZ(Xorfilter);
    keyvect stack = xorfilter_init(x, keys, n);

    if (VECT_LEN(&stack) == 0) {
        DEL(x);
        return 0;
    }

    x->fp16  = NEWZA(uint16_t, VECT_LEN(&stack) * 3);
    x->is_16 = 1;
    while (VECT_LEN(&stack) > 0) {
        keyidx   ki = VECT_POP_BACK(&stack);
        uint16_t fp = __xfp16(ki.hash);
        fpidx    z = hash3(ki.hash, x->size);

        x->fp16[ki.idx] = fp ^ x->fp16[z.i] ^ x->fp16[z.j] ^ x->fp16[z.k];
    }

    VECT_FINI(&stack);
    return x;
}


int
Xorfilter_contains(Xorfilter *x, uint64_t key)
{
    uint64_t h = hashkey(key, x->seed);
    fpidx    z = hash3(h, x->size);

    if (x->is_16) {
        uint16_t fp = __xfp16(h);
        uint16_t *b = x->fp16;
        return fp == (b[z.i] ^ b[z.j] ^ b[z.k]);
    }
    uint8_t fp = __xfp8(h);
    uint8_t *b = x->fp8;
    return fp == (b[z.i] ^ b[z.j] ^ b[z.k]);
}


// bits per entry
double
Xorfilter_bpe(Xorfilter *x)
{
    double sz = (double)(8 * xorfilter_size(x));
    return sz / ((double)x->n);
}


void
Xorfilter_delete(Xorfilter *x)
{
    if (x->ptr && !x->is_mmap) DEL(x->ptr);
    DEL(x);
}
/* EOF */
