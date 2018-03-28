/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * bloom.c - Partitioned and Counting bloom filters.
 *
 * Copyright (c) 2015 Sudhi Herle <sw at herle.net>
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
 *
 * Notation:
 * =========
 *   e  = false positive error rate that is desired (input parameter)
 *   n  = number of elements predicted by this bloom filter (input
 *        parameter)
 *
 *   m  = total number of 'slots' in this bloom filter. When
 *        partitioned, each partition will have m/k bits.
 *        IMPORTANT: In 'struct bloom', we use 'm' for the size of
 *        the partition - since that is the most frequently used
 *        value.
 *
 *   p  = fill ratio; we have hard coded this to be 1/2 (50%) in all
 *        the math that follows.
 *
 *   k  = number of hash functions and also the number of partitions
 *        of the slots. Each partition holds ceil(m/k) slots.
 *
 *  The original equation for 'm' is:
 *
 *      m = n * (-log(e)) / (log(p) * log(1-p)))
 *
 *  Putting p = 1/2 above, we get:
 *
 *      m = n * (-log(e) / (log(2) ** 2))
 *
 *  Thus, the two key values of 'k' and 'm' are calculated as:
 *
 *      k = -log(e) / log(2)
 *      m = n * (-log(e) / (log(2) ** 2))
 *
 * Scalable Bloom Filters
 * ======================
 * The implementation of the scalable bloom filter closely follows
 * the ideas laid out in [3]. Continuing with their notation:
 *
 *   r  = error tighetening ratio for scalable bloom filter growth
 *   s  = filter size scale factor
 *
 * For each successive filter we create, the number of items in the
 * partition grows by a factor of 's'. Thus, for the i_th new filter
 * we add:
 *
 *    m[i] = m[0] * (s ** i)
 *
 *  With s = 2, this simplifies to:
 *
 *    m[i] = m[0] << i
 *
 * Per [3]:
 *   o 0.5 <= r <= 0.9.  We choose 0.9 as the value for our
 *     purposes.
 *   o s = 2. This gives us exponential growth on the number of
 *     elements we can hold. What this means is that we essentially
 *     double the size of each succeeding filter while tightening
 *     the FP rate by a factor of 'r'.
 *       
 *
 * Notes:
 * ======
 *   o This implementation borrows ideas from [4] to create a
 *     partitioned bloom filter.
 *
 *   o This implementation uses optimizations in Mitzenmacher &
 *     Kirsch [1].
 *     We use a seeded 64-bit hash function: fasthash64 due to Zilong Tan.
 *     The seed is a random 64 bit quantity initialized when the filter
 *     is created. To be clear, this hash function is used in the
 *     sense of Mitzenmacher & Kirsch [1].
 *
 *   o Instead of assuming any specific hash function on the input
 *     bytes, this library requires the caller to provide a hashed
 *     uint64_t. The caller is responsible for ensuring they pick a
 *     good hash function of their choice (murmur, siphash24, FNV2a
 *     are all good choices). The library makes no other assumption
 *     on the provided input.
 *
 *   o The math above is from [2].
 *
 *   o The library provides facilities to safely marshal and
 *     unmarshall the bloom filter to a file. The marshalled data
 *     uses strong checksums (SHA256) on the filter bits as well as
 *     the header. The checksums are verified during unmarshaling.
 *
 *   o The marshal/unmarshal code is in bloom_marshal.c
 *
 * References:
 * ===========
 * [1] Less Hashing, Same Performance: Building a Better Bloom Filter
 *     http://www.eecs.harvard.edu/~kirsch/pubs/bbbf/rsa.pdf
 *
 * [2] https://en.wikipedia.org/wiki/Bloom_filter
 *
 * [3] Scalable Bloom Filters
 *     http://gsd.di.uminho.pt/members/cbm/ps/dbloom.pdf
 *
 * [4] Approximate caches for packet classification
 *     http://www.ieee-infocom.org/2004/Papers/45_3.PDF
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <inttypes.h>

#include "utils/utils.h"
#include "utils/bloom.h"

#include "bloom_internal.h"


// Common on OS X and *BSD. Not so on Linux.
extern void arc4random_buf(void* out, size_t n);



// Compression function from fasthash
#define mix(h) ({                   \
            (h) ^= (h) >> 23;       \
            (h) *= 0x2127599bf4325c37ULL;   \
            (h) ^= (h) >> 47; h; })


