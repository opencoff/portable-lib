/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/abyssinian_rand.h
 *
 * Copyright (c) 2014 Sudhi Herle <sw at herle.net>
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

#ifndef ___UTILS_ABYSSINIAN_RAND_H_8638468_1407429002__
#define ___UTILS_ABYSSINIAN_RAND_H_8638468_1407429002__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

struct abyssinian_state
{
    uint64_t x, y;
};
typedef struct abyssinian_state abyssinian_state;

extern void abyssinian_init(abyssinian_state* s, uint32_t seed);
extern uint32_t abyssinian_rand32(abyssinian_state*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_ABYSSINIAN_RAND_H_8638468_1407429002__ */

/* EOF */
