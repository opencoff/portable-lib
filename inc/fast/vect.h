/* :vi:ts=4:sw=4:
 *
 * vect.h - Typesafe, templatized Vector for "C"
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

#ifndef __VECTOR_2987113_H__
#define __VECTOR_2987113_H__

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "utils/new.h"


#ifdef __cplusplus
#ifndef typeof
#define typeof(a)   __typeof__(a)
#endif
#endif // __cplusplus

/*
 * Define a vector of type 'type'
 */
#define VECT_TYPE(nm, type)     \
    struct nm {                 \
        type * array;           \
        size_t size;            \
        size_t capacity;        \
    }


/*
 * typedef a vector of type 'type'. The typedef will have the same
 * name as the struct tag.
 */
#define VECT_TYPEDEF(nm, type)  typedef VECT_TYPE(nm,type) nm


/*
 * Initialize a vector to have at least 'cap' entries as its initial
 * capacity.
 */
#define VECT_INIT(v, cap)       do {                             \
                                    typeof(v) v_ = (v);          \
                                    size_t cap_ = (cap);         \
                                    if ( cap_ > 0 )  {           \
                                        if (cap_ < 16) cap_ = 16;\
                                        else {                   \
                                        }                        \
                                        v_->array = NEWZA(typeof(v_->array[0]), cap_);\
                                        v_->capacity = cap_;    \
                                    } else {                    \
                                        v_->array = 0;          \
                                        v_->capacity = 0;       \
                                    }                           \
                                    v_->size = 0;               \
                                } while (0)


/*
 * Delete a vector. Don't use the container after calling this
 * macro.
 */
#define VECT_FINI(v)       do {                        \
                                typeof(v) v_ = (v);    \
                                if (v_->array)         \
                                    DEL(v_->array);    \
                                v_->capacity = 0;      \
                                v_->size     = 0;      \
                                v_->array    = 0;      \
                            } while (0)


/*
 * Swap two vectors. Type-safe.
 */
#define VECT_SWAP(a, b)         do {                                         \
                                        size_t c0_ = (a)->capacity;          \
                                        size_t c1_ = (a)->size;              \
                                        typeof((a)->array) c2_ = (a)->array; \
                                        (a)->capacity = (b)->capacity;  \
                                        (a)->size     = (b)->size;      \
                                        (a)->array    = (b)->array;     \
                                        (b)->capacity = c0_;            \
                                        (b)->size     = c1_;            \
                                        (b)->array    = c2_;            \
                                    } while (0)

// internal helper macro that doesn't rename variables (assumes all
// vars are "unique" in their scope).
#define __vect_RESERVE(v_, want_)                                   \
                        do {                                        \
                            size_t n_ = v_->capacity;               \
                            if  ( want_ >= n_ ) {                   \
                                do { n_ *= 2; } while (n_ < want_); \
                                v_->array = RENEWA(typeof(v_->array[0]), v_->array, n_);\
                                memset(v_->array+v_->size, 0, sizeof(v_->array[0]) * (n_ - v_->capacity));\
                                v_->capacity = n_;     \
                            }                          \
                        } while (0)

/*
 * Reserve at least 'want' entries in the vector.
 * The _new_ capacity will be at least this much.
 */
#define VECT_RESERVE(v,want)    do {                        \
                                    typeof(v) v_ = (v);     \
                                    size_t w_    = want;    \
                                    __vect_RESERVE(v_, w_); \
                                } while (0)



/*
 * Ensure at least 'xtra' _additional_ entries in the vector.
 */
#define VECT_ENSURE(v,xtra)    do {\
                                    typeof(v) v_ = (v);           \
                                    size_t x_ = (xtra) + v_->size;\
                                    __vect_RESERVE(v_, x_);       \
                               } while (0)


/*
 * Get the next slot in the array - assumes the caller has
 * previously reserved space by using "VECT_ENSURE(v, 1)"
 */
#define VECT_GET_NEXT(v) (v)->array[(v)->size++]


/*
 * Append a new entry 'e' to the _end_ of the vector.
 */
#define VECT_PUSH_BACK(v,e)    do {                                 \
                                    typeof(v) v_ = (v);             \
                                    __vect_RESERVE(v_, v_->size+1); \
                                    v_->array[v_->size++] = e;      \
                                } while (0)


/*
 * Remove last element and store in 'e'
 * DO NOT call this if size is zero.
 */
#define VECT_POP_BACK(v)    ({ \
                                typeof(v) v_ = (v);     \
                                assert(v_->size > 0);   \
                                typeof(v_->array[0])   z_ = v_->array[--v_->size]; \
                            z_;})



/*
 * Put an entry at the front of the vector.
 */
#define VECT_PUSH_FRONT(v,e)         do {                             \
                                            typeof(v) v_ = v;         \
                                            __vect_RESERVE(v_, v_->size+1);         \
                                            typeof(e) * start_ = v_->array;         \
                                            typeof(e) * p_     = start_ + v_->size; \
                                            for (; p_ > start_; p_--)   \
                                                *p_ = *(p_ - 1);         \
                                            *p_ = (e);                   \
                                            v_->size++;                  \
                                      } while (0)


/*
 * Remove an entry at the front of the vector.
 */
