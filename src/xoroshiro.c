/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * xoroshiro.c - Xoroshiro PRNG; a successor to Xorshift 128+
 *
 * Copyright (c) 2017 Sudhi Herle <sw at herle.net>
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
 * ======
 *  - Xoroshiro 128+ is a successor to Xorshift 128+
 *  - It is faster and produces better PRNG
 *  - Reference: http://xoroshiro.di.unimi.it
 */
#include <stdint.h>
#include <stdlib.h>

#include "utils/utils.h"
#include "utils/xoroshiro.h"


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
xoro128plus_init(xoro128plus *s, uint64_t seed)
{
    if (!seed) seed = makeseed();

    s->v0 = seed;
    s->v1 = splitmix64(seed);
}



static inline uint64_t
rotl(const uint64_t x, unsigned int k)
{
    return (x << k) | (x >> (64 - k));
}



uint64_t
xoro128plus_u64(xoro128plus *s)
{
    uint64_t v0 = s->v0;
    uint64_t v1 = s->v1;
    uint64_t r  = v0 + v1;

    v1 ^= v0;
    s->v0 = rotl(v0, 55) ^ v1 ^ (v1 << 14);
    s->v1 = rotl(v1, 36);

    return r;
}

/* EOF */
