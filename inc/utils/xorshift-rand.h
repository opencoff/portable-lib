/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/xorshift-rand.h - Xorshift+ PRNG
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
 * Implementation of Xorshift64*, Xorshift128+, Xorshift1024*.
 *
 * Reference: http://xorshift.di.unimi.it/
 */

#ifndef ___UTILS_XORSHIFT_RAND_H_1501331_1449272000__
#define ___UTILS_XORSHIFT_RAND_H_1501331_1449272000__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdlib.h>

struct xs64star
{
    uint64_t v;
};
typedef struct xs64star xs64star;

struct xs128plus
{
    uint64_t v[2];
};
typedef struct xs128plus xs128plus;

struct xs1024star
{
    uint64_t v[16];
    uint32_t p;
};
typedef struct xs1024star xs1024star;



void     xs64star_init(xs64star*, uint64_t seed);
uint64_t xs64star_u64(xs64star*);

void     xs128plus_init(xs128plus*, uint64_t *seed, size_t n);
uint64_t xs128plus_u64(xs128plus*);

void     xs1024star_init(xs1024star*, uint64_t *seed, size_t sn);
uint64_t xs1024star_u64(xs1024star*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_XORSHIFT_RAND_H_1501331_1449272000__ */

/* EOF */
