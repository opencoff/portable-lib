/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * fast/simd.h - Portable SIMD for amd64 and arm64
 *
 * Copyright (c) 2025 Sudhi Herle <sw at herle.net>
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

#ifndef ___FAST_SIMD_H__geTEq1VjplXcRszm___
#define ___FAST_SIMD_H__geTEq1VjplXcRszm___ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
#include <immintrin.h>
#include <emmintrin.h>

typedef __m128i simd_vec128_t;

/* Load two 64-bit values into 128-bit vector */
#define SIMD_SET_EPI64X(hi, lo)     _mm_set_epi64x((hi), (lo))

/* Broadcast byte to all 16 positions */
#define SIMD_SET1_EPI8(byte)         _mm_set1_epi8((byte))
#define SIMD_CMPEQ_EPI8(a, b)        _mm_cmpeq_epi8((a), (b))
#define SIMD_MOVEMASK_EPI8(v)        _mm_movemask_epi8((v))
#define SIMD_PREFETCH_T0(addr)       _mm_prefetch((addr), _MM_HINT_T0)

#elif defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>

typedef uint8x16_t simd_vec128_t;

/* Load two 64-bit values into 128-bit vector */
#define SIMD_SET_EPI64X(hi, lo)     __arm64_simd_set_epi64(hi,lo)
#define SIMD_SET1_EPI8(b)           vdupq_n_u8(b)
#define SIMD_CMPEQ_EPI8(a, b)       vceqq_u8(a, b)
#define SIMD_MOVEMASK_EPI8(v)     __arm64_simd_movemask_epi8(v)
#define SIMD_PREFETCH_T0(addr)       __builtin_prefetch((addr), 0, 3)

static inline simd_vec128_t __arm64_simd_set_epi64(uint64_t hi, uint64_t lo) {
    uint64_t vals[2] = {lo, hi};
    return vreinterpretq_u8_u64(vld1q_u64(vals));
}

/* Extract comparison mask - ARM doesn't have movemask, need to emulate */
static inline int __arm64_simd_movemask_epi8(simd_vec128_t v) {
    /* Shift each byte's MSB to bit position 0 */
    uint8x16_t mask = vshrq_n_u8(v, 7);

    /* Pack into 16-bit value using pairwise operations */
    static const uint8_t shifts[16] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
    };
    uint8x16_t shift_vec = vld1q_u8(shifts);
    uint8x16_t masked = vandq_u8(mask, vdupq_n_u8(1));
    uint8x16_t shifted = vshlq_u8(masked, vreinterpretq_s8_u8(shift_vec));

    /* Sum all bytes to get final mask */
    return vaddvq_u8(shifted);
}


#else
#define __NO_SIMD__ 1
#endif /* Architecture selection */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___FAST_SIMD_H__geTEq1VjplXcRszm___ */

/* EOF */
