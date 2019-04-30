/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * mpmc_bounded_queue.h - Lock-Free, MPMC bounded queues.
 *
 * Inspired by Dmitry Vyukov's C++ code.
 *
 * Copyright (C) 2005-2010, Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Introduction
 * ============
 *   Macros to create and manipulate "type indepedent" fast,
 *   bounded, circular queues.
 *   - These are lock-free and require C11 <stdatomic.h>
 *   - The Q is lock-free but NOT wait-free.
 *   - Enq/Deq operations require one CAS.
 *
 *   Use:
 *     Make a type for your queue to hold objects of your
 *     interest. e.g.,
 *
 *          MPMCQ_TYPE(typename, type, Q_SIZE);
 *
 *     This makes a new Queue data type (typedef) called 'typename'
 *     to hold objects of type 'type'. The size of the circular
 *     Queue is 'size' elements.
 *     Next, declare one or more objects of type 'typename':
 *          typename my_queue;
 *
 *     Operations allowed:
 *       o MPMCQ_INIT(my_queue, Q_size)
 *            MUST be called before any use of other macros. Q_size
 *            MUST be the same as the one used for MPMCQ_TYPE().
 *       o MPMCQ_DYN_INIT(my_queue, q_size)
 *            Call this function if you don't know the size of the
 *            queue at compile time. Only _ONE_ of MPMCQ_INIT() or
 *            MPMCQ_DYN_INIT() must be called.
 *       o MPMCQ_ENQ(my_queue, elem, status)
 *            Enqueue 'elem' into 'my_queue' and obtain result in
 *            'status'. If status != 0 then Queue-full.
 *       o MPMCQ_DEQ(my_queue, elem, status)
 *            Dequeue an element from 'my_queue' and put the element
 *            into 'elem'. If status != 0 then Queue-empty.
 *
 *
 *  The 'R' index tracks the next slot from which to read.
 *  The 'W' index tracks the next slot ready to accept a write.
 *
 *  Both 'R' and 'W' increment without wrapping; the only wrapping
 *  happens when they reach their boundaries.
 */

#ifndef __MPMC_BOUNDED_QUEUE_31741145_H__
#define __MPMC_BOUNDED_QUEUE_31741145_H__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdatomic.h>
#include <assert.h>
#include "utils/utils.h"

/*
 * Sensible default for modern 64-bit processors.
 *
 * No compiler provides us with a pragma or CPP macro for the cache
 * line size. Boo.
 *
 * If your CPU has a different cache line size, define this macro on
 * the command line (-DCACHELINE_SIZE=xxx).
 */
#ifndef CACHELINE_SIZE
#define CACHELINE_SIZE      64
#endif


#ifdef __GNUC__
#define __CACHELINE_ALIGNED __attribute__((aligned(CACHELINE_SIZE)))
#elif defined(_MSC_VER)
#define __CACHELINE_ALIGNED __declspec(align(CACHELINE_SIZE))
#else
#error "Can't define __CACHELINE_ALIGNED for your compiler/machine"
#endif /* __CACHELINE_ALIGNED */

#ifdef __cplusplus
#ifndef typeof
#define typeof(a)   __typeof__(a)
#endif
#endif

/*
 * We put the rd, wr and sz elements in different cache lines by
 * forcibly padding the remaining space in this cache line.
 */
#define __mpmc_padz(n)      ((CACHELINE_SIZE - (n))/sizeof(uint32_t))
struct __mpmcq
{
    atomic_uint_fast64_t rd; /* Next slot from which to read */
    uint32_t __pad0[__mpmc_padz(8)];

    atomic_uint_fast64_t wr; /* Last successful write */
    uint32_t __pad1[__mpmc_padz(8)];

    uint64_t sz;    /* Capacity of the queue: Always power of 2 */
    uint64_t mask;  /* Mask to make future computation faster */
    uint32_t __pad2[__mpmc_padz(16)];
};
typedef struct __mpmcq __mpmcq;



/*
 * MPMC Node. This captures the notion of a queue-element and a tag.
 */

#define MPMCQ_NODETY(ty)        ty ## _node
#define MPMCQ_NODE(typnm, ty)   struct __CACHELINE_ALIGNED MPMCQ_NODETY(typnm) \
                                { \
                                    atomic_uint_fast64_t seq; \
                                    ty   d;\
                                };\
                                typedef struct __CACHELINE_ALIGNED MPMCQ_NODETY(typnm) MPMCQ_NODETY(typnm)




/* Create a new queue called 'typnm' of a fixed size 'sz' to hold
 * elements of type * 'ty'. */
