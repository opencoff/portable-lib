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
#include <string.h>
#include <stdlib.h>

#include "utils/utils.h"
#include "utils/xorshift-rand.h"


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
    return z;
}



void
xs64star_init(xs64star* s, uint64_t seed)
{
    s->v = seed ? seed : makeseed();
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
xs128plus_init(xs128plus* s, uint64_t *seed, size_t n)
{
    if (!seed) n = 0;

    switch (n) {
        case 0:
            s->v[0] = makeseed();
            s->v[1] = makeseed();
            break;

        case 1:
            s->v[0] = seed[0];
            s->v[1] = makeseed();
            break;

        default:
        case 2:
            s->v[0] = seed[0];
            s->v[1] = seed[1];
            if (s->v[0] == 0 && s->v[1] == 0) {
                s->v[0] = makeseed();
                s->v[1] = makeseed();
            }
            break;
    }
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
xs1024star_init(xs1024star* s, uint64_t *seed, size_t sn)
{
    size_t i,
           j = 0;

    s->p = makeseed() & 15;

    if (seed) {
        for (i=0; i < sn; ++i) {
            s->v[j++] = seed[i] ? seed[i] : makeseed();

            if (j == 16) return;
        }
    }
    
    while (j < 16) {
        s->v[j++] = makeseed();
    }

}


uint64_t
xs1024star_u64(xs1024star* s)
{
    uint64_t s0 = s->v[s->p++];

    s->p &= 15;

    uint64_t s1 = s->v[s->p];

    s1  ^= s1 << 31;
    s->v[s->p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30);

    return s->v[s->p] * 1181783497276652981uLL;
}

/* EOF */