#define VECT_POP_FRONT(v)           ({\
                                        typeof(v) v_ = v;           \
                                        typeof(v_->array[0]) *p_   = v_->array;      \
                                        typeof(v_->array[0]) *end_ = p_ + --v_->size;\
                                        typeof(v_->array[0]) e_    = *p_;            \
                                        for (; p_ < end_; p_++) \
                                            *p_ = *(p_ + 1);    \
                                        e_;\
                                     })



/*
 * Reset the size of the vector without freeing any memory.
 */
#define VECT_RESET(v)   do { \
                            (v)->size = 0; \
                        } while (0)



/*
 * Copy a vector from 's' to 'd'.
 */
#define VECT_COPY(d, s)                     \
                    do {                    \
                        typeof(d) a_ = (d); \
                        typeof(s) b_ = (s); \
                        __vect_RESERVE(a_, b_->size);\
                        memcpy(a_->array, b_->array, b_->size * sizeof(b_->array[0])); \
                        a_->size = b_->size; \
                    } while (0)


/*
 * Append another vector to the end of this vector
 */
#define VECT_APPEND_VECT(d, s)              \
                    do {                    \
                        typeof(d) a_ = (d); \
                        typeof(s) b_ = (s); \
                        __vect_RESERVE(a_, a_->size + b_->size);\
                        memcpy(a_->array+a_->size, b_->array, b_->size * sizeof(b_->array[0])); \
                        a_->size += b_->size; \
                    } while (0)

/*
 * Sort the vector 'v' using the sort function 'cmp'.
 * The sort function must use the same type signature as qsort()
 */
#define VECT_SORT(v, cmp) do { \
        qsort((v)->array, (v)->size, sizeof((v)->array[0]), \
                (int (*)(const void*, const void*)) cmp); \
    } while (0)



/*
 * Fisher Yates Shuffle - randomize the elements.
 */
#define VECT_SHUFFLE(v, rnd) do {                                   \
                                typeof(v) v_ = (v);                 \
                                typeof(v_->array) p_ = v_->array;   \
                                typeof(p_[0]) e_;                   \
                                int i_;                             \
                                for (i_= v_->size-1; i_ > 0; i_--){ \
                                    int j_ = rnd() % (i_+1);        \
                                    e_     = p_[i_];                \
                                    p_[i_] = p_[j_];                \
                                    p_[j_] = e_;                    \
                                }                                   \
                            } while (0)


/*
 * Reservoir sampling: choose k random elements with equal probability.
 *
 * Read 'k' uniform random samples from 's' and write to 'd' - using
 * random number generator rnd(). 'rnd' is expected to generate true
 * random numbers with the full width of size_t (at least 32 bits).
 *
 * Note that sampling is only meaningful when 's' has more than 'k'
 * elements.
 *
 * Return false if 's' has fewer than 'k' elements, true otherwise.
 */
#define VECT_SAMPLE(d, s, k, rnd) ({    int z_ = 0;                             \
                                        size_t k_ = (k);                        \
                                        typeof(s) b_ = (s);                     \
                                        if (k_ <  b_->size) {                   \
                                            typeof(d) a_ = (d);                 \
                                            typeof(a_->array) x_ = a_->array;   \
                                            typeof(a_->array) y_ = b_->array;   \
                                            size_t i_;                          \
                                            __vect_RESERVE(a_, k_);             \
                                            a_->size = k_;                      \
                                            for (i_=0; i_ < k_; i_++)           \
                                                x_[i_] = y_[i_];                \
                                            for (i_=k_; i_ < b_->size; i_++) {  \
                                                size_t j_ = ((size_t)rnd()) % (i_+1);       \
                                                if (j_ < k_) x_[j_] = y_[i_];   \
                                            }                                   \
                                            z_ = 1;                             \
                                        }                                       \
                                        z_;})



/*
 * Return pointer to a random element uniformly chosen from the vector.
 * Use 'rnd()' as the uniform random number generator.
 * 'rnd' is expected to generate true random numbers with the full
 * width of size_t (at least 32 bits).
 */
#define VECT_RAND_ELEM(v, rnd) ({   typeof(v) a_ = v;            \
                                    size_t n_ = ((size_t)rnd()) % a_->size; \
                                 &a_->array[n_];})

/*
 * Iterate through vector 'v' and set each element to the pointer
 * 'p'
 */
#define VECT_FOR_EACH(v, p)     for (p = &(v)->array[0];        p < &VECT_END(v); ++p)
#define VECT_FOR_EACHi(v, i, p) for (p = &(v)->array[0], i = 0; i < (v)->size;    ++p, ++i)

/* -- Various accessors -- */

/*
 * Get the first and last element (both are valid entries).
 * No validation is done with respect to size etc.
 */
#define VECT_FIRST_ELEM(v)     ((v)->array[0])
#define VECT_LAST_ELEM(v)      ((v)->array[(v)->size-1])
#define VECT_TOP(v)            VECT_LAST_ELEM(v)

/*
 * One past the last element. Useful when iterating with vectors
 */
#define VECT_END(v)            ((v)->array[(v)->size])

/* Fetch the n'th element */
#define VECT_ELEM(v,n)  ((v)->array[(n)])

#define VECT_LEN(v)        ((v)->size)
#define VECT_LEN(v)         ((v)->size)
#define VECT_CAPACITY(v)    ((v)->capacity)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __VECTOR__2987113_H__ */

/* EOF */
