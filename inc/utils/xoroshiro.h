/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/xoroshiro.h - Xoroshiro 128+ PRNG
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
 *  Reference: http://xoroshiro.di.unimi.it
 */

#ifndef ___UTILS_XOROSHIRO_H__LHmNergujy102vzM___
#define ___UTILS_XOROSHIRO_H__LHmNergujy102vzM___ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdlib.h>
#include "utils/splitmix.h"

struct xoro128plus
{
    uint64_t v0, v1;
};
typedef struct xoro128plus xoro128plus;


/*
 * Xoroshiro 128+ PRNG
 */
extern void xoro128plus_init(xoro128plus *z, uint64_t seed);
extern uint64_t xoro128plus_u64(xoro128plus *z);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_XOROSHIRO_H__LHmNergujy102vzM___ */

/* EOF */
