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
#include "utils/splitmix.h"



void
xs64star_init(xs64star* s, uint64_t seed)
{
    s->v = seed ? splitmix64(&seed) : makeseed();
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
    s->v[1] = v1 ^ v0 ^ (v1 >> 18) ^ (v0 >> 5);
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
    s->p = 15 & xs128plus_u64(&z);
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