/*
 * fasthash64() - but tuned for exactly _one_ round and
 * one 64-bit word.
 *
 * Borrowed from Zilong Tan's superfast hash.
 * Copyright (C) 2012 Zilong Tan (eric.zltan@gmail.com)
 */
static uint64_t
hash_val(uint64_t v, uint64_t salt)
{
    const uint64_t m = 0x880355f21e6d1965ULL;
    uint64_t h       = (8 * m);

    h ^= mix(v);
    h *= m;

    return mix(h) ^ salt;
}


/*
 * Number of bits per unit of storage
 */
#define UNITSIZE    (sizeof(uint8_t))
#define UNITBITS    (8 * UNITSIZE)


static inline int
testbit(uint8_t* bm, uint64_t i)
{
    uint64_t j = i / UNITBITS;
    uint64_t k = i % UNITBITS;
    return bm[j] & (1 << k);
}

// Set bit and return previous val
static inline void
setbit(uint8_t* bm, uint64_t i)
{
    uint64_t j = i / UNITBITS;
    uint64_t k = i % UNITBITS;
    uint64_t m = 1 << k;

    bm[j] |= m;
}



/*
 * Add a new entry into the filter.
 */
static void
standard_bloom_probe(bloom* b, uint64_t val)
{
    uint64_t z  = hash_val(val, b->salt);
    uint64_t h1 = z & 0xffffffff;
    uint64_t h2 = z >> 32;
    uint64_t i;

    for (i = 0; i < b->k; ++i) {
        uint64_t k = (h1 + i * h2) % b->m; // bit to set
        uint64_t j = k + (i * b->m);       // in parition 'i'
        setbit(b->bitmap, j);
    }

    b->size++;
}




/*
 * Return False if element is definitely NOT in the filter.
 *
 * A True return value is not indicative of existence.
 */
static int
standard_bloom_find(bloom* b, uint64_t val)
{
    uint64_t z  = hash_val(val, b->salt);
    uint64_t h1 = z & 0xffffffff;
    uint64_t h2 = z >> 32;
    uint64_t i;

    for (i = 0; i < b->k; ++i) {
        uint64_t k = (h1 + i * h2) % b->m; // bit to set
        uint64_t j = k + (i * b->m);       // in parition 'i'
        if (!testbit(b->bitmap, j)) return 0;
    }

    return 1;
}




/*
 * Probe():
 *   o if value not in table, add it
 *   o else, increment count
 */
static void
counting_bloom_probe(bloom* b, uint64_t val)
{
    uint64_t z  = hash_val(val, b->salt);
    uint64_t h1 = z & 0xffffffff;
    uint64_t h2 = z >> 32;
    uint64_t i;

    for (i = 0; i < b->k; ++i) {
        uint64_t k = (h1 + i * h2) % b->m;
        uint64_t j = k + (i * b->m);

        b->bitmap[j]++;
    }

    b->size++;
}


/*
 * Return False if element was not found, True otherwise.
 */
static int
counting_bloom_remove(bloom* b, uint64_t val)
{
    uint64_t z  = hash_val(val, b->salt);
    uint64_t h1 = z & 0xffffffff;
    uint64_t h2 = z >> 32;
    uint64_t i;
    uint64_t r = 0;

    for (i = 0; i < b->k; ++i) {
        uint64_t k = (h1 + i * h2) % b->m;
        uint64_t j = k + (i * b->m);
        if (b->bitmap[j]) {
            b->bitmap[j] -= 1;
            r = 1;
        }
    }

    b->size--;
    return r;
}



/*
 * Return True if val is in the bloom filter; false otherwise.
 */
static int
counting_bloom_find(bloom* b, uint64_t val)
{
    uint64_t z  = hash_val(val, b->salt);
    uint64_t h1 = z & 0xffffffff;
    uint64_t h2 = z >> 32;
    uint64_t i;

    for (i = 0; i < b->k; ++i) {
        uint64_t k = (h1 + i * h2) % b->m;
        uint64_t j = k + (i * b->m);
        if (!b->bitmap[j])  return 0;
    }

    return 1;
}


/*
 * The number of slots needed to hold the bitmap is proportional to
 * the desired false positive error rate.
 */
