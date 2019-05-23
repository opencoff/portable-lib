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
#include "utils/new.h"
#include "utils/typeutils.h"


// Given n in bits, return # of u64's needed to hold it
#define _BITSIZE_W(n)   (_ALIGN_UP(n, 64)/64)



/* Dynamic bitset */
struct bitset
{
    uint64_t n;   // number of words
    uint64_t w[]; // storage for bits
};
typedef struct bitset bitset;



/*
 * Create a new bitset and return a pointer to it.
 */
static inline bitset*
bitset_new(size_t n)
{
    // n is number of bits
    uint64_t m = _BITSIZE_W(n); // # of u64
    void*    p = NEWZA(uint8_t, (sizeof(bitset) + (m * 8)));
    bitset*  b = (bitset *)p;

    b->n = m;
    return b;
}

static inline void
bitset_del(bitset *b)
{
    DEL(b);
}

/*
 * Allocate bitset on the stack and return it.
 * This _must_ be a macro - since alloca() is only meaningful in the
 * context of the current function.
 */
#define bitset_alloca(x) ({\
                            uint64_t n_ = (x);                   \
                            uint64_t m_ = _BITSIZE_W(n_);        \
                            void*    p_ = alloca(sizeof(bitset) + (8 *m_)); \
                            bitset*  b_ = (bitset *)p_;          \
                            b_->n = m_;                          \
                            memset(b_->w, 0, 8 * m_);            \
                            b_; \
                         })


/*
 * Duplicate bitset 'a' by allocating a new one on the stack.
 * This "function" _must_ be a macro - since alloca() is only
 * meaningful in the context of the current function.
 */
#define bitset_dup_alloca(a) ({\
                                bitset *a_  = (a);          \
                                uint64_t m_ = 8 * a_->n;   \
                                bitset *b_  = alloca(sizeof(bitset) + m_); \
                                b_->n = a_->n;                  \
                                memset(b_->w, 0, m_);           \
                                memcpy(b_->w, a_->w,  m_);      \
                                b_; \
                            })

// a = b (copy)
static inline void
bitset_copy(bitset *a, bitset *b)
{
    assert(a->n == b->n);
    memcpy(a->w, b->w, 8 * a->n);
}

/*
 * Duplicate bitset 'a', copy its contents to a newly created
 * instance and return it.
 */
static inline bitset*
bitset_dup(bitset *a)
{
    void   *p = NEWZA(uint8_t, sizeof(bitset) + (8 * a->n));
    bitset *b = (bitset*)p;

    b->n = a->n;
    memcpy(b->w, a->w, 8 * a->n);
    return b;
}

// Do: bit[i] = 1
static inline void
bitset_set(bitset *b, size_t i)
{
    assert(i < (64*b->n));
    uint64_t *w = &b->w[i / 64];

    *w |= (1 << (i % 64));
}


// do: bit[i] = 0
static inline void
bitset_clr(bitset *b, size_t i)
{
    assert(i < (64*b->n));
    uint64_t *w = &b->w[i / 64];

    *w &= ~(1 << (i % 64));
}


/*
 * Predicate: Return true if bit 'i' in 'b' is set, false otherwise.
 */
static inline int
bitset_isset(bitset *b, size_t i)
{
    assert(i < (64*b->n));
    uint64_t *w = &b->w[i / 64];
    return (*w & (1 << (i % 64))) > 0;
}

// return current value of bit 'i'
#define bitset_value(x,i)  bitset_isset(x,i)


// a |= b
static inline void
bitset_or(bitset *a, bitset *b)
{
    uint64_t *x = &a->w[0],
             *y = &b->w[0],
             *e = x + a->n;

    assert(a->n == b->n);
    for (; x < e; x++, y++) *x |= *y;
}


// a &= b
static inline void
bitset_and(bitset *a, bitset *b)
{
    uint64_t *x = &a->w[0],
             *y = &b->w[0],
             *e = x + a->n;

    assert(a->n == b->n);
    for (; x < e; x++, y++) *x &= *y;
}


// a ^= b
static inline void
bitset_xor(bitset *a, bitset *b)
{
    uint64_t *x = &a->w[0],
             *y = &b->w[0],
             *e = x + a->n;

    assert(a->n == b->n);
    for (; x < e; x++, y++) *x ^= *y;
}


// a &= ~b
static inline void
bitset_andnot(bitset *a, bitset *b)
{
    uint64_t *x = &a->w[0],
             *y = &b->w[0],
             *e = x + a->n;

    assert(a->n == b->n);
    for (; x < e; x++, y++) *x &= ~*y;
}


// a &= ~a
static inline void
bitset_not(bitset *a)
{
    uint64_t *x = &a->w[0],
             *e = x + a->n;

    for (; x < e; x++) *x = ~*x;
}




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __BITSETS_H__ */

/* EOF */
