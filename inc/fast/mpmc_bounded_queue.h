/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * mpmc_bounded_queue.h - a generic MPMC Bounded Queue
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
 *
 * Introduction
 * ============
 * This is a bounded, MPMC Generic Queue; inspired by
 * https://github.com/rigtorp/MPMCQueue.
 *
 * C macros are used to create and manipulate type-safe
 * queues. These are lock-free (not wait-free) and are
 * fixed-size, multi-producer, multi-consumer queues.
 *
 * The only dependency is a C11 compliant compiler & stdlib.
 *
 *
 * Usage:
 *   - Let's say you want to instantiate a fixed-size queue to hold
 *     'struct req' objects. We start by defining a type for 
 *     "a queue of 'struct req'" objects. We will name this queue 
 *     "req_q". 
 *
 *     MPMC_QUEUE_TYPE(req_q, struct req, REQ_Q_SIZE);
 *
 *  - Now, we can declare a queue of type 'req_q':
 *
 *          req_q Q;
 *
 *  - Enqueue: MPMC_QUEUE_ENQ_WAIT(&Q, *req)
 *  - Enqueue without blocking:
 *
 *          if (!MPMC_QUEUE_ENQ(&Q, *req) {
 *              panic("queue full");
 *          }
 *
 *  - Dequeue: MPMC_QUEUE_DEQ_WAIT(&Q, *req)
 *  - Dequeue without blocking:
 *
 *          if (!MPMC_QUEUE_DEQ(&Q, *req) {
 *              panic("queue empty");
 *          }
 */

#ifndef ___MPMC2_H__vCbg0fx5IeW9lDA6___
#define ___MPMC2_H__vCbg0fx5IeW9lDA6___ 1

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

#define __cachepad(n)   ((CACHELINE_SIZE - (n))/sizeof(uint64_t))

#ifdef __cplusplus
#ifndef typeof
#define typeof(a)   __typeof__(a)
#endif
#endif

struct __mpmc_q {
    atomic_uint_fast64_t head;
    uint64_t __pad0[__cachepad(8)];

    atomic_uint_fast64_t tail;
    uint64_t __pad1[__cachepad(8)];

    uint64_t sz;
    uint64_t mask;
    uint64_t __pad2[__cachepad(16)];
};
typedef struct __mpmc_q __mpmc_q;

static inline void
__mpmc_q_init(__mpmc_q * q, size_t sz) {
    assert((sz & (sz-1)) == 0);

    q->sz   = (uint64_t)sz;
    q->mask = q->sz - 1;
}

static inline void
__mpmc_q_reset(__mpmc_q *q) {
    atomic_store(&q->head, 0);
    atomic_store(&q->tail, 0);
}

static inline void
__mpmc_q_fini(__mpmc_q * q) {
    memset(q, 0, sizeof *q);
}

static inline size_t
__mpmc_q_len(__mpmc_q *q) {
    uint64_t hd = atomic_load_explicit(&q->head, memory_order_relaxed),
             tl = atomic_load_explicit(&q->tail, memory_order_relaxed);

    if (hd < tl) {
        // wraparound
        uint64_t diff = ~((uint64_t)0) - tl;
        return diff + hd;
    }
    return (size_t)(hd - tl);
}

// best effort - since we have concurrent readers/writers
static inline int
__mpmc_q_full_p(__mpmc_q *q) {
    return __mpmc_q_len(q) == q->sz;
}

// best effort - since we have concurrent readers/writers
static inline int
__mpmc_q_empty_p(__mpmc_q *q) {
    return __mpmc_q_len(q) == 0;
}


// Construct a name for the corresponding slot - but tie it to the
// queue typename (ty_). We need this because in C, nested struct 
// decl have no implied nested scope - it's as if the inner
// struct was actually declared outside. So, to avoid type
// collision, we construct distinct slot-types for each queue type.
#define _MPMC_SLOT_TYPNM(ty_) ty_ ## __slot

// declare a type for the individual cache-aligned slots
#define _MPMC_SLOT_TYPE(nm_, ty_)                                   \
            struct __CACHELINE_ALIGNED _MPMC_SLOT_TYPNM(nm_) {      \
                atomic_uint_fast64_t turn __CACHELINE_ALIGNED;      \
                ty_ data;                                           \
          };                                                        \
          typedef struct __CACHELINE_ALIGNED _MPMC_SLOT_TYPNM(nm_) _MPMC_SLOT_TYPNM(nm_)


#define MPMC_QUEUE_TYPE(nm_, ty_)                                   \
                    _MPMC_SLOT_TYPE(nm_, ty_);                      \
                    struct nm_ {                                    \
                        __mpmc_q q;                                 \
                        _MPMC_SLOT_TYPNM(nm_)  *slot;               \
                    };                                              \
                    typedef struct nm_ nm_


// handy helpers
#define __mpmc_slot_ty(q_)    typeof((q_)->slot[0])
#define __mpmc_q_turn(q_, v_) ((v_) / (q_)->q.sz)
#define __mpmc_q_idx(q_, i_)  ((i_) & (q_)->q.mask)

// allocate a cacheline aligned block of memory.
static void *
__alloc(size_t n)
{
    void *ptr = 0;
    int r = posix_memalign(&ptr, CACHELINE_SIZE, n);
    assert(r == 0);
    assert(ptr);

    return memset(ptr, 0, n);
}

// create a new queue of type 'ty' and size 'sz'
#define MPMC_QUEUE_NEW(ty_, sz_)    ({                              \
        uint64_t _n = NEXTPOW2(sz_);                                \
        ty_ * _q = 0;                                               \
        _q = __alloc(sizeof *_q);                                   \
                                                                    \
        _q->slot = __alloc(_n  * sizeof *_q->slot);                 \
        __mpmc_q_init(&_q->q, _n);                                  \
        _q;                                                         \
        })


