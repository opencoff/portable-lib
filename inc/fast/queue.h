/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * queue.h - Fast queue services.
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
 *   queues.
 *
 *   Use:
 *     Make a type for your queue to hold objects of your
 *     interest. e.g.,
 *          FQ_TYPE(typename, type, Q_size);
 *     This makes a new Queue data type (typedef) called 'typename'
 *     to hold objects of type 'type'. The size of the circular
 *     Queue is 'size' elements.
 *     Next, declare one or more objects of type 'typename':
 *          typename my_queue;
 *
 *     Operations allowed:
 *       o FQ_INIT(my_queue, Q_size)
 *            MUST be called before any use of other macros. Q_size
 *            MUST be the same as the one used for FQ_TYPE().
 *       o FQ_ENQ(my_queue, elem)
 *            Enqueue 'elem' into 'my_queue'; return true on
 *            success, false otherwise.
 *       o FQ_DEQ(my_queue, elem)
 *            Dequeue an element from 'my_queue' and put the element
 *            into 'elem'. Return true on success, false otherwise.
 *
 *  The queue algorithm uses two read & write pointers.
 *  The read pointer tracks the last successful read.
 *  The write pointer tracks the last successful write.
 *  
 *  Thus, the conditions for queue full and queue empty are the
 *  same. This simplicification means one queue slot will always go
 *  unused.
 */

#ifndef __FAST_QUEUE_H__
#define __FAST_QUEUE_H__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include "utils/utils.h"

#ifdef __cplusplus
#ifndef typeof
#define typeof(a)   __typeof__(a)
#endif
#endif

struct __fast_queue
{
    uint32_t rd, /* Next slot from which to read */
             wr; /* Last successful write */

    uint32_t sz;    /* Capacity of the queue */
};
typedef struct __fast_queue __fast_queue;


/* Create a new queue called 'typnm' of a fixed size 'sz' to hold
 * elements of type * 'ty'. */
#define FQ_TYPE(typnm_,ty_,sz_) struct typnm_       \
                                 {                          \
                                     __fast_queue q;        \
                                     ty_ elem[(sz_)];       \
                                 }

#define FQ_TYPEDEF(typnm_,ty_,sz_) typedef FQ_TYPE(typnm_,ty_,sz_) typnm_


/*
 * Create a "dynamic" fast-queue whose size will be initialized
 * later. The actual memory will be allocated via malloc().
 */
#define FQ_DYN_TYPEDEF(typenm_,ty_) \
                                 struct typenm_       \
                                 {                    \
                                     __fast_queue q;  \
                                     ty_ *elem;       \
                                 };                   \
                                typedef struct typenm_ typenm_;\

/* Handy shorthand for the type of the queue element */
#define __qetyp(q)      typeof((q)->elem[0])

/* Initialize queue 'q_' for size 'sz_' */
#define FQ_INIT(q_,sz_)                                     \
                            do {                            \
                                __fast_queue * _q = &(q_)->q;  \
                                _q->rd = _q->wr = 0;    \
                                _q->sz = sz_;               \
                            } while (0)


/* Finalize a queue */
#define FQ_FINI(q0)     do {} while (0)


/*
 * Initialize a dynamic queue.
 */
#define FQ_DYN_INIT(q_, sz_) \
                            do {                            \
                                __fast_queue * _q = &(q_)->q;  \
                                _q->rd = _q->wr = 0;    \
                                _q->sz = sz_;               \
                                (q_)->elem = NEWZA(__qetype(q_), sz_);\
                            } while (0)


/*
 * Finalize a dynamic queue.
 */
#define FQ_DYN_FINI(q_)             \
                            do {    \
                                __fast_queue * _q = &(q_)->q;  \
                                _q->rd = _q->wr = 0; \
                                _q->sz = 0;\
                                DEL((q_)->elem); \
                            } while (0)



/* Return true if queue is full */
static inline int
__q_full_p(__fast_queue * q)
{
    uint32_t wr = q->wr + 1;
    if (wr == q->sz)
        wr = 0;

    return q->rd == wr;
}

/* Return true if queue is empty */
static inline int
__q_empty_p(__fast_queue * q)
{
    return q->rd == q->wr;
}

/* Return number of valid elements in the queue */
static inline uint32_t
__q_size(__fast_queue* q)
{
    uint32_t n = 0;

    if (q->rd < q->wr)
        n = q->wr - q->rd;
    else if (q->rd > q->wr)
        n = q->sz - q->rd + q->wr;

    return n;
}


/*
 * If queue is not full:
 *     Set 'p_wr' to next write-index and return True
 *
 * Return False if queue is full.
 */
static inline int
__next_wr(__fast_queue* q, uint32_t *p_wr)
{
    uint32_t wr  = q->wr;
    uint32_t nwr = wr + 1;

    if (nwr == q->sz)
        nwr = 0;

    if (nwr != q->rd) {
        *p_wr = wr;
        q->wr = nwr;
        return 1;
    }

    return 0;
}


/*
 * If queue is not empty:
 *     Set 'p_wr' to next read-index and return True
 *
 * Return False if queue is empty.
 */
static inline int
__next_rd(__fast_queue* q, uint32_t* p_rd)
{
    uint32_t rd  = q->rd;

    if (rd == q->wr)
        return 0;

    uint32_t nrd = rd + 1;
    if (nrd == q->sz)
        nrd = 0;

    *p_rd = rd;
    q->rd = nrd;
    return 1;
}


/*
 * Enqueue 'e' into the queue.
 * Return true if success, false if Q full.
 */
#define FQ_ENQ(q_, e_)  ({ \
                    __fast_queue* fq = &(q_)->q; \
                    int z_ = 1; \
                    uint32_t w_; \
                    if (__next_wr(fq, &w_))  \
                        (q_)->elem[w_] = e_; \
                    else \
                        z_ = 0; \
                    z_;})



/*
 * Dequeue an element and store in 'e'.
 * Return true on success, false if Q empty
 */
#define FQ_DEQ(q_, e_)  ({ \
                    __fast_queue* fq = &(q_)->q;    \
                    int z_ = 1;              \
                    uint32_t r_;             \
                    if (__next_rd(fq, &r_))  \
                        e_ = (q_)->elem[r_]; \
                    else                     \
                        z_ = 0;              \
                    z_;})


/* Reset Queue to empty */
#define FQ_RESET(q_)                                        \
                            do {                            \
                                __fast_queue * _q = &(q_)->q;  \
                                _q->rd = _q->wr = 0;\
                            } while (0)


/* Predicates that answer true/false */
#define FQ_FULL_P(q_)       __q_full_p(&(q_)->q)
#define FQ_EMPTY_P(q_)      __q_empty_p(&(q_)->q)


/* Number of elements in the queue */
#define FQ_SIZE(q_)         __q_size(&(q_)->q)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __FAST_QUEUE_H__ */

/* EOF */