#define MPMCQ_TYPEDEF(typnm_,ty_,sz_) \
                                 MPMCQ_NODE(typnm_, ty_); \
                                 struct typnm_            \
                                 {                        \
                                     __mpmcq q;           \
                                     MPMCQ_NODETY(typnm_) elem[sz_];\
                                 }; \
                                 typedef struct typnm_ typnm_


/*
 * Create a "dynamic" fast-queue whose size will be initialized
 * later. The actual memory will be allocated via malloc().
 */
#define MPMCQ_DYN_TYPEDEF(typenm_,ty_) \
                                 MPMCQ_NODE(typenm_, ty_); \
                                 struct typenm_       \
                                 {                    \
                                     __mpmcq q;       \
                                     MPMCQ_NODETY(typenm_) *elem;\
                                 };                   \
                                typedef struct typenm_ typenm_


/* Handy shorthand for the type of the queue element */
#define __mpmcqetyp(q)      typeof((q)->elem[0])


/*
 * Initialize a queue.
 */
static inline __mpmcq*
__mpmcq_init(__mpmcq* q, size_t n)
{
    assert(0 == (n & (n-1)));   /* always a power of 2 */

    atomic_init(&q->rd, 0);
    atomic_init(&q->wr, 0);
    q->sz   = n;
    q->mask = n - 1;
    return q;
}


/*
 * Return best guess for size of queue. This function cannot
 * guarantee accurate results in the presence of multiple concurrent
 * readers and writers.
 */
static inline uint32_t
__mpmcq_size(__mpmcq* q)
{
    uint_fast64_t rd = atomic_load_explicit(&q->rd, memory_order_consume);
    uint_fast64_t wr = atomic_load_explicit(&q->wr, memory_order_consume);

    uint64_t n = wr - rd;

    // 64-bit seq# wrap around
    if (rd > wr) n = ~0 - n;
    return n;
}


/* Return true if queue is full */
static inline int
__mpmcq_full_p(__mpmcq * q)
{
    return q->sz == __mpmcq_size(q);
}

/* Return true if queue is empty */
static inline int
__mpmcq_empty_p(__mpmcq * q)
{
    uint_fast64_t rd = atomic_load_explicit(&q->rd, memory_order_consume);
    uint_fast64_t wr = atomic_load_explicit(&q->wr, memory_order_consume);

    return rd == wr;
}


#define XCHG(a, b, c)   atomic_compare_exchange_weak_explicit(a, b, c, memory_order_relaxed, memory_order_relaxed)

#ifndef _BACKOFF_MIN
#define _BACKOFF_MIN    128
#endif

#ifndef _BACKOFF_MAX
#define _BACKOFF_MAX    1024
#endif


/*
 * Exponential delay with max value.
 * This loop is run when we see contention on a cell.
 * Idea due to: Massimo Torquati (fastflow)
 */
#define _exp_backoff(bk) do {                           \
                            volatile unsigned int _i;   \
                            for(_i = 0; _i < bk; ++_i); \
                            bk <<= 1;                   \
                            bk &= _BACKOFF_MAX;         \
                         } while (0)



/*
 * Enqueue element 'e_'
 * Return:
 *    True  on success
 *    False on Queue full
 */
#define MPMCQ_ENQ(q_, e_) ({                            \
                    __mpmcq* _q = &(q_)->q;             \
                    __mpmcqetyp(q_) * _nd;              \
                    int _z = 0;                         \
                    unsigned int _bk = _BACKOFF_MIN;    \
                    uint_fast64_t _p, _seq;             \
                    for (;;) {                          \
                        _p   = atomic_load_explicit(&_q->wr, memory_order_relaxed);     \
                        _nd  = &(q_)->elem[_p & _q->mask];                              \
                        _seq = atomic_load_explicit(&_nd->seq, memory_order_acquire);   \
                        if (_p == _seq) {                       \
                            if (XCHG(&_q->wr, &_p, (_p+1))) {   \
                                _z     = 1;                     \
                                _nd->d = e_;                    \
                                atomic_store_explicit(&_nd->seq, _seq+1, memory_order_release);\
                                break;                  \
                            } else _exp_backoff(_bk);   \
                        } else if (_p > _seq) {         \
                            _z = 0;                     \
                            break;                      \
                        }                               \
                    }                                   \
                    _z;})




/*
 * Dequeue into 'e_'
 * Return:
 *    True  on success
 *    False on Queue empty
 */