static bloom*
counting_bloom_init(bloom* b, size_t n, double e)
{
    uint64_t k = make_k(e);
    uint64_t m = make_m(n, e);
    uint64_t s = m / k + ((m % k) > 0);  // bytes per slice

    uint64_t bytes = (s * k);
    b->bitmap      = NEWZA(uint8_t, bytes);
    if (!b->bitmap)  return 0;

    b->k      = k;
    b->m      = s;
    b->e      = e;
    b->bmsize = bytes;
    b->flags  = 0;

    // use a random salt for the seeded hash function.
    // If your OS doesn't have this - try:
    //   http://github.com/opencoff/mt-arc4random
    arc4random_buf(&b->salt, sizeof b->salt);

    return b;
}




static bloom*
standard_bloom_init(bloom* b, size_t n, double e)
{
    uint64_t k     = make_k(e);
    uint64_t m     = make_m(n, e);
    uint64_t msub  = m / k + ((m % k) > 0);
    uint64_t nbits = _ALIGN_UP(msub * k, 64);
    uint64_t units = nbits / UNITBITS;

    uint64_t bytes = (units * UNITSIZE);
    b->bitmap      = NEWZA(uint8_t, bytes);
    if (!b->bitmap)  return 0;

    b->k = k;
    b->m = msub;
    b->e = e;

    b->bmsize  = bytes;
    b->flags   = 0;

    // use a random salt for the seeded hash function.
    arc4random_buf(&b->salt, sizeof b->salt);

    return b;
}


/*
 * Common finalizer for standard and counting bloom filters.
 */
static void
bloom_fini(bloom* b)
{
    if (!(b->flags & BLOOM_BITMAP_MMAP))
        DEL(b->bitmap);
}


/*
 * Empty remove function. Always pretends that it worked!
 */
static int
null_remov(bloom *b, uint64_t v)
{
    USEARG(b);
    USEARG(v);
    return 1;
}

/*
 * Textual descriptions of each
 */
static char*
standard_bloom_desc(bloom* b, char *buf, size_t bsiz)
{
    uint64_t bits  = b->m * b->k;
    uint64_t bytes = bits / 8;

    char sz[128];
    humanize_size(sz, sizeof sz, bytes);

    snprintf(buf, bsiz, "standard-bloom: FP-prob: %5.4f: %" PRIu64 " partitions x %" PRIu64 " slots/partition = %s; "
                        "%" PRIu64 " elem (est fill ratio %5.4f)", b->e,
                        b->k, b->m, sz, b->size, bloom_fill_ratio_est(b));

    return buf;
}


static char*
counting_bloom_desc(bloom* b, char *buf, size_t bsiz)
{
    uint64_t bytes = b->m * b->k;

    char sz[128];
    humanize_size(sz, sizeof sz, bytes);

    snprintf(buf, bsiz, "counting-bloom: FP-prob: %5.4f: %" PRIu64 " partitions x %" PRIu64 " slots/partition = %s; "
                        "%" PRIu64 " elem (est fill ratio %5.4f)", b->e,
                        b->k, b->m, sz, b->size, bloom_fill_ratio_est(b));

    return buf;
}


/*
 * Scalable Bloom filters
 */



static inline bloom*
maybe_grow_filter(scalable_bloom *sb)
{
    if (sb->len == sb->cap) {
        uint32_t x = sb->cap << 1;
        assert(x > sb->cap);

        bloom* bv = RENEWA(bloom, sb->bfa, x);
        assert(bv);

        sb->bfa = bv;
        sb->cap = x;
    }

    // Return pointer to un-initialized filter
    return &sb->bfa[sb->len++];
}

// insert a new element 'v' into a scalable filter
static void
scalable_probe(scalable_bloom* sb, uint64_t v)
{
    uint32_t i = sb->len - 1;

    assert(i < sb->len);

    bloom *f = &sb->bfa[i];

    if (bloom_fill_ratio_est(f) > 0.5) {
        double e   = f->e * sb->r;
        uint64_t m = f->m * f->k * sb->scale;
        size_t n   = make_n(m, e);

        // Get the next filter to use
        f = maybe_grow_filter(sb);

        standard_bloom_init(f, n, e);
    }

    standard_bloom_probe(f, v);
}


static void
scalable_fini(scalable_bloom *sb)
{
    uint32_t i;

    for (i = 0; i < sb->len; ++i) {
        bloom* f = &sb->bfa[i];

        bloom_fini(f);
    }

    DEL(sb->bfa);
    memset(sb, 0x33, sizeof *sb);
}


