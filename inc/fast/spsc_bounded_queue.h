/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * spsc_bounded_queue.h - Lock-Free, SPSC bounded queues.
 *
 * This is for SINGLE PRODUCER, SINGLE CONSUMER concurrency.
 * Do NOT use it if you have multiple producers or consumers.
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
 *   Macros to create and manipulate "type indepedent" fast circular
 *   queues. These are lock-free and require C11 <stdatomic.h>
 *
 *   Use:
 *     Make a type for your queue to hold objects of your
 *     interest. e.g.,
 *
 *          SPSCQ_TYPE(typename, type, Q_SIZE);
 *
 *     This makes a new Queue data type (typedef) called 'typename'
 *     to hold objects of type 'type'. The size of the circular
 *     Queue is 'size' elements.
 *     Next, declare one or more objects of type 'typename':
 *          typename my_queue;
 *
 *     Operations allowed:
 *       o SPSCQ_INIT(my_queue, Q_size)
 *            MUST be called before any use of other macros. Q_size
 *            MUST be the same as the one used for SPSCQ_TYPE().
 *       o SPSCQ_ENQ(my_queue, elem, status)
 *            Enqueue 'elem' into 'my_queue' and obtain result in
 *            'status'. If status != 0 then Queue-full.
 *       o SPSCQ_DEQ(my_queue, elem, status)
 *            Dequeue an element from 'my_queue' and put the element
 *            into 'elem'. If status != 0 then Queue-empty.
 *       o SPSCQ_PEEK(my_queue, elem, status)
 *            Peek at the element at the head of the queue. DOES NOT
 *            change any queue pointers. status != 0 if Queue-empty.
 *       o SPSCQ_FLUSH(my_queue)
 *            "Flush" queue contents.
 *
 *
 *  The 'R' index tracks the next slot from which to read.
 *  The 'W' index tracks the next slot ready to accept a write.
 *
 *  Thus, the conditions for queue full and queue empty are the
 *  same. This simplicification means one queue slot will always go
 *  unused.
 */

#ifndef __SPSC_BOUNDED_QUEUE_31741145_H__
#define __SPSC_BOUNDED_QUEUE_31741145_H__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdatomic.h>
#include "utils/utils.h"

/*
 * Sensible default for modern 64-bit processors (4 64-bit words).
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

#define __PADSZ(x)  ((CACHELINE_SIZE - sizeof(x))/sizeof(uint32_t))

#ifdef __cplusplus
#ifndef typeof
#define typeof(a)   __typeof__(a)
#endif
#endif

/*
 * We put the rd, wr and sz elements in different cache lines by
 * forcibly padding the remaining space in this cache line.
 */
struct __spscq
{
    atomic_uint_fast32_t rd; /* Next slot from which to read */
    uint32_t __pad0[__PADSZ(atomic_uint_fast32_t)];

    atomic_uint_fast32_t wr; /* Last successful write */
    uint32_t __pad1[__PADSZ(atomic_uint_fast32_t)];

    uint32_t sz;    /* Capacity of the queue */
    uint32_t __pad2[__PADSZ(atomic_uint_fast32_t)];
};
typedef struct __spscq __spscq;





/* Create a new queue called 'typnm' of a fixed size 'sz' to hold
 * elements of type * 'ty'. */
#define SPSCQ_TYPE(typnm_,ty_,sz_) struct typnm_          \
                                 {                      \
                                     __spscq q;           \
                                     ty_ elem[(sz_)];   \
                                 }

#define SPSCQ_TYPEDEF(typnm_,ty_,sz_) typedef SPSCQ_TYPE(typnm_,ty_,sz_) typnm_

/*
 * Create a "dynamic" fast-queue whose size will be initialized
 * later. The actual memory will be allocated via malloc().
 */
#define SPSCQ_DYN_TYPEDEF(typenm_,ty_) \
                                 struct typenm_       \
                                 {                    \
                                     __spscq q;         \
                                     ty_ *elem;       \
                                 };                   \
                                typedef struct typenm_ typenm_;\

/* Handy shorthand for the type of the queue element */
#define __spscqetyp(q)      typeof((q)->elem[0])

/*
 * Initialize a dynamic queue.
 */

