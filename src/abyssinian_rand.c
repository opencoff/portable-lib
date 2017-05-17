/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * abyssinian_rand.c
 *
 * This is an implementation of a random number generator
 * that is designed to generate up to 2^^32 numbers per seed.
 * 
 * Its period is about 2^^126 and passes all BigCrush tests.
 * It is the fastest generator I could find that passes all tests.
 * 
 * Furthermore, the input seeds are hashed to avoid linear
 * relationships between the input seeds and the low bits of
 * the first few outputs.
 *
 * Copyright (c) 2012 Christopher A. Taylor
 * Borrowed from hard.io/calico github repo.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#include <stdint.h>

typedef struct abyssinian_state st;

void
abyssinian_init(st* s, uint32_t seed)
{
    uint32_t x = seed,
             y = seed;

    // Based on the mixing functions of MurmurHash3
    static const uint64_t C1 = 0xff51afd7ed558ccdULL;
    static const uint64_t C2 = 0xc4ceb9fe1a85ec53ULL;
    uint64_t _x, _y;

    x += y;
    y += x;

    uint64_t seed_x = 0x9368e53c2f6af274ULL ^ x;
    uint64_t seed_y = 0x586dcd208f7cd3fdULL ^ y;

    seed_x *= C1;
    seed_x ^= seed_x >> 33;
    seed_x *= C2;
    seed_x ^= seed_x >> 33;

    seed_y *= C1;
    seed_y ^= seed_y >> 33;
    seed_y *= C2;
    seed_y ^= seed_y >> 33;

    _x = seed_x;
    _y = seed_y;

    // Inlined Next(): Discard first output

    _x = (uint64_t)0xfffd21a7 * (uint32_t)_x + (uint32_t)(_x >> 32);
    _y = (uint64_t)0xfffd1361 * (uint32_t)_y + (uint32_t)(_y >> 32);

    s->x = _x;
    s->y = _y;
}



uint32_t
abyssinian_rand32(st* s)
{
#define ROL32(n, r) ( ((uint32_t)(n) << (r)) | ((uint32_t)(n) >> (32 - (r))) )
    
    s->x = (uint64_t)0xfffd21a7 * (uint32_t)s->x + (uint32_t)(s->x >> 32);
    s->y = (uint64_t)0xfffd1361 * (uint32_t)s->y + (uint32_t)(s->y >> 32);
    return ROL32((uint32_t)s->x, 7) + (uint32_t)s->y;
}

/* EOF */
