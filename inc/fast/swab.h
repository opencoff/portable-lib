/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * swab.h - Macros to swap bytes from one endian to another.
 *
 * Copyright (c) 2004-2008  Sudhi Herle <sw at herle.net>
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

/*
 * Inspired by similar facilities in the linux kernel.
 */

#ifndef __FAST_SWAB_H__
#define __FAST_SWAB_H__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>


/*
 * Macro definitions of byte swap synthetic instructions. Don't use
 * these unless you know 'y' is a constant.
 * XXX Many archs define these as native instructions; need a way to
 *     make the arch specific hooks available.
 */
#define __constant_swab16(y) \
( \
    ((uint16_t)( \
        (((uint16_t)(y) & (uint16_t)0x00ffU) << 8) | \
        (((uint16_t)(y) & (uint16_t)0xff00U) >> 8) )) \
)

#define __constant_swab32(y) \
( \
    ((uint32_t)( \
        (((uint32_t)(y) & (uint32_t)0x000000ffUL) << 24) | \
        (((uint32_t)(y) & (uint32_t)0x0000ff00UL) <<  8) | \
        (((uint32_t)(y) & (uint32_t)0x00ff0000UL) >>  8) | \
        (((uint32_t)(y) & (uint32_t)0xff000000UL) >> 24) )) \
)

#define __constant_swab64(y) \
( \
    ((uint64_t)( \
        (uint64_t)(((uint64_t)(y) & (uint64_t)0x00000000000000ffULL) << 56) | \
        (uint64_t)(((uint64_t)(y) & (uint64_t)0x000000000000ff00ULL) << 40) | \
        (uint64_t)(((uint64_t)(y) & (uint64_t)0x0000000000ff0000ULL) << 24) | \
        (uint64_t)(((uint64_t)(y) & (uint64_t)0x00000000ff000000ULL) <<  8) | \
        (uint64_t)(((uint64_t)(y) & (uint64_t)0x000000ff00000000ULL) >>  8) | \
        (uint64_t)(((uint64_t)(y) & (uint64_t)0x0000ff0000000000ULL) >> 24) | \
        (uint64_t)(((uint64_t)(y) & (uint64_t)0x00ff000000000000ULL) >> 40) | \
        (uint64_t)(((uint64_t)(y) & (uint64_t)0xff00000000000000ULL) >> 56) )) \
)



/*
 * Functions that do byte swapping. It's OK to use the macros
 * inside the function since 'x' is essentially "constant".
 */

static inline uint16_t __fswab16(uint16_t x)
{
    return __constant_swab16(x);
}

static inline uint32_t __fswab32(uint32_t x)
{
    return __constant_swab32(x);
}

static inline uint64_t __fswab64(uint64_t x)
{
    return __constant_swab64(x);
}



#if defined(__GNUC__)
#define __swab16(x) (__builtin_constant_p((uint16_t)(x)) ? __constant_swab16(x) : __fswab16(x))
#define __swab32(x) (__builtin_constant_p((uint32_t)(x)) ? __constant_swab32(x) : __fswab32(x))
#define __swab64(x) (__builtin_constant_p((uint64_t)(x)) ? __constant_swab64(x) : __fswab64(x))
#else
#define __swab16(x) __fswab16(x)
#define __swab32(x) __fswab32(x)
#define __swab64(x) __fswab64(x)
#endif /* __GNUC__  */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __FAST_SWAB_H__ */

/* EOF */