// lookup 'v' in a scalable bloom filter 'sb'
// We start from the "largest" filter in the set and work
// down. Note that when we insert, we *always* insert at the largest filter
// in the list.
static int
scalable_find(scalable_bloom* sb, uint64_t v)
{
    assert(sb->len > 0);

    int64_t i = sb->len - 1;
    for (; i >= 0; i--) {
        bloom* f = &sb->bfa[i];
        if (standard_bloom_find(f, v)) return 1;
    }
    return 0;
}



/*
 * Scalable bloom filters description can get long!
 */
static char*
scalable_bloom_desc(scalable_bloom* sb, char *buf, size_t bsiz)
{
    char* ret = buf;
    bloom* f = &sb->bfa[0];
    char* p;

    asprintf(&p, "scalable-bloom: %d filters (scale %d, err tightening factor %5.4f)\n",
                 sb->len, sb->scale, sb->r);

    ssize_t n = strcopy(buf, bsiz, p); free(p);
    if (n < 0) return ret;

    buf  += n;
    bsiz -= n;

    char sz[128];
    uint32_t i;
    uint64_t size;
    for (i = 0; i < sb->len; ++i) {
        f = &sb->bfa[i];
        size = f->m * f->k;

        // standard bloom filters use one BIT per slot. So, the
        // total size is reduced by a factor of 8.
        size /= 8;

        humanize_size(sz, sizeof sz, size);
        asprintf(&p, "    [%02u] FP-prob: %5.4f: %" PRIu64 " partitions x %" PRIu64 " slots/partition [%s]: %" PRIu64 " elem (fill %6.3f)\n",
                i, f->e, f->k, f->m, sz, f->size, bloom_fill_ratio_est(f));

        n = strcopy(buf, bsiz, p); free(p);
        if (n < 0) return ret;

        buf  += n;
        bsiz -= n;
    }
    return ret;
}


// Initialize function pointers for scalable bloom filter.
// Return true on success, false otherwise
static int
setup_scalable_bloom(Bloom *b, uint32_t nfilt)
{
    scalable_bloom* sb = NEWZ(scalable_bloom);
    if (!sb) return 0;

    sb->bfa = NEWZA(bloom, nfilt);
    if (!sb->bfa) {
        DEL(sb);
        return 0;
    }

    sb->b     = b;
    b->find   = (int  (*)(void*, uint64_t))scalable_find;
    b->probe  = (void (*)(void*, uint64_t))scalable_probe;
    b->remov  = (int  (*)(void*, uint64_t))null_remov;
    b->fini   = (void (*)(void*))scalable_fini;
    b->desc   = (char* (*)(void*, char*, size_t))scalable_bloom_desc;
    b->name   = "scalable-standard-bloom";
    b->typ    = BLOOM_TYPE_SCALE;
    b->filter = sb;

    sb->len   = 1; // we start with the first level of the filter.
    sb->cap   = nfilt;

    return 1;
}


// Initialize function pointers for standard bloom filter.
// Return true on success, false otherwise
static inline int
setup_standard_bloom(Bloom *b)
{
    b->find   = (int  (*)(void*, uint64_t))standard_bloom_find;
    b->probe  = (void (*)(void*, uint64_t))standard_bloom_probe;
    b->remov  = (int  (*)(void*, uint64_t))null_remov;
    b->fini   = (void (*)(void*          ))bloom_fini;
    b->desc   = (char* (*)(void*, char*, size_t))standard_bloom_desc;
    b->name   = "standard-bloom";
    b->typ    = BLOOM_TYPE_QUICK;
    b->filter = NEWZ(bloom);

    return !!b->filter;
}


// Initialize function pointers for counting bloom filter.
// Return true on success, false otherwise
static inline int
setup_counting_bloom(Bloom *b)
{
    b->find   = (int  (*)(void*, uint64_t))counting_bloom_find;
    b->probe  = (void (*)(void*, uint64_t))counting_bloom_probe;
    b->remov  = (int  (*)(void*, uint64_t))counting_bloom_remove;
    b->fini   = (void (*)(void*          ))bloom_fini;
    b->desc   = (char* (*)(void*, char*, size_t))counting_bloom_desc;
    b->name   = "counting-bloom";
    b->typ    = BLOOM_TYPE_COUNTING;
    b->filter = NEWZ(bloom);

    return !!b->filter;
}



