/*
 *  RTE RING
 *
 *  DERIVED FROM INTEL DPDK
 *  DERIVED FROM FREEBSD BUFRING
 *  Converted to use C11 stdatomic.h - Sudhi Herle
 *
 *  BSD LICENSE
 *
 *  Copyright(c) 2010-2013 Intel Corporation. All rights reserved.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *    * Neither the name of Intel Corporation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Derived from FreeBSD's bufring.h
 *
 **************************************************************************
 *
 * Copyright (c) 2007-2009 Kip Macy kmacy@freebsd.org
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. The name of Kip Macy nor the names of other
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ***************************************************************************/
#ifndef __RTE_RING_x5Vy7GJBmkFRcl9L__
#define __RTE_RING_x5Vy7GJBmkFRcl9L__ 1

#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <errno.h>


/**
 * RTE Ring
 *
 * The Ring Manager is a fixed-size queue, implemented as a table of
 * pointers. Head and tail pointers are modified atomically, allowing
 * concurrent access to it. It has the following features:
 *
 * - FIFO (First In First Out)
 * - Maximum size is fixed; the pointers are stored in a table.
 * - Lockfree (not wait free)
 * - Multi- or single-consumer dequeue.
 * - Multi- or single-producer enqueue.
 * - Bulk enqueue/dequeue  (all of N objects)
 * - Burst enqueue/dequeue (best effort: as many of N objects)
 * - Header-only; doesn't need any other support file
 * - Uses C11 atomics
 *
 * A Ring can be designated as SP or SC during creation time
 * (RING_FP_xx flags). However, callers if they are aware of their
 * usage can call the appopriate SP/MP or SC/MC functions directly.
 */