#define SPSCQ_DYN_INIT(q_, sz_) \
                            do {                        \
                                __spscq_init(&(q_)->q, sz_);\
                                (q_)->elem = NEWZA(__spscqetyp(q_), sz_);\
                            } while (0)


/*
 * Finalize a dynamic queue.
 */
#define SPSCQ_DYN_FINI(q_)            \
                            do {    \
                                __spscq * _q = &(q_)->q;  \
                                _q->sz = 0;\
                                DEL((q_)->elem); \
                                (q_)->elem = 0;\
                            } while (0)



/* Initialize queue 'q_' for size 'sz_' */
#define SPSCQ_INIT(q_,sz_)                                \
                            do {                        \
                                __spscq_init(&(q_)->q, sz_);\
                            } while (0)




/*
 * Initialize a queue.
 */
static inline __spscq*
__spscq_init(__spscq* q, size_t n)
{
    atomic_init(&q->rd, 0);
    atomic_init(&q->wr, 0);
    q->sz = n;
    return q;
}


/* Return true if queue is full */
static inline int
__spscq_full_p(__spscq * q)
{
    uint_fast32_t wr = 1 + atomic_load_explicit(&q->wr, memory_order_acquire);

    if (wr == q->sz) wr = 0;

    return wr == atomic_load_explicit(&q->rd, memory_order_acquire);
}

/* Return true if queue is empty */
static inline int
__spscq_empty_p(__spscq * q)
{
    uint_fast32_t rd = atomic_load_explicit(&q->rd, memory_order_acquire);

    return rd == atomic_load_explicit(&q->wr, memory_order_acquire);
}

/*
 * Return best guess for size of queue. This function cannot
 * guarantee accurate results in the presence of multiple concurrent
 * readers and writers.
 */
static inline uint32_t
__spscq_size(__spscq* q)
{
    uint_fast32_t n  = 0;
    uint_fast32_t rd = atomic_load_explicit(&q->rd, memory_order_acquire);
    uint_fast32_t wr = atomic_load_explicit(&q->wr, memory_order_acquire);

    if (rd < wr)
        n = wr - rd;
    else if (rd > wr)
        n = q->sz - rd + wr;

    return n;
}


/*
 * Enqueue element 'e_'  into the queue.
 * Return True on success, False if Q is full.
 */
#define SPSCQ_ENQ(q_, e_)  ({ \
                    __spscq* _q = &(q_)->q; \
                    int _z = 0;             \
                    uint_fast32_t wr_  = atomic_load_explicit(&_q->wr, memory_order_relaxed);\
                    uint_fast32_t nwr_ = wr_ + 1;\
                    if (nwr_ == _q->sz) nwr_ = 0; \
                    if (nwr_ != atomic_load_explicit(&_q->rd, memory_order_acquire)) { \
                        _z = 1;               \
                        (q_)->elem[wr_] = e_; \
                        atomic_store_explicit(&_q->wr, nwr_, memory_order_release); \
                    } \
                    _z; })


/*
 * Dequeue a element and put it in 'e_'.
 * Return True on success, False if Q empty.
 */
#define SPSCQ_DEQ(q_, e_)   ({                 \
                    __spscq* _q = &(q_)->q;    \
                    int _z = 0;                \
                    uint_fast32_t rd_ = atomic_load_explicit(&_q->rd, memory_order_relaxed);\
                    if (rd_ != atomic_load_explicit(&_q->wr, memory_order_acquire)) {       \
                        _z = 1;                     \
                        e_ = (q_)->elem[rd_++];       \
                        if (rd_ == _q->sz) rd_ = 0;\
                        atomic_store_explicit(&_q->rd, rd_, memory_order_release); \
                    } \
                    _z;})


/* Reset Queue to empty */
#define SPSCQ_RESET(q_)                                  \
                            do {                         \
                                __spscq * _q = &(q_)->q; \
                                atomic_init(&_q->rd, 0); \
                                atomic_init(&_q->wr, 0); \
                            } while (0)


/* Predicates that answer true/false */
#define SPSCQ_FULL_P(q_)       __spscq_full_p(&(q_)->q)
#define SPSCQ_EMPTY_P(q_)      __spscq_empty_p(&(q_)->q)


/* Number of elements in the queue */
#define SPSCQ_SIZE(q_)         __spscq_size(&(q_)->q)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __SPSC_BOUNDED_QUEUE_31741145_H__ */

/* EOF */
