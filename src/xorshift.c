/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * xorshift.c - Xorshift+ PRNG
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
 * Reference: http://xorshift.di.unimi.it/
 */

#include <stdint.h>
#include <stdlib.h>

#include "utils/utils.h"
#include "utils/xorshift-rand.h"


/*
 * SplitMix64 PRNG
 */
static inline uint64_t
splitmix64(uint64_t x)
{
    uint64_t z;

    z = x += 0x9E3779B97F4A7C15;
    z = (z ^ (z >> 30) * 0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27) * 0x94D049BB133111EB);

    return z ^ (z >> 31);
}


/*
 * Generate a data dependent random value from the CPU timestamp
 * counter.
 */
static uint64_t
makeseed()
{
    uint64_t c = sys_cpu_timestamp();
    uint64_t m = c & 0xff;
    uint64_t i, j, n;
    uint64_t z = sys_cpu_timestamp();

    for (i = 0; i < m; i++) {
        c = sys_cpu_timestamp();
        n = c % 1024;
        for (j = 0; j < n; ++j) {
            z = (c * (j+1)) ^ (z * i);
        }
    }
    return splitmix64(z);
}


void
xs64star_init(xs64star* s, uint64_t seed)
{
    s->v = seed ? splitmix64(seed) : makeseed();
}


uint64_t
xs64star_u64(xs64star* s)
{
    s->v ^= s->v >> 12;
    s->v ^= s->v << 25;
    s->v ^= s->v << 27;
    return s->v * 2685821657736338717uLL;
}


void
xs128plus_init(xs128plus* s, uint64_t seed)
{
    xs64star z;

    xs64star_init(&z, seed);

    s->v[0] = xs64star_u64(&z);
    s->v[1] = xs64star_u64(&z);
}


uint64_t
xs128plus_u64(xs128plus* s)
{
    uint64_t v1 = s->v[0];
    uint64_t v0 = s->v[1];

    s->v[0] = v0;
    v1     ^= (v1 << 23);
    s->v[1] = v1 ^ v0 ^ (v1 >> 17) ^ (v0 >> 26);
    return s->v[1] + v0;
}



void
xs1024star_init(xs1024star* s, uint64_t seed)
{
    size_t i;
    xs128plus z;

    xs128plus_init(&z, seed);

    for (i=0; i < 16; ++i) {
        s->v[i] = xs128plus_u64(&z);
    }
}


uint64_t
xs1024star_u64(xs1024star* s)
{
    uint64_t s0 = s->v[s->p++];
    uint64_t s1;

    s->p &= 15;

    s1  = s->v[s->p];
    s1 ^= s1 << 31;
    s->v[s->p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30);

    return s->v[s->p] * 1181783497276652981uLL;
}

/* EOF */