#ifdef __cplusplus
extern "C" {
#endif


#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

#ifndef likely
#define likely(expr)    __builtin_expect((expr), !0)
#endif
#ifndef unlikely
#define unlikely(expr)  __builtin_expect((expr), 0)
#endif

#ifdef __GNUC__
#define __CACHELINE_ALIGNED __attribute__((aligned(CACHE_LINE_SIZE)))
#elif defined(_MSC_VER)
#define __CACHELINE_ALIGNED __declspec(align(CACHE_LINE_SIZE))
#else
#error "Can't define __CACHELINE_ALIGNED for your compiler/machine"
#endif /* __CACHELINE_ALIGNED */

/*
 * Builtin gcc/clang intrinsics for pausing CPU execution for a
 * short while; it is used in tight loops while waiting for a shared
 * variable to update/
 */
#include <emmintrin.h>
#define rte_pause() _mm_pause()


enum rte_ring_queue_behavior {
    RTE_RING_QUEUE_FIXED = 0, /* Enq/Deq a fixed number of items from a ring */
    RTE_RING_QUEUE_VARIABLE   /* Enq/Deq as many items a possible from ring */
};


/**
 * An RTE ring structure.
 *
 * The producer and the consumer have a head and a tail index. The particularity
 * of these index is that they are not between 0 and size(ring). These indexes
 * are between 0 and 2^32, and we mask their value when we access the ring[]
 * field. Thanks to this assumption, we can do subtractions between 2 index
 * values in a modulo-32bit base: that's why the overflow of the indexes is not
 * a problem.
 */
struct __rte_index {
    atomic_uint_fast32_t head;
    atomic_uint_fast32_t tail;
};

struct __CACHELINE_ALIGNED rte_ring {
    struct __CACHELINE_ALIGNED {
        uint32_t size;           /**< Size of the ring. */
        uint32_t mask;           /**< Mask (size-1) of ring. */
        uint16_t sp_enq;         /**< True if single producer */
        uint16_t sc_deq;         /**< True if single consumer */
    };

    /** Ring producer & consumer status. */
    struct __rte_index prod __CACHELINE_ALIGNED;
    struct __rte_index cons __CACHELINE_ALIGNED;

    void * volatile ring[0] __CACHELINE_ALIGNED;
};

#define RING_F_SP_ENQ 0x0001 /**< The default enqueue is "single-producer". */
#define RING_F_SC_DEQ 0x0002 /**< The default dequeue is "single-consumer". */


/**
 * Create a new ring named *name* in memory.
 *
 * This function uses ``memzone_reserve()`` to allocate memory. Its size is
 * set to *count*, which must be a power of two. Water marking is
 * disabled by default.
 * Note that the real usable ring size is *count-1* instead of
 * *count*.
 *
 * @param count
 *   The size of the ring (must be a power of 2).
 * @param flags
 *   An OR of the following:
 *    - RING_F_SP_ENQ: If this flag is set, the default behavior when
 *      using ``rte_ring_enqueue()`` or ``rte_ring_enqueue_bulk()``
 *      is "single-producer". Otherwise, it is "multi-producers".
 *    - RING_F_SC_DEQ: If this flag is set, the default behavior when
 *      using ``rte_ring_dequeue()`` or ``rte_ring_dequeue_bulk()``
 *      is "single-consumer". Otherwise, it is "multi-consumers".
 * @return
 *   On success, the pointer to the new allocated ring. NULL on error with
 */
static inline struct rte_ring *
rte_ring_create(unsigned count, unsigned flags)
{
    size_t sz  = 0;
    struct rte_ring *r = 0;

    if (count & (count-1)) count = NEXTPOW2(count);

    sz = sizeof(*r) + (count * sizeof(void *));
    sz = _ALIGN_UP(sz, CACHE_LINE_SIZE);

    r  = (struct rte_ring *)NEWZA(uint8_t, sz);

    r->size   = count;
    r->mask   = count-1;
    r->sp_enq = (flags & RING_F_SP_ENQ) > 0;
    r->sc_deq = (flags & RING_F_SC_DEQ) > 0;

    r->prod.head  = r->prod.tail = 0;
    r->cons.head  = r->cons.tail = 0;

    return r;
}


/**
 * Destroy a created ring.
 */
static inline void
rte_ring_destroy(struct rte_ring *r)
{
    DEL(r);
}


/*
 * Unrolled store data to ring
 */
static inline void
__store_ring(struct rte_ring *r, uint_fast32_t head, void * const* obj, unsigned n)
{
    unsigned i;
    unsigned loops = n & (~(unsigned) 0x03U);
    unsigned idx   = head & r->mask;


    // If we know we won't wrap around, we unroll 4 times
    if (likely((idx + n) <= r->mask)) {
        for (i = 0; i < loops; i += 4, idx += 4) {
                r->ring[idx+0] = obj[i+0];
                r->ring[idx+1] = obj[i+1];
                r->ring[idx+2] = obj[i+2];
                r->ring[idx+3] = obj[i+3];
        }

        // mop up remainder
        switch (n & 0x3) {
            case 3:
                r->ring[idx+0] = obj[i+0];
                r->ring[idx+1] = obj[i+1];
                r->ring[idx+2] = obj[i+2];
                break;

            case 2:
                r->ring[idx+0] = obj[i+0];
                r->ring[idx+1] = obj[i+1];
                break;

            case 1:
                r->ring[idx+0] = obj[i+0];
                break;
        }
    } else {
        const uint32_t mask = r->mask;

        for (i = 0; i < n; i++, idx++) {
            r->ring[idx & mask] = obj[i];
        }
    }
}


/*
 * Unrolled load data from ring
 */
static inline void
__load_ring(struct rte_ring *r, uint_fast32_t head, void **obj, unsigned n)
{
    unsigned i;
    unsigned loops = n & (~(unsigned) 0x03U);
    unsigned idx   = head & r->mask;


    // If we know we won't wrap around, we unroll 4 times
    if (likely((idx + n) <= r->mask)) {
        for (i = 0; i < loops; i += 4, idx += 4) {
                obj[i+0] = r->ring[idx+0];
                obj[i+1] = r->ring[idx+1];
                obj[i+2] = r->ring[idx+2];
                obj[i+3] = r->ring[idx+3];
        }

        // mop up remainder
        switch (n & 0x3) {
            case 3:
                obj[i+0] = r->ring[idx+0];
                obj[i+1] = r->ring[idx+1];
                obj[i+2] = r->ring[idx+2];
                break;

            case 2:
                obj[i+0] = r->ring[idx+0];
                obj[i+1] = r->ring[idx+1];
                break;

            case 1:
                obj[i+0] = r->ring[idx+0];
                break;
        }
    } else {
        const uint32_t mask = r->mask;
        for (i = 0; i < n; i++, idx++) {
            obj[i+0] = r->ring[idx & mask];
        }
    }
}


/**
 * @internal Enqueue several objects on the ring (multi-producers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * producer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param behavior
 *   RTE_RING_QUEUE_FIXED:    Enqueue a fixed number of items from a ring
 *   RTE_RING_QUEUE_VARIABLE: Enqueue as many items a possible from ring
 * @return
 *   Depend on the behavior value
 *   if behavior = RTE_RING_QUEUE_FIXED
 *   - 0: Success; objects enqueue.
 *   - -EDQUOT: Quota exceeded. The objects have been enqueued, but the
 *     high water mark is exceeded.
 *   - -ENOBUFS: Not enough room in the ring to enqueue, no object is enqueued.
 *   if behavior = RTE_RING_QUEUE_VARIABLE
 *   - n: Actual number of objects enqueued.
 */
static inline int
__rte_ring_mp_do_enqueue(struct rte_ring *r, void * const *obj_table,
             unsigned n, enum rte_ring_queue_behavior behavior)
{
    const unsigned max  = n;
    const uint32_t mask = r->mask;

    uint_fast32_t prod_head, prod_next;
    uint_fast32_t cons_tail, free_entries;
    int success;

    /* move prod.head atomically */
    do {
        /* Reset n to the initial burst count */
        n = max;

        prod_head = atomic_load_explicit(&r->prod.head, memory_order_acquire);
        cons_tail = atomic_load_explicit(&r->cons.tail, memory_order_acquire);

        /* The subtraction is done between two unsigned 32bits value
         * (the result is always modulo 32 bits even if we have
         * prod_head > cons_tail). So 'free_entries' is always between 0
         * and size(ring)-1. */
        free_entries = (mask + cons_tail - prod_head);

        /* check that we have enough room in ring */
        if (unlikely(n > free_entries)) {
            if (behavior == RTE_RING_QUEUE_FIXED) return -ENOBUFS;

            /* No free entry available */
            if (unlikely(free_entries == 0)) return 0;

            n = free_entries;
        }

        prod_next = prod_head + n;
        success   = atomic_compare_exchange_weak_explicit(&r->prod.head, &prod_head,
                             prod_next,
                             memory_order_acquire, memory_order_relaxed);
    } while (unlikely(success == 0));

    /* write entries in ring */
    __store_ring(r, prod_head, obj_table, n);

    /*
     * If there are other enqueues in progress that preceeded us,
     * we need to wait for them to complete
     */
    while (unlikely(r->prod.tail != prod_head))
        rte_pause();

	atomic_store_explicit(&r->prod.tail, prod_next, memory_order_release);

    return n;
}

/**
 * @internal Enqueue several objects on a ring (NOT multi-producers safe).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param behavior
 *   RTE_RING_QUEUE_FIXED:    Enqueue a fixed number of items from a ring
 *   RTE_RING_QUEUE_VARIABLE: Enqueue as many items a possible from ring
 * @return
 *   Depend on the behavior value
 *   if behavior = RTE_RING_QUEUE_FIXED
 *   - 0: Success; objects enqueue.
 *   - -EDQUOT: Quota exceeded. The objects have been enqueued, but the
 *     high water mark is exceeded.
 *   - -ENOBUFS: Not enough room in the ring to enqueue, no object is enqueued.
 *   if behavior = RTE_RING_QUEUE_VARIABLE
 *   - n: Actual number of objects enqueued.
 */
static inline int
__rte_ring_sp_do_enqueue(struct rte_ring *r, void * const *obj_table,
             unsigned n, enum rte_ring_queue_behavior behavior)
{
    const uint32_t mask = r->mask;
    uint_fast32_t prod_head, cons_tail;
    uint_fast32_t prod_next, free_entries;

    prod_head = atomic_load_explicit(&r->prod.head, memory_order_acquire);
    cons_tail = atomic_load_explicit(&r->cons.tail, memory_order_acquire);

    /* The subtraction is done between two unsigned 32bits value
     * (the result is always modulo 32 bits even if we have
     * prod_head > cons_tail). So 'free_entries' is always between 0
     * and size(ring)-1. */
    free_entries = mask + cons_tail - prod_head;

    /* check that we have enough room in ring */
    if (unlikely(n > free_entries)) {
        if (behavior == RTE_RING_QUEUE_FIXED) return -ENOBUFS;

        /* No free entry available */
        if (unlikely(free_entries == 0)) return 0;

        n = free_entries;
    }

    prod_next = prod_head + n;
    atomic_store_explicit(&r->prod.head, prod_next, memory_order_release);

    /* write entries in ring */
    __store_ring(r, prod_head, obj_table, n);

    assert(atomic_load(&r->prod.tail) == prod_head);
    atomic_store_explicit(&r->prod.tail, prod_next, memory_order_release);

    return n;
}

/**
 * @internal Dequeue several objects from a ring (multi-consumers safe). When
 * the request objects are more than the available objects, only dequeue the
 * actual number of objects
 *
 * This function uses a "compare and set" instruction to move the
 * consumer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects) that will be filled.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @param behavior
 *   RTE_RING_QUEUE_FIXED:    Dequeue a fixed number of items from a ring
 *   RTE_RING_QUEUE_VARIABLE: Dequeue as many items a possible from ring
 * @return
 *  > 0: number of entries actually dequeued; in case of FIXED
 *  queue, it is exactly n; in case of VARIABLE queue, it will be
 *  less than n.
 *  == 0: for VARIABLE queue - no free slots available
 *  < 0:  for FIXED queue - no free slots available
 */
static inline int
__rte_ring_mc_do_dequeue(struct rte_ring *r, void **obj_table,
         unsigned n, enum rte_ring_queue_behavior behavior)
{
    const unsigned max  = n;
    uint_fast32_t cons_head, prod_tail;
    uint_fast32_t cons_next, entries;
    int success;

    /* move cons.head atomically */
    do {
        /* Restore n as it may change every loop */
        n = max;

        cons_head = atomic_load_explicit(&r->cons.head, memory_order_acquire);
        prod_tail = atomic_load_explicit(&r->prod.tail, memory_order_acquire);

        /* The subtraction is done between two unsigned 32bits value
         * (the result is always modulo 32 bits even if we have
         * cons_head > prod_tail). So 'entries' is always between 0
         * and size(ring)-1. */
        entries = (prod_tail - cons_head);

        /* Set the actual entries for dequeue */
        if (unlikely(n > entries)) {
            if (behavior == RTE_RING_QUEUE_FIXED) return -ENOENT;
            if (unlikely(entries == 0)) return 0;
            n = entries;
        }

        cons_next = cons_head + n;
        success   = atomic_compare_exchange_weak_explicit(&r->cons.head, &cons_head,
                             cons_next,
                             memory_order_acquire, memory_order_relaxed);
    } while (likely(success == 0));


    __load_ring(r, cons_head, obj_table, n);

    /*
     * If there are other dequeues in progress that preceded us,
     * we need to wait for them to complete
     */
    while (unlikely(r->cons.tail != cons_head))
        rte_pause();

	atomic_store_explicit(&r->cons.tail, cons_next, memory_order_release);

    return n;
}

/**
 * @internal Dequeue several objects from a ring (NOT multi-consumers safe).
 * When the request objects are more than the available objects, only dequeue
 * the actual number of objects
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects) that will be filled.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @param behavior
 *   RTE_RING_QUEUE_FIXED:    Dequeue a fixed number of items from a ring
 *   RTE_RING_QUEUE_VARIABLE: Dequeue as many items a possible from ring
 * @return
 *   Depend on the behavior value
 *   if behavior = RTE_RING_QUEUE_FIXED
 *   - 0: Success; objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue; no object is
 *     dequeued.
 *   if behavior = RTE_RING_QUEUE_VARIABLE
 *   - n: Actual number of objects dequeued.
 */
static inline int
__rte_ring_sc_do_dequeue(struct rte_ring *r, void **obj_table,
         unsigned n, enum rte_ring_queue_behavior behavior)
{
    uint_fast32_t cons_head, prod_tail;
    uint_fast32_t cons_next, entries;

    cons_head = atomic_load_explicit(&r->cons.head, memory_order_acquire);
    prod_tail = atomic_load_explicit(&r->prod.tail, memory_order_acquire);

    /* The subtraction is done between two unsigned 32bits value
     * (the result is always modulo 32 bits even if we have
     * cons_head > prod_tail). So 'entries' is always between 0
     * and size(ring)-1. */
    entries = prod_tail - cons_head;

    if (unlikely(n > entries)) {
        if (behavior == RTE_RING_QUEUE_FIXED) return -ENOENT;
        if (unlikely(entries == 0)) return 0;
        n = entries;
    }

    cons_next = cons_head + n;
    atomic_store_explicit(&r->cons.head, cons_next, memory_order_release);

    __load_ring(r, cons_head, obj_table, n);

    assert(atomic_load(&r->cons.tail) == cons_head);
    atomic_store_explicit(&r->cons.tail, cons_next, memory_order_release);
    return n;
}

/**
 * Enqueue all objects on the ring (multi-producers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * producer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @return
 *   - 0: Success; objects enqueue.
 *   - -EDQUOT: Quota exceeded. The objects have been enqueued, but the
 *     high water mark is exceeded.
 *   - -ENOBUFS: Not enough room in the ring to enqueue, no object is enqueued.
 */
static inline int
rte_ring_mp_enqueue_bulk(struct rte_ring *r, void * const *obj_table,
             unsigned n)
{
    return __rte_ring_mp_do_enqueue(r, obj_table, n, RTE_RING_QUEUE_FIXED);
}

/**
 * Enqueue all objects on a ring (NOT multi-producers safe).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @return
 *   - 0: Success; objects enqueued.
 *   - -EDQUOT: Quota exceeded. The objects have been enqueued, but the
 *     high water mark is exceeded.
 *   - -ENOBUFS: Not enough room in the ring to enqueue; no object is enqueued.
 */
static inline int
rte_ring_sp_enqueue_bulk(struct rte_ring *r, void * const *obj_table,
             unsigned n)
{
    return __rte_ring_sp_do_enqueue(r, obj_table, n, RTE_RING_QUEUE_FIXED);
}

/**
 * Enqueue all objects on a ring.
 *
 * This function calls the multi-producer or the single-producer
 * version depending on the default behavior that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @return
 *   - 0: Success; objects enqueued.
 *   - -EDQUOT: Quota exceeded. The objects have been enqueued, but the
 *     high water mark is exceeded.
 *   - -ENOBUFS: Not enough room in the ring to enqueue; no object is enqueued.
 */
static inline int
rte_ring_enqueue_bulk(struct rte_ring *r, void * const *obj_table, unsigned n)
{
    if (r->sp_enq)
        return rte_ring_sp_enqueue_bulk(r, obj_table, n);
    else
        return rte_ring_mp_enqueue_bulk(r, obj_table, n);
}

/**
 * Enqueue one object on a ring (multi-producers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * producer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj
 *   A pointer to the object to be added.
 * @return
 *   - 0: Success; objects enqueued.
 *   - -EDQUOT: Quota exceeded. The objects have been enqueued, but the
 *     high water mark is exceeded.
 *   - -ENOBUFS: Not enough room in the ring to enqueue; no object is enqueued.
 */
static inline int
rte_ring_mp_enqueue(struct rte_ring *r, void *obj)
{
    return rte_ring_mp_enqueue_bulk(r, &obj, 1);
}

/**
 * Enqueue one object on a ring (NOT multi-producers safe).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj
 *   A pointer to the object to be added.
 * @return
 *   - 0: Success; objects enqueued.
 *   - -EDQUOT: Quota exceeded. The objects have been enqueued, but the
 *     high water mark is exceeded.
 *   - -ENOBUFS: Not enough room in the ring to enqueue; no object is enqueued.
 */
static inline int
rte_ring_sp_enqueue(struct rte_ring *r, void *obj)
{
    return rte_ring_sp_enqueue_bulk(r, &obj, 1);
}

/**
 * Enqueue one object on a ring.
 *
 * This function calls the multi-producer or the single-producer
 * version, depending on the default behaviour that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj
 *   A pointer to the object to be added.
 * @return
 *   - 0: Success; objects enqueued.
 *   - -EDQUOT: Quota exceeded. The objects have been enqueued, but the
 *     high water mark is exceeded.
 *   - -ENOBUFS: Not enough room in the ring to enqueue; no object is enqueued.
 */
static inline int
rte_ring_enqueue(struct rte_ring *r, void *obj)
{
    if (r->sp_enq)
        return rte_ring_sp_enqueue_bulk(r, &obj, 1);
    else
        return rte_ring_mp_enqueue_bulk(r, &obj, 1);
}

/**
 * Dequeue several objects from a ring (multi-consumers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * consumer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects) that will be filled.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @return
 *   - 0: Success; objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue; no object is
 *     dequeued.
 */
static inline int
rte_ring_mc_dequeue_bulk(struct rte_ring *r, void **obj_table, unsigned n)
{
    return __rte_ring_mc_do_dequeue(r, obj_table, n, RTE_RING_QUEUE_FIXED);
}

/**
 * Dequeue several objects from a ring (NOT multi-consumers safe).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects) that will be filled.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table,
 *   must be strictly positive.
 * @return
 *   - 0: Success; objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue; no object is
 *     dequeued.
 */
static inline int
rte_ring_sc_dequeue_bulk(struct rte_ring *r, void **obj_table, unsigned n)
{
    return __rte_ring_sc_do_dequeue(r, obj_table, n, RTE_RING_QUEUE_FIXED);
}

/**
 * Dequeue several objects from a ring.
 *
 * This function calls the multi-consumers or the single-consumer
 * version, depending on the default behaviour that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects) that will be filled.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @return
 *   - 0: Success; objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue, no object is
 *     dequeued.
 */
static inline int
rte_ring_dequeue_bulk(struct rte_ring *r, void **obj_table, unsigned n)
{
    if (r->sc_deq)
        return rte_ring_sc_dequeue_bulk(r, obj_table, n);
    else
        return rte_ring_mc_dequeue_bulk(r, obj_table, n);
}

/**
 * Dequeue one object from a ring (multi-consumers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * consumer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_p
 *   A pointer to a void * pointer (object) that will be filled.
 * @return
 *   - 0: Success; objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue; no object is
 *     dequeued.
 */
static inline int
rte_ring_mc_dequeue(struct rte_ring *r, void **obj_p)
{
    return rte_ring_mc_dequeue_bulk(r, obj_p, 1);
}

/**
 * Dequeue one object from a ring (NOT multi-consumers safe).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_p
 *   A pointer to a void * pointer (object) that will be filled.
 * @return
 *   - 0: Success; objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue, no object is
 *     dequeued.
 */
static inline int
rte_ring_sc_dequeue(struct rte_ring *r, void **obj_p)
{
    return rte_ring_sc_dequeue_bulk(r, obj_p, 1);
}

/**
 * Dequeue one object from a ring.
 *
 * This function calls the multi-consumers or the single-consumer
 * version depending on the default behaviour that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_p
 *   A pointer to a void * pointer (object) that will be filled.
 * @return
 *   - 0: Success, objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue, no object is
 *     dequeued.
 */
static inline int
rte_ring_dequeue(struct rte_ring *r, void **obj_p)
{
    if (r->sc_deq)
        return rte_ring_sc_dequeue_bulk(r, obj_p, 1);
    else
        return rte_ring_mc_dequeue_bulk(r, obj_p, 1);
}


/**
 * Enqueue as many objects on the ring (multi-producers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * producer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @return
 *   - n: Actual number of objects enqueued.
 */
static inline int
rte_ring_mp_enqueue_burst(struct rte_ring *r, void * const *obj_table,
             unsigned n)
{
    return __rte_ring_mp_do_enqueue(r, obj_table, n, RTE_RING_QUEUE_VARIABLE);
}

/**
 * Enqueue many several objects on a ring (NOT multi-producers safe).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @return
 *   - n: Actual number of objects enqueued.
 */
static inline int
rte_ring_sp_enqueue_burst(struct rte_ring *r, void * const *obj_table,
             unsigned n)
{
    return __rte_ring_sp_do_enqueue(r, obj_table, n, RTE_RING_QUEUE_VARIABLE);
}

/**
 * Enqueue several objects on a ring.
 *
 * This function calls the multi-producer or the single-producer
 * version depending on the default behavior that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @return
 *   - n: Actual number of objects enqueued.
 */
static inline int
rte_ring_enqueue_burst(struct rte_ring *r, void * const *obj_table,
              unsigned n)
{
    if (r->sp_enq)
        return    rte_ring_sp_enqueue_burst(r, obj_table, n);
    else
        return    rte_ring_mp_enqueue_burst(r, obj_table, n);
}

/**
 * Dequeue several objects from a ring (multi-consumers safe). When the request
 * objects are more than the available objects, only dequeue the actual number
 * of objects
 *
 * This function uses a "compare and set" instruction to move the
 * consumer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects) that will be filled.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @return
 *   - n: Actual number of objects dequeued, 0 if ring is empty
 */
static inline int
rte_ring_mc_dequeue_burst(struct rte_ring *r, void **obj_table, unsigned n)
{
    return __rte_ring_mc_do_dequeue(r, obj_table, n, RTE_RING_QUEUE_VARIABLE);
}

/**
 * Dequeue several objects from a ring (NOT multi-consumers safe).When the
 * request objects are more than the available objects, only dequeue the
 * actual number of objects
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects) that will be filled.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @return
 *   - n: Actual number of objects dequeued, 0 if ring is empty
 */
static inline int
rte_ring_sc_dequeue_burst(struct rte_ring *r, void **obj_table, unsigned n)
{
    return __rte_ring_sc_do_dequeue(r, obj_table, n, RTE_RING_QUEUE_VARIABLE);
}

/**
 * Dequeue multiple objects from a ring up to a maximum number.
 *
 * This function calls the multi-consumers or the single-consumer
 * version, depending on the default behaviour that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects) that will be filled.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @return
 *   - Number of objects dequeued, or a negative error code on error
 */
static inline int
rte_ring_dequeue_burst(struct rte_ring *r, void **obj_table, unsigned n)
{
    if (r->sc_deq)
        return rte_ring_sc_dequeue_burst(r, obj_table, n);
    else
        return rte_ring_mc_dequeue_burst(r, obj_table, n);
}



/**
 * Return the number of entries in a ring.
 *
 * @param r
 *   A pointer to the ring structure.
 * @return
 *   The number of entries in the ring.
 */
static inline unsigned
rte_ring_count(struct rte_ring *r)
{
    uint_fast32_t prod_tail = atomic_load_explicit(&r->prod.tail, memory_order_relaxed);
    uint_fast32_t cons_tail = atomic_load_explicit(&r->cons.tail, memory_order_relaxed);
    return ((prod_tail - cons_tail) & r->mask);
}

/**
 * Return the number of free entries in a ring.
 *
 * @param r
 *   A pointer to the ring structure.
 * @return
 *   The number of free entries in the ring.
 */
static inline unsigned
rte_ring_free_count(struct rte_ring *r)
{
    return r->mask - rte_ring_count(r);
}


/**
 * Test if a ring is full.
 *
 * @param r
 *   A pointer to the ring structure.
 * @return
 *   - 1: The ring is full.
 *   - 0: The ring is not full.
 */
static inline int
rte_ring_full(struct rte_ring *r)
{
    return rte_ring_free_count(r) == 0;
}

/**
 * Test if a ring is empty.
 *
 * @param r
 *   A pointer to the ring structure.
 * @return
 *   - 1: The ring is empty.
 *   - 0: The ring is not empty.
 */
static inline int
rte_ring_empty(struct rte_ring *r)
{
    return rte_ring_count(r) == 0;
}


static inline void
rte_ring_dump(char *buf, size_t n, struct rte_ring *r)
{
    ssize_t x;
    x = snprintf(buf, n, "ring <%p>: size=%u, used %u, avail %u\n"
                         "   cons.tail=%u, cons.head=%u\n"
                         "   prod.tail=%u, prod.head=%u\n",
                            r, r->size, rte_ring_count(r), rte_ring_free_count(r),
                            r->cons.tail, r->cons.head,
                            r->prod.tail, r->prod.tail
           );

    if (x < 0 || ((size_t)x) >= n) buf[n-1] = 0;
}


#ifdef __cplusplus
}
#endif

#endif /* __RTE_RING_x5Vy7GJBmkFRcl9L__ */
