/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * xorfilt_internal.h - Internal header for Xorfilter
 *
 * Copyright (c) 2019 Sudhi Herle <sw at herle.net>
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

#ifndef ___XORFILT_INTERNAL_H__0GeyRYHtVn9W58QS___
#define ___XORFILT_INTERNAL_H__0GeyRYHtVn9W58QS___ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <math.h>

/*
 * Holds the state for the Xorfilter. We keep this opaque to
 * callers.  The actual filter Xor8 or Xor16 is picked based on
 * the variant 'is_16'.
 */
struct Xorfilter {
    union {
        uint8_t *fp8;
        uint16_t *fp16;
        void *ptr;
    };
    uint64_t seed;
    uint32_t size;
    uint32_t n;     // number of elements
    uint8_t  is_16;
    uint8_t  is_mmap;
};
typedef struct Xorfilter Xorfilter;

// Return filter size in bytes
static inline uint64_t
xorfilter_size(Xorfilter *x)
{
    size_t sz = 3 * x->size;

    return x->is_16 ? sz * 2 : sz;
}

// Given number of elements, calculate the actual size of the
// Xorfilter.
static inline size_t
xorfilter_calc_size(size_t n)
{
    double dn   = 32.0 + ceil(1.23 * (double)(n));
    size_t size = (size_t)(dn) / 3;
    return size;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___XORFILT_INTERNAL_H__0GeyRYHtVn9W58QS___ */

/* EOF */
