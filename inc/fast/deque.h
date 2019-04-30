/* :vi:ts=4:sw=4:
 * 
 * dequeu.h - Typesafe, templatized double-ended vect for "C"
 *
 * Copyright (c) 2000-2010 Sudhi Herle <sw at herle.net>
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

#ifndef __DEQUE_2987113_H__
#define __DEQUE_2987113_H__

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
#include <stdlib.h>
#include "utils/utils.h"
    

#ifdef __cplusplus
#ifndef typeof
#define typeof(a)   __typeof__(a)
#endif
#endif

/*
 * Define a vector of type 'type'
 */
#define DEQUE_TYPE(nm, type)    \
    struct nm {                 \
        type * array;           \
        size_t start;           \
        size_t size;            \
        size_t max;             \
    }


/*
 * typedef a vector of type 'type'. The typedef will have the same
 * name as the struct tag.
 */
#define DEQUE_TYPEDEF(nm, type)  typedef DEQUE_TYPE(nm,type) nm


/*
 * Initialize a vector to have at least 'cap' entries as its initial
 * max.
 */
#define DEQUE_INIT(v, cap) \
    do {                        \
        typeof(v) v_ = (v);     \
        int cap_ = (cap);       \
        if (cap_ < 16) cap_ = 8;\
        cap_ *= 2;              \
        v_->array = NEWZA(typeof(v_->array[0]), cap_);\
        v_->max   = cap_;      \
        v_->size  = 0;         \
        v_->start = cap_ >> 1; \
    } while (0)


/*
 * Delete a vector. Don't use the container after calling this
 * macro.
 */
#define DEQUE_FINI(v)       \
    do {                    \
        DEL((v)->array);    \
        (v)->max   = 0;     \
        (v)->size  = 0;     \
        (v)->array = 0;     \
    } while (0)


/*
 * Reserve at least 'want' entries in the vector.
 * The _new_ max will be at least this much.
 */
#define DEQUE_RESERVE(v,want)       \
    do {                            \
        int n_ = (v)->max;          \
        int want_ = want;           \
        if  ( want_ >= n_ ) {       \
            do { n_ *= 2; } while (n_ < want_); \
            (v)->array = RENEWA(typeof((v)->array[0]), (v)->array, n_);\
            memset((v)->array+(v)->size, 0, sizeof((v)->array[0]) * (n_ - (v)->size));\
            (v)->max = n_;     \
        }                           \
    } while (0)



/*
 * Ensure at least 'xtra' _additional_ entries in the vector.
 */
#define DEQUE_ENSURE(v,xtra)    DEQUE_RESERVE(v, (xtra)+(v)->size)


/*
 * Get the next slot in the array - assumes the caller has
 * previously reserved space by using "DEQUE_ENSURE(v, 1)"
 */
#define DEQUE_GET_NEXT(v) (v)->array[(v)->size++]


/*
 * Append a new entry 'e' to the _end_ of the vector.
 */
#define DEQUE_PUSH_BACK(v,e)    \
    do {                       \
        DEQUE_ENSURE(v,1);    \
        (v)->array[(v)->size++] = e;\
    } while (0)

/* Handy synonym for PUSH_BACK()  */
#define DEQUE_APPEND(v,e)  DEQUE_PUSH_BACK(v,e)


/*
 * Remove last element and store in 'e'
 * DO NOT call this if size is zero.
 */
#define DEQUE_POP_BACK(v)    ({ \
                                assert((v)->size > 0); \
                                typeof((v)->array[0])   zz = (v)->array[--(v)->size]; \
                            zz;})



#if 0

#define DEQUE_ADD_AT(v,e,n)      \
    do {                        \
        DEQUE_RESERVE(v,n);      \
        (v)->array[(n)] = e;    \
        if ( (n) >= (v)->size ) \
            (v)->size = (n)+1;  \
    } while (0)


#define DEQUE_RM_AT(v,rm,n) \
    do { \
        if ( (n) < (v)->size ) { \
            typeof(rm) * _x = &(v)->array[0]; \
            typeof(rm) * _e = _x + --(v)->size; \
            _x += (n); \
            rm = *_x;\
            for (; _x < _e; ++_x) \
                *_x = *(_x+1); \
        } \
    } while (0)
