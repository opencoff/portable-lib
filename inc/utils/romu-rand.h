/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * inc/utils/romu-rand.h - Romu pseudo random generators
 *
 * Independent implementation of https://romu-random.org/
 *
 * Copyright (c) 2020 Sudhi Herle <sw at herle.net>
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

#ifndef ___INC_UTILS_ROMU_RAND_H__G9lvMx72FT3HsP1w___
#define ___INC_UTILS_ROMU_RAND_H__G9lvMx72FT3HsP1w___ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

/*
 * Single struct to capture state for 
 * romu_quad, romu_trio and romu_duo
 */
struct romu
{
    uint64_t w, x, y, z;
};
typedef struct romu romu;


/**
 * Initialize a romu-Quad random number instance.
 */
extern void romu_quad_init(romu *r, uint64_t a, uint64_t b);
extern uint64_t romu_quad(romu *r);

extern void romu_trio_init(romu *r, uint64_t a, uint64_t b);
extern uint64_t romu_trio(romu *r);

extern void romu_duo_init(romu *r, uint64_t a);
extern uint64_t romu_duo(romu *r);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___INC_UTILS_ROMU_RAND_H__G9lvMx72FT3HsP1w___ */

/* EOF */