// delete the queue 'q_' and its contents.
// For catching use-after-free bugs, we fill the free'd memory with
// a pattern.
#define MPMC_QUEUE_DEL(q_) do {                                     \
        typeof(q_) _qc = (q_);                                      \
        memset(_qc->slot, 0xaa,  _qc->q.sz * sizeof _qc->slot);     \
        free(_qc->slot);                                            \
        memset(_qc, 0x55, sizeof *_qc);                             \
        free(_qc);                                                  \
        (q_) = 0;                                                   \
} while (0)


// Reset the queue; Note - you should NOT call this from a
// multi-threaded context.
#define MPMC_QUEUE_RESET(q_)    do {                                \
        typeof(q_) _qc = (q_);                                      \
        __mpmc_slot_ty(qc) *_p   = &qc->slot[0],                    \
                           *_end = p + qc->q.sz;                    \
                                                                    \
        for (; _p < _end; _p++) {                                   \
            atomic_store(&_p->turn, 0);                             \
            memset(_p->data, 0, sizeof _p->data);                   \
        }                                                           \
        __mpmc_q_reset(&_qc->q);                                    \
} while (0)


// Enqueue 'e' in q; return true if successful, false otherwise
// This function is NOT wait-free, but it is lock-free
#define MPMC_QUEUE_ENQ(q_, e_) ({                                                           \
        int _ret = 0;                                                                       \
        typeof(q_) _qc = (q_);                                                              \
        __mpmc_q *_q   = &_qc->q;                                                           \
        uint64_t _hd   = atomic_load_explicit(&_q->head, memory_order_acquire);             \
                                                                                            \
        for(;;) {                                                                           \
            __mpmc_slot_ty(_qc) *_slot = &_qc->slot[__mpmc_q_idx(_qc, _hd)];                \
            uint64_t _turn = __mpmc_q_turn(_qc, _hd) * 2;                                   \
                                                                                            \
            if (atomic_load_explicit(&_slot->turn, memory_order_acquire) == _turn) {        \
                if (atomic_compare_exchange_strong(&_q->head, &_hd, _hd+1)) {               \
                        _slot->data = e_;                                                   \
                        atomic_store_explicit(&_slot->turn, _turn+1, memory_order_release); \
                        _ret = 1;                                                           \
                        break;                                                              \
                }                                                                           \
            } else {                                                                        \
                uint64_t _prev = _hd;                                                       \
                _hd = atomic_load_explicit(&_q->head, memory_order_acquire);                \
                if (_prev == _hd)  break;                                                   \
            }                                                                               \
        }                                                                                   \
        _ret;                                                                               \
})


