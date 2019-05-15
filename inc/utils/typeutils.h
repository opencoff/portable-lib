/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/typeutils.h - utilities that operate on types
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

#ifndef ___UTILS_TYPEUTILS_H__s7oiuSycd5RtbJFW___
#define ___UTILS_TYPEUTILS_H__s7oiuSycd5RtbJFW___ 1

#include <stdint.h>

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Functional style casts */
#define pUCHAR(x)    ((unsigned char*)(x))
#define pUINT(x)     ((unsigned int*)(x))
#define pULONG(x)    ((unsigned long*)(x))
#define pCHAR(x)     ((char*)(x))
#define pINT(x)      ((int*)(x))
#define pLONG(x)     ((long*)(x))

#define _ULLONG(x)   ((unsigned long long)(x))
#define _ULONG(x)    ((unsigned long)(x))
#define _UINT(x)     ((unsigned int)(x))
#define _PTRDIFF(x)  ((ptrdiff_t)(x))

#define pU8(x)       ((uint8_t *)(x))
#define pU16(x)      ((uint16_t *)(x))
#define pU32(x)      ((uint32_t *)(x))
#define pU64(x)      ((uint64_t *)(x))

#define pVU8(x)      ((volatile uint8_t *)(x))
#define pVU16(x)     ((volatile uint16_t *)(x))
#define pVU32(x)     ((volatile uint32_t *)(x))
#define pVU64(x)     ((volatile uint64_t *)(x))

#define _U8(x)       ((uint8_t)(x))
#define _U16(x)      ((uint16_t)(x))
#define _U32(x)      ((uint32_t)(x))
#define _U64(x)      ((uint64_t)(x))

#define _VU8(x)      ((volatile uint8_t)(x))
#define _VU16(x)     ((volatile uint16_t)(x))
#define _VU32(x)     ((volatile uint32_t)(x))
#define _VU64(x)     ((volatile uint64_t)(x))

#ifdef __cplusplus
#ifndef typeof
#define typeof(a)   __typeof__(a)
#endif
#endif


/* Align a quantity 'v' to 'n' byte boundary and return as type 'T". */
#define _ALIGN_UP(v, n)        (typeof(v))({ \
                                  uint64_t x_ = _U64(v); \
                                  uint64_t z_ = _U64(n) - 1; \
                                  (x_ + z_) & ~z_;})


#define _ALIGN_DOWN(v, n)    (typeof(v))({ \
                                uint64_t x_ = _U64(v); \
                                uint64_t z_ = _U64(n) - 1; \
                                x_ & ~z_;})

#define _IS_ALIGNED(v, n)    ({ \
                                uint64_t x_  = _U64(v); \
                                uint64_t z_ = _U64(n) - 1; \
                                x_ == (x_ & ~z_);})

#define _IS_POW2(n)          ({ \
                                uint64_t x_ = _U64(n); \
                                x_ == (x_ & ~(x_-1));})

/*
 * Round 'v' to the next closest power of 2.
 */
#define NEXTPOW2(v)     ({\
                            uint64_t n_ = _U64(v)-1;\
                            n_ |= n_ >> 1;      \
                            n_ |= n_ >> 2;      \
                            n_ |= n_ >> 4;      \
                            n_ |= n_ >> 8;      \
                            n_ |= n_ >> 16;     \
                            n_ |= n_ >> 32;     \
                            (typeof(v))(n_+1);  \
                      })





#ifdef __cplusplus
}

#include <cassert>

namespace putils {


/*
 * Some templates to make life easier
 */

// Simple template to align a qty to a desired boundary

// align 'p' to next upward boundary of 'n'
// i.e., the return value may be larger than 'p' but never less than
// 'p'
template <typename T> inline T
align_up(T p, unsigned long n)
{
    // n must be power of two
    assert(n == (n & ~(n-1)));
    uint64_t v = (uint64_t(p) + (n-1)) & ~(n-1);

    return T(v);
}


// align 'p' to nearest downward boundary of 'n'
// i.e., the return value will not be larger than 'p'
template <typename T> inline T
align_down(T p, unsigned long n)
{
    // n must be power of two
    assert(n == (n & ~(n-1)));
    uint64_t v = uint64_t(p) & ~(n-1);

    return T(v);
}


// Return true if 'p' is aligned at the 'n' byte boundary and false
// otherwise
template <typename T> inline bool
is_aligned(T p, unsigned long n)
{
    // n must be power of two
    assert(n == (n & ~(n-1)));
    return uint64_t(p) == (uint64_t(p) & ~uint64_t(n-1));
}

template <typename T> inline T
round_up_pow2(T z)
{
    uint64_t n = uint64_t(z);
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;

    return T(n+1);
}


} // namespace putils

#endif /* __cplusplus */

#endif /* ! ___UTILS_TYPEUTILS_H__s7oiuSycd5RtbJFW___ */

/* EOF */
