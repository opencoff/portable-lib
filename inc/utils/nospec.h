/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * nospec.h - Cross platform workarounds for speculation
 * related hijinx.
 * 
 * Largely borrowed from the linux kernel.
 *
 * Copyright (c) 2024 Sudhi Herle <sw at herle.net>
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

#ifndef ___INC_UTILS_NOSPEC_H__ADtJQlNp0ubf25oq___
#define ___INC_UTILS_NOSPEC_H__ADtJQlNp0ubf25oq___ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <inttypes.h>


#ifndef OPTIMIZER_HIDE_VAR
/* Make the optimizer believe the variable can be manipulated arbitrarily. */
#define OPTIMIZER_HIDE_VAR(var) __asm__ ("" : "=r" (var) : "0" (var))
#endif

static inline uint64_t
__array_index_mask_nospec(uint64_t index, uint64_t size)
{
        /*
         * Always calculate and emit the mask even if the compiler
         * thinks the mask is not needed. The compiler does not take
         * into account the value of @index under speculation.
         */
        OPTIMIZER_HIDE_VAR(index);
        return ~(int64_t)(index | (size - 1UL - index)) >> (64 - 1);
}


/*
 * array_index_nospec - sanitize an array index after a bounds check
 *
 * For a code sequence like:
 *
 *     if (index < size) {
 *         index = array_index_nospec(index, size);
 *         val = array[index];
 *     }
 *
 * ...if the CPU speculates past the bounds check then
 * array_index_nospec() will clamp the index within the range of [0,
 * size).
 */
#define array_index_nospec(index, size)                                 \
({                                                                      \
        typeof(index) _i = (index);                                     \
        typeof(size)  _s = (size);                                      \
        uint64_t   _mask = __array_index_mask_nospec(_i, _s);           \
        (typeof(_i)) (_i & _mask);                                      \
})


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___INC_UTILS_NOSPEC_H__ADtJQlNp0ubf25oq___ */

/* EOF */
