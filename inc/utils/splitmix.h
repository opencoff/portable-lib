/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/splitmix.h - Splitmix64 Simple PRNG
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
 *  Reference: http://xoroshiro.di.unimi.it
 */

#ifndef ___UTILS_SPLITMIX_H__LHmNergujy102vzM___
#define ___UTILS_SPLITMIX_H__LHmNergujy102vzM___ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_SPLITMIX_H__LHmNergujy102vzM___ */

/* EOF */
