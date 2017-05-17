/* :vi:ts=4:sw=4:
 * 
 * bitset.h - Arbitrarily large bitsets.
 *  Portable "C" bitsets
 *
 * Copyright (c) 2005 Sudhi Herle <sw at herle.net>
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

#ifndef __BITSETS_H__
#define __BITSETS_H__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <assert.h>
#include <stdint.h>
#include "utils/utils.h"


#define _WORDBITS       (8 * sizeof(uint64_t))

#define _modrem(x)      (((x) % _WORDBITS) != 0)
#define _bits2words(x)  (_modrem(x) + ((x) / _WORDBITS))
#define _wordoff(x)     ((x) / _WORDBITS)
#define _bitoff(x)      ((x) % _WORDBITS)
#define _mask(x)        (1 << _bitoff(x))


/* Dynamic bitset */
struct bitset
{
    uint64_t n;
    uint64_t bm[];
};
typedef struct bitset bitset;




/*
 * Create a new bitset and return a pointer to it.
 */
#define BITSET_NEW(n_)  ({ \
                            uint64_t n = n_;    \
                            uint64_t m = _bits2words(n); \
                            bitset*  b = (bitset*)NEWZA(uint8_t, sizeof(bitset) + (m*8)); \
                            b->n = n; \
                            b;  \
                         })
                

#define BITSET_ALLOCA(n_)   ({ \
                            uint64_t n = n_;    \
                            uint64_t m = _bits2words(n); \
                            bitset*  b = (bitset*)alloca(uint8_t, sizeof(bitset) + (m*8)); \
                            b->n = n; \
                            b;  \
                            )}


#define BITSET_DEL(b)       do { \
                                DEL(b); \
                            } while (0)


/*
 * b[i] = 1
 */
#define BITSET_SET(b, i)    do { \
                                assert(_U64(i) < (b)->n); \
                                uint64_t* bm = &(b)->bm[_wordoff(i)]; \
                                *bm |= _mask(i); \
                            } while (0)


/*
 * b[i] = 0
 */
#define BITSET_CLR(b, i)    do  { \
                                assert(_U64(i) < (b)->n); \
                                uint64_t * x_ = &(b)->bm[_wordoff(i)]; \
                                *x_ &= ~_mask(i); \
                            } while (0)




/*
 *  Generic b1 OP= b2
 */
#define __BITSET_OP(b1,b2,OP)   do { \
                                    assert((b1)->n == (b2)->n);   \
                                    uint64_t * x_ = &(b1)->bm[0], \
                                             * y_ = &(b2)->bm[0], \
                                             * e_ = x_ + _bits2words((b1)->n); \
                                    for(; x_ < e_, ++x_, ++y_)    \
                                            *x_ OP= *y_;          \
                                } while (0)


#define BITSET_AND(b1,b2)   __BITSET_OP(b1,b2, &)
#define BITSET_OR(b1, b2)   __BITSET_OP(b1,b2, |)
#define BITSET_XOR(b1,b2)   __BITSET_OP(b1,b2, ^)

#define BITSET_NOT(b)       do { \
                                uint64_t * x_ = &(b1)->bm[0], \
                                         * e_ = x_ + _bits2words((b1)->n); \
                                for(; x_ < e_, ++x_) *x_ = ~*x;         \
                            } while (0)




/*
 * Predicate: Return true if bit 'i' in 'b' is set, false otherwise.
 */
#define BITSET_IS_SET(b, i)     ({ \
                                    assert(_U64(i) < (b)->n);                 \
                                    ((b)->bm[_wordoff(i)] & _mask(i));  \
                                })





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __BITSETS_H__ */

/* EOF */