/*
 * Internal routine to setup a naked bloom filter and its function
 * pointers. 'n' is the number of filters for scalable filters. It
 * is ignored for non-scalable filters.
 *
 * The actual filter memory is done by the unmarshal code.
 *
 * Returns:
 *      ptr to allocated & setup bloom filter
 *      0 on failure
 */
Bloom*
__alloc_bloom(int typ, uint32_t n)
{
    Bloom *b = NEWZ(Bloom);

    if (!b) return 0;

    switch (typ) {
        case BLOOM_TYPE_SCALE:
            if (setup_scalable_bloom(b, n)) return b;
            break;

        case BLOOM_TYPE_COUNTING:
            if (setup_counting_bloom(b))    return b;
            break;

        case BLOOM_TYPE_QUICK:
            if (setup_standard_bloom(b))    return b;
            break;
    }

    DEL(b);
    return 0;
}

// Create a scalable, non-counting bloom filter.
static Bloom*
scalable_new(Bloom *b, uint64_t n, double e)
{
    if (!setup_scalable_bloom(b, 8)) return 0;

    scalable_bloom *sb = b->filter;

    b->n      = n;
    b->e      = e;

    sb->scale = 2;
    sb->r     = 0.9;

    // Initialize the first filter in the list.
    if (standard_bloom_init(&sb->bfa[0], n, e)) return b;

    DEL(sb->bfa);
    DEL(sb);
    return 0;
}



#define _EPSILON    2.2204460492503131e-16

static int
bloom_eq(bloom *a, bloom *b)
{
    if (a->m != b->m)           return 0;
    if (a->k != b->k)           return 0;
    if (a->salt != b->salt)     return 0;
    if (a->size != b->size)     return 0;
    if (a->bmsize != b->bmsize) return 0;
    if (0 != memcmp(a->bitmap, b->bitmap, a->bmsize)) return 0;

    return 1;
}


static int
scaled_bloom_eq(scalable_bloom *a, scalable_bloom *b)
{
    if (a->len != b->len)             return 0;
    if (a->scale != b->scale)         return 0;
    if (fabs(a->r - b->r) > _EPSILON) return 0;

    uint32_t i;
    for (i = 0; i < a->len; i++) {
        bloom *x = &a->bfa[i],
              *y = &b->bfa[i];

        if (!bloom_eq(x, y)) return 0;
    }

    return 1;
}


/*
 * Public interfaces to the filters
 */

Bloom*
Standard_bloom_init(Bloom *b, uint64_t n, double e, int scalable)
{
    if (scalable) return scalable_new(b, n, e);

    b->n      = n;
    b->e      = e;

    if (!setup_standard_bloom(b))             return 0;
    if (standard_bloom_init(b->filter, n, e)) return b;

    DEL(b->filter);
    return 0;
}


Bloom*
Counting_bloom_init(Bloom *b, uint64_t n, double e)
{
    b->n      = n;
    b->e      = e;

    if (!setup_counting_bloom(b))             return 0;
    if (counting_bloom_init(b->filter, n, e)) return b;

    DEL(b->filter);
    return 0;
}

// free memory associated with filter 'b'
void
Bloom_fini(Bloom *b)
{
    b->fini(b->filter);

    DEL(b->filter);
    memset(b, 0x55, sizeof *b);
}

// create a new instance of a counting bloom filter
Bloom*
Counting_bloom_new(uint64_t n, double e)
{
    Bloom* b  = NEWZ(Bloom);
    if (!b) return 0;

    if (Counting_bloom_init(b, n, e)) return b;

    DEL(b);
    return 0;
}


// create a new instance of a (non-counting) bloom filter
Bloom*
Standard_bloom_new(uint64_t n, double e, int scalable)
{
    Bloom* b  = NEWZ(Bloom);
    if (!b) return 0;

    if (Standard_bloom_init(b, n, e, scalable)) return b;

    DEL(b);
    return 0;
}

// free all memory associated with filter 'b' and 'b' itself.
void
Bloom_delete(Bloom *b)
{
    Bloom_fini(b);
    DEL(b);
}



// Return true if two bloom filters are identical; false otherwise
int
Bloom_eq(Bloom *a, Bloom *b)
{
    if (a->typ != b->typ) return 0;

    if (a->typ == BLOOM_TYPE_SCALE) return scaled_bloom_eq(a->filter, b->filter);

    return bloom_eq(a->filter, b->filter);
}


/* EOF */