#define MPMCQ_DEQ(q_, e_)   ({                          \
                    __mpmcq* _q = &(q_)->q;             \
                    __mpmcqetyp(q_) * _nd;              \
                    int _z = 0;                         \
                    unsigned int _bk = _BACKOFF_MIN;    \
                    uint_fast64_t _p, _seq;             \
                    for (;;) {                          \
                        _p   = atomic_load_explicit(&_q->rd, memory_order_relaxed);     \
                        _nd  = &(q_)->elem[_p & _q->mask];                              \
                        _seq = atomic_load_explicit(&_nd->seq, memory_order_acquire);   \
                        int64_t _diff = ((int64_t)_seq) - ((int64_t)(_p+1));            \
                        if (_diff == 0) {                       \
                            if (XCHG(&_q->rd, &_p, (_p+1))) {   \
                                _z = 1;                         \
                                e_ = _nd->d;                    \
                                atomic_store_explicit(&_nd->seq, _p+_q->mask+1, memory_order_release);\
                                break;                  \
                            } else _exp_backoff(_bk);   \
                        } else if (_diff < 0 ) {        \
                            _z = 0;                     \
                            break;                      \
                        }                               \
                    } \
                    _z; })




/*
 * Internal function for validating the sequence# of the elements in
 * the queue.
 *
 * Returns: # of errors detected.
 *
 * XXX Do NOT call this from a multi-threaded context!
 */
#define __MPMCQ_CONSISTENCY_CHECK(qx) ({                \
                        typeof(qx) q_ = (qx);           \
                        __mpmcq* q0 = &q_->q;           \
                        uint64_t z = __mpmcq_size(q0);  \
                        uint64_t r = atomic_load_explicit(&q0->rd, memory_order_relaxed);     \
                        uint64_t seq = atomic_load_explicit(&q_->elem[r & q0->mask].seq, memory_order_acquire);\
                        uint64_t ii;                    \
                        int err = 0;                    \
                        for (ii = 0; ii < z; ii++) {    \
                            r = ii+atomic_load_explicit(&q0->rd, memory_order_relaxed);     \
                            uint64_t ss = atomic_load_explicit(&q_->elem[r & q0->mask].seq, memory_order_acquire);\
                            if (ss == seq) seq++;       \
                            else err++;                 \
                        }       \
                        err; })



/* Reset Queue to empty */
#define MPMCQ_RESET(q_)                      \
                            do {             \
                                __mpmcq * _q = &(q_)->q;  \
                                typeof(q_) _mq = q_;      \
                                atomic_init(&_q->rd, 0);  \
                                atomic_init(&_q->wr, 0);  \
                                __mpmcq_node_init(_mq, _q->sz); \
                            } while (0)


/* Predicates that answer true/false */
#define MPMCQ_FULL_P(q_)       __mpmcq_full_p(&(q_)->q)
#define MPMCQ_EMPTY_P(q_)      __mpmcq_empty_p(&(q_)->q)


/* Number of elements in the queue */
#define MPMCQ_SIZE(q_)         __mpmcq_size(&(q_)->q)



/*
 * Initialize a dynamic queue.
 */


#define __mpmcq_node_init(q_, sz_)   do {                 \
                size_t x_;                                \
                for (x_ = 0; x_ < sz_; x_++)              \
                    atomic_init(&(q_)->elem[x_].seq, x_); \
            } while (0)


/* Initialize queue 'q_' for size 'sz_' */
#define MPMCQ_INIT(q_,sz_)  do {                             \
                                size_t zz_ = NEXTPOW2(sz_);  \
                                __mpmcq_init(&(q_)->q, zz_); \
                                __mpmcq_node_init(q_, zz_);  \
                            } while (0)



// Nothing to free.
// XXX Should we memset the space to something garbage?
#define MPMCQ_FINI(q)       do {                            \
                                memset(q, 0xaa, sizeof *q); \
                            } while (0)


#define MPMCQ_DYN_INIT(q_, sz_) do {                                \
                                size_t zz_   = NEXTPOW2(sz_);       \
                                __mpmcq_init(&(q_)->q, zz_);        \
                                (q_)->elem   = NEWZA(__mpmcqetyp(q_), zz_); \
                                __mpmcq_node_init(q_, zz_);         \
                            } while (0)


/*
 * Finalize a dynamic queue.
 */
#define MPMCQ_DYN_FINI(q_)       do {                    \
                                __mpmcq * _q = &(q_)->q; \
                                _q->sz = 0;              \
                                DEL((q_)->elem);         \
                                (q_)->elem = 0;          \
                            } while (0)



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __MPMC_BOUNDED_QUEUE_31741145_H__ */

/* EOF */
