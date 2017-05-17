/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * seahash.c
 *
 * Copyright (c) 2016 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#include "utils/seahash.h"
#include <sys/types.h>


static inline uint64_t
diffuse(uint64_t x)
{
    x ^= (x >> 32);
    x *= 0x7ed0e9fa0d94a33;
    x ^= (x >> 32);
    x *= 0x7ed0e9fa0d94a33;
    x ^= (x >> 32);
    return x;
}

#define _u64(x)             ((uint64_t)x)
#define _IS_ALIGNED(v, n)   ({ uint64_t x  = _u64(v); \
                                uint64_t z1 = _u64(n) - 1; \
                                x == (x & ~z1);})


// Decode a little-endian 64-bit quantity from a byte buffer
static inline uint64_t
dec_LE_u64(const uint8_t * p) {
    return         p[0]
           | (_u64(p[1]) << 8)
           | (_u64(p[2]) << 16)
           | (_u64(p[3]) << 24)
           | (_u64(p[4]) << 32)
           | (_u64(p[5]) << 40)
           | (_u64(p[6]) << 48)
           | (_u64(p[7]) << 56);
}



// Initialize seahash with a given seed.
// XXX We assume the seed to be at least 4 uint64 long or a nullptr
void
seahash_init(seahash_state *st, const uint64_t *init)
{
    if (!init) {
        static const uint64_t v[4] = { 0x16f11fe89b0d677c, 0xb480a793d8e6c86c,
                                       0x6fe2e5aaf078ebc9, 0x14f994a4c5259381
                                     };
        init = &v[0];
    }

    st->v[0] = init[0];
    st->v[1] = init[1];
    st->v[2] = init[2];
    st->v[3] = init[3];
    st->i    = 0;
    st->n    = 0;
}


// One round of seahash
#define ROUND(st, x) do { \
                        size_t i    = st->i & 3;    \
                        uint64_t *z = &st->v[i];    \
                        *z ^= x;                    \
                        *z  = diffuse(*z);          \
                        st->i += 1;                 \
                     } while (0)



// Word aligned 'p' for multiple words
static const void*
_update_fast(seahash_state *st, const uint64_t *p, size_t words)
{
    uint64_t n = words / 4;

    uint64_t x;

    if (n > 0) {
        // We hope to get some data-stream parallelism by caching
        // the 4 words in 4 vars.
        uint64_t i = st->i,
                 a = st->v[3 & (i+0)],
                 b = st->v[3 & (i+1)],
                 c = st->v[3 & (i+2)],
                 d = st->v[3 & (i+3)];

        st->n += (n * 4);
        st->i += (n * 4);

        // Process 4 words at a time.
        for (; n > 0; n--) {
            a = a ^ *p++;
            b = b ^ *p++;
            c = c ^ *p++;
            d = d ^ *p++;

            a = diffuse(a);
            b = diffuse(b);
            c = diffuse(c);
            d = diffuse(d);
        }

        st->v[3 & (i+0)] = a;
        st->v[3 & (i+1)] = b;
        st->v[3 & (i+2)] = c;
        st->v[3 & (i+3)] = d;
    }

    st->n += (words & 3);
    switch (words & 3) {
        case 3:
            x = *p++;
            ROUND(st, x);
        case 2:
            x = *p++;
            ROUND(st, x);
        case 1:
            x = *p++;
            ROUND(st, x);

        default:
        case 0:
            break;
    }
    return p;
}


// NON-Word aligned 'p' for multiple words
static const void*
_update_slow(seahash_state *st, const uint8_t *p, size_t words)
{
    uint64_t n = words / 4;

    uint64_t x;

    if (n > 0) {
        // We hope to get some data-stream parallelism by caching
        // the 4 words in 4 vars.
        uint64_t i = st->i,
                 a = st->v[3 & (i+0)],
                 b = st->v[3 & (i+1)],
                 c = st->v[3 & (i+2)],
                 d = st->v[3 & (i+3)];

        st->n += (n * 4);
        st->i += (n * 4);

        // Process 4 words at a time and try to get some instr level
        // parallelism
        for (; n > 0; n--) {
            a = a ^ dec_LE_u64(p); p += 8;
            b = b ^ dec_LE_u64(p); p += 8;
            c = c ^ dec_LE_u64(p); p += 8;
            d = d ^ dec_LE_u64(p); p += 8;

            a = diffuse(a);
            b = diffuse(b);
            c = diffuse(c);
            d = diffuse(d);
        }

        st->v[3 & (i+0)] = a;
        st->v[3 & (i+1)] = b;
        st->v[3 & (i+2)] = c;
        st->v[3 & (i+3)] = d;
    }

    st->n += (words & 3);
    switch (words & 3) {
        case 3:
            x = dec_LE_u64(p); p += 8;
            ROUND(st, x);
        case 2:
            x = dec_LE_u64(p); p += 8;
            ROUND(st, x);
        case 1:
            x = dec_LE_u64(p); p += 8;
            ROUND(st, x);

        default:
        case 0:
            break;
    }
    return p;
}


// Hash 'n' bytes of data in 'vbuf' and update state
void
seahash_update(seahash_state *st, const void * vbuf, size_t n)
{
    const uint8_t *buf = vbuf;
    size_t words = n / 8;
    size_t rem   = n % 8;

    if (words > 0) {
        buf = _IS_ALIGNED(buf, 8) ?  _update_fast(st, (uint64_t*)buf, words) : _update_slow(st, buf, words);
    }

    if (rem == 0) return;

    uint64_t x = 0;
    switch (rem) {
        case 7:
            x |= _u64(*buf);
            buf++;

        case 6:
            x <<= 8;
            x |= _u64(*buf);
            buf++;

        case 5:
            x <<= 8;
            x |= _u64(*buf);
            buf++;

        case 4:
            x <<= 8;
            x |= _u64(*buf);
            buf++;

        case 3:
            x <<= 8;
            x |= _u64(*buf);
            buf++;

        case 2:
            x <<= 8;
            x |= _u64(*buf);
            buf++;

        case 1:
            x <<= 8;
            x |= _u64(*buf);
            buf++;

        default:
        case 0:
            break;
    }

    ROUND(st, x);

    st->n += 1;
}


uint64_t
seahash_finish(seahash_state *st)
{
    uint64_t *v = st->v;

    // Use Austin Appleby's diffusion - to ensure intermixing
    // between the 4 lanes.

    v[0] ^= diffuse(v[1]);
    v[2] ^= diffuse(v[3]);
    v[1] ^= diffuse(v[0]);
    v[3] ^= diffuse(v[2]);

    return diffuse(v[1] ^ v[3] ^ st->n);
}


/* EOF */