#endif


/*
 * Put an entry at the front of the vector.
 */
#define DEQUE_PUSH_FRONT(v,e)         \
    do {                             \
        typeof(e) * p_;              \
        typeof(e) * start_;          \
        DEQUE_ENSURE(v,1);          \
        start_ = (v)->array + 1;     \
        p_ = start_ + (v)->size - 1; \
        for (; p_ >= start_; p_--)   \
            *p_ = *(p_ - 1);         \
        *p_ = (e);                   \
        (v)->size++;                 \
    } while (0)


/*
 * Remove an entry at the front of the vector.
 */
#define DEQUE_POP_FRONT(v,e)\
    do {                        \
        typeof(e) * p_ = (v)->array; \
        typeof(e) * end_ = p_ + --(v)->size;\
        (e) = *p_;              \
        for (; p_ < end_; p_++) \
            *p_ = *(p_ + 1);    \
    } while (0)



/*
 * Reset the size of the vector without freeing any memory.
 */
#define DEQUE_RESET(v)   \
    do { \
        (v)->size = 0; \
    } while (0)



/*
 * Copy a vector from 's' to 'd'.
 */
#define DEQUE_COPY(d, s) \
    do { \
        DEQUE_RESERVE(d, (s)->size); \
        memcpy((d)->array, (s)->array, (s)->size * sizeof((s)->array[0])); \
        (d)->size = (s)->size; \
    } while (0)


/*
 * Append another vector to the end of this vector
 */
#define DEQUE_APPEND_VECT(d, s) \
    do { \
        DEQUE_ENSURE(d, (s)->size); \
        memcpy((d)->array+(d)->size, (s)->array, (s)->size * sizeof((s)->array[0])); \
        (d)->size += (s)->size; \
    } while (0)

/*
 * Sort the vector 'v' using the sort function 'cmp'.
 * The sort function must use the same type signature as qsort()
 */
#define DEQUE_SORT(v, cmp) \
    do { \
        qsort((v)->array, (v)->size, sizeof((v)->array[0]), \
                (int (*)(const void*, const void*)) cmp); \
    } while (0)



/*
 * Fisher Yates Shuffle - randomize the elements.
 */
#define DEQUE_SHUFFLE(v, rnd) do {                                   \
                                typeof(v) _v = (v);                 \
                                typeof(_v->array) _p = _v->array;   \
                                typeof(_v->array[0]) _e;            \
                                int _i;                             \
                                for (_i= _v->size; --_i > 0; ) {    \
                                    int _j = rnd() % _v->size;      \
                                    _e     = _p[_i];                \
                                    _p[_i] = _p[_j];                \
                                    _p[_j] = _e;                    \
                                }                                   \
                            } while (0)


/*
 * Iterate through vector 'v' and set each element to the pointer
 * 'p'
 */
#define DEQUE_FOR_EACH(v, p)     for (p = &(v)->array[0];        p < &DEQUE_END(v); ++p)
#define DEQUE_FOR_EACHi(v, i, p) for (p = &(v)->array[0], i = 0; i < (v)->size;    ++p, ++i)

/* -- Various accessors -- */

/*
 * Get the first and last element (both are valid entries).
 * No validation is done with respect to size etc.
 */
#define DEQUE_FIRST_ELEM(v)     ((v)->array[0])
#define DEQUE_LAST_ELEM(v)      ((v)->array[(v)->size-1])
#define DEQUE_TOP(v)            DEQUE_LAST_ELEM(v)

/*
 * One past the last element. Useful when iterating with vectors
 */
#define DEQUE_END(v)            ((v)->array[(v)->size])

/* Fetch the n'th element */
#define DEQUE_ELEM(v,n)  ((v)->array[(n)])

#define DEQUE_SIZE(v)        ((v)->size)
#define DEQUE_CAPACITY(v)    ((v)->max)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __DEQUE__2987113_H__ */

/* EOF */