// Dequeue the next element from the queue and return it in 'e_'.
// Return true if successful, false otherwise
#define MPMC_QUEUE_DEQ(q_, e_)  ({                                                          \
        int _ret = 0;                                                                       \
        typeof(q_) _qc = (q_);                                                              \
        __mpmc_q *_q   = &_qc->q;                                                           \
        uint64_t _tl   = atomic_load_explicit(&_q->tail, memory_order_acquire);             \
                                                                                            \
        for(;;) {                                                                           \
            __mpmc_slot_ty(_qc) *_slot = &_qc->slot[__mpmc_q_idx(_qc, _tl)];                \
            uint64_t _turn = 1 + (__mpmc_q_turn(_qc, _tl) * 2);                             \
                                                                                            \
            if (atomic_load_explicit(&_slot->turn, memory_order_acquire) == _turn) {        \
                if (atomic_compare_exchange_strong(&_q->tail, &_tl, _tl+1)) {               \
                        _ret = 1;                                                           \
                        e_ = _slot->data;                                                   \
                        atomic_store_explicit(&_slot->turn, _turn+1, memory_order_release); \
                        break;                                                              \
                }                                                                           \
            } else {                                                                        \
                uint64_t _prev = _tl;                                                       \
                _tl = atomic_load_explicit(&_q->tail, memory_order_acquire);                \
                if (_prev == _tl)  break;                                                   \
            }                                                                               \
        }                                                                                   \
        _ret;                                                                               \
})


// Block until 'q_' becomes non-full and then enqueue 'e_'
// NB: atomic_fetch_add() returns the previous value.
#define MPMC_QUEUE_ENQ_WAIT(q_, e_) do {                                                    \
        typeof(q_) _qc = (q_);                                                              \
        __mpmc_q *_q   = &_qc->q;                                                           \
        uint64_t _hd   = atomic_fetch_add(&_q->head, 1);                                    \
        __mpmc_slot_ty(_qc) *_slot = &_qc->slot[__mpmc_q_idx(_qc, _hd)];                    \
        uint64_t _turn = __mpmc_q_turn(_qc, _hd) * 2;                                       \
                                                                                            \
        while (_turn != atomic_load_explicit(&_slot->turn, memory_order_acquire)) { }       \
                                                                                            \
        _slot->data = e_;                                                                   \
        atomic_store_explicit(&_slot->turn, _turn+1, memory_order_release);                 \
} while(0)
            

// Block until queue is not-empty and dequeue next element into 'e_'
#define MPMC_QUEUE_DEQ_WAIT(q_, e_) do {                                                    \
        typeof(q_) _qc = (q_);                                                              \
        __mpmc_q *_q   = &_qc->q;                                                           \
        uint64_t _tl   = atomic_fetch_add(&_q->tail, 1);                                    \
        __mpmc_slot_ty(_qc) *_slot = &_qc->slot[__mpmc_q_idx(_qc, _tl)];                    \
        uint64_t _turn = 1 + (__mpmc_q_turn(_qc, _tl) * 2);                                 \
                                                                                            \
        while (_turn != atomic_load_explicit(&_slot->turn, memory_order_acquire)) { }       \
                                                                                            \
        e_ = _slot->data;                                                                   \
        atomic_store_explicit(&_slot->turn, _turn+1, memory_order_release);                 \
} while(0)


// Describe the queue metadata into string 'str_' whose size is
// 'strsz_'.
#define MPMC_QUEUE_DESC(q_, str_, strsz_)  do {                                             \
                typeof(q_) _qc = (q_);                                                      \
                __mpmc_q *_q   = &_qc->q;                                                   \
                char * _s      = (str_);                                                    \
                size_t _sz     = (strsz_);                                                  \
                __mpmc_slot_ty(_qc) *_sl = &_qc->slot[0];                                   \
                size_t  _dsz   = sizeof(_sl->data);                                         \
                size_t  _slsz  = sizeof(*_sl);                                              \
                snprintf(_s, _sz, "cap %zd, slotsz %zd datum %zd",                          \
                        _q->sz, _slsz, _dsz);                                               \
} while (0)


// Return the current occupied size of the queue. Note that this is
// best-effort when we have concurrent readers/writers.
#define MPMC_QUEUE_SIZE(q_)     ((size_t)__mpmc_q_len(&(q_)->q))

// Return the capacity of the mpmc queue
#define MPMC_QUEUE_CAP(q_)      ((size_t)((q_)->sz))

// Return true if the queue is full, false otherwise. Note that this
// is best effort when we have concurrent readers/writers.
#define MPMC_QUEUE_FULL_P(q_)   __mpmc_q_full_p(&(q_)->q)

// Return true if the queue is empty, false otherwise. Note that
// this is best effort when we have concurrent readers/writers.
#define MPMC_QUEUE_EMPTY_P(q_)  __mpmc_q_empty_p(&(q_)->q)



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___MPMC2_H__vCbg0fx5IeW9lDA6___ */

/* EOF */
