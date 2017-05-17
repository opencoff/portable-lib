/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * fast/ringbuf.h - Multi-producer, single consumer bounded
 * queue.
 *
 * Derived from FreeBSD's bufring.h
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
 * --------------------------------------------------------------------------
 * Derived from Intel DPDK and FreeBSD kernel.
 *
 *   Copyright(c) 2010-2013 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Portable C Macro edition (c) 2015 Sudhi Herle <sw at herle.net>
 *
 * If you need a commercial license for this work, please contact
 * the author.
 */

#ifndef ___FAST_RINGBUF_H_8587652_1440385495__
#define ___FAST_RINGBUF_H_8587652_1440385495__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdatomic.h>
#include <errno.h>

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

#ifndef likely
#define likely(expr)    __builtin_expect((expr), !0)
#endif
#ifndef unlikely
#define unlikely(expr)  __builtin_expect((expr), 0)
#endif


/*
 * Atomic intrinsics
 */
#define __atomic32_cmpset(d,e,s)    __sync_bool_compare_and_swap((volatile uint32_t*)(d), (uint32_t)(e), (uint32_t)(s))

#ifndef __arm__

#define __rmb()     __sync_synchronize()
#define __wmb()     __sync_synchronize()
#define __relax()   /* Nothing */

#elif defined(__x86_64__) || defined(__i386__)

#define __rmb()     asm volatile("lfence;" ::: "memory")
#define __wmb()     asm volatile("sfence;" ::: "memory")
#define __relax()   asm volatile("pause")

#else
#error "Don't know how to define rmb() and wmb()"
#endif


#else
#error "Can't define __CACHELINE_ALIGNED for your compiler/machine"
#endif /* __CACHELINE_ALIGNED */


/*
 * The Ring can support fixed # of enq/deq or variable# of enq/deq
 */
enum ringbuf_queue_behavior {
    RING_QUEUE_FIXED = 0, /* Enq/Deq a fixed number of items from a ring */
    RING_QUEUE_VARIABLE   /* Enq/Deq as many items a possible from ring */
};

/**
 * An RING ring structure.
 *
 * The producer and the consumer have a head and a tail index. The particularity
 * of these index is that they are not between 0 and size(ring). These indexes
 * are between 0 and 2^32, and we mask their value when we access the ring[]
 * field. Thanks to this assumption, we can do subtractions between 2 index
 * values in a modulo-32bit base: that's why the overflow of the indexes is not
 * a problem.
 */
struct ringbuf {
    uint64_t flags;                       /**< Flags supplied at creation. */

    /** Ring producer status. */
    struct prod {
        uint32_t watermark;      /**< Maximum items before EDQUOT. */
        uint32_t sp_enqueue;     /**< True, if single producer. */
        uint32_t size;           /**< Size of ring. */
        uint32_t mask;           /**< Mask (size-1) of ring. */
        volatile uint32_t head;  /**< Producer head. */
        volatile uint32_t tail;  /**< Producer tail. */
    } prod __CACHELINE_ALIGNED;

    /** Ring consumer status. */
    struct cons {
        uint32_t sc_dequeue;     /**< True, if single consumer. */
        uint32_t size;           /**< Size of the ring. */
        uint32_t mask;           /**< Mask (size-1) of ring. */
        volatile uint32_t head;  /**< Consumer head. */
        volatile uint32_t tail;  /**< Consumer tail. */
    } cons __CACHELINE_ALIGNED;


#ifdef RING_DEBUG
    struct ringbuf_debug_stats stats[RING_MAX_LCORE];
#endif

    void * volatile ring[1] \
            __CACHELINE_ALIGNED; /**< Memory space of ring starts here. */
};

#define RING_F_SP_ENQ       0x0001     /**< The default enqueue is "single-producer". */
#define RING_F_SC_DEQ       0x0002     /**< The default dequeue is "single-consumer". */
#define RING_QUOT_EXCEED    (1 << 31)  /**< Quota exceed for burst ops */
#define RING_SZ_MASK        (unsigned)(0x0fffffff) /**< Ring size mask */

/**
 * @internal When debug is enabled, store ring statistics.
 * @param r
 *   A pointer to the ring.
 * @param name
 *   The name of the statistics field to increment in the ring.
 * @param n
 *   The number to add to the object-oriented statistics.
 */
#ifdef RING_DEBUG
#define __RING_STAT_ADD(r, name, n) do {       \
        unsigned __lcore_id = rte_lcore_id(); \
        r->stats[__lcore_id].name##_objs += n;  \
        r->stats[__lcore_id].name##_bulk += 1;  \
    } while(0)
#else
#define __RING_STAT_ADD(r, name, n)
#endif

/**
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
 *      using ``ringbuf_enqueue()`` or ``ringbuf_enqueue_bulk()``
 *      is "single-producer". Otherwise, it is "multi-producers".
 *    - RING_F_SC_DEQ: If this flag is set, the default behavior when
 *      using ``ringbuf_dequeue()`` or ``ringbuf_dequeue_bulk()``
 *      is "single-consumer". Otherwise, it is "multi-consumers".
 * @return
 *   On success, the pointer to the new allocated ring. NULL on error with
 *    rte_errno set appropriately. Possible errno values include:
 *    - E_RING_NO_CONFIG - function could not get pointer to rte_config structure
 *    - E_RING_SECONDARY - function was called from a secondary process instance
 *    - E_RING_NO_TAILQ - no tailq list could be got for the ring list
 *    - EINVAL - count provided is not a power of 2
 *    - ENOSPC - the maximum number of memzones has already been allocated
 *    - EEXIST - a memzone with the same name already exists
 *    - ENOMEM - no appropriate memory area found in which to create memzone
 */
struct ringbuf *ringbuf_create(unsigned count, unsigned flags);

/**
 * Change the high water mark.
 *
 * If *count* is 0, water marking is disabled. Otherwise, it is set to the
 * *count* value. The *count* value must be greater than 0 and less
 * than the ring size.
 *
 * This function can be called at any time (not necessarily at
 * initialization).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param count
 *   The new water mark value.
 * @return
 *   - 0: Success; water mark changed.
 *   - -EINVAL: Invalid water mark value.
 */
int ringbuf_set_water_mark(struct ringbuf *r, unsigned count);

/**
 * Dump the status of the ring to the console.
 *
 * @param r
 *   A pointer to the ring structure.
 */
void ringbuf_dump(const struct ringbuf *r);

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
 *   RING_QUEUE_FIXED:    Enqueue a fixed number of items from a ring
 *   RING_QUEUE_VARIABLE: Enqueue as many items a possible from ring
 * @return
 *   Depend on the behavior value
 *   if behavior = RING_QUEUE_FIXED
 *   - 0: Success; objects enqueue.
 *   - -EDQUOT: Quota exceeded. The objects have been enqueued, but the
 *     high water mark is exceeded.
 *   - -ENOBUFS: Not enough room in the ring to enqueue, no object is enqueued.
 *   if behavior = RING_QUEUE_VARIABLE
 *   - n: Actual number of objects enqueued.
 */
static inline int
__ringbuf_mp_do_enqueue(struct ringbuf *r, void * const *obj_table,
             unsigned n, enum ringbuf_queue_behavior behavior)
{
    uint32_t prod_head, prod_next;
    uint32_t cons_tail, free_entries;
    const unsigned max = n;
    int success;
    unsigned i;
    uint32_t mask = r->prod.mask;
    int ret;

    /* move prod.head atomically */
    do {
        /* Reset n to the initial burst count */
        n = max;

        prod_head = r->prod.head;
        cons_tail = r->cons.tail;
        /* The subtraction is done between two unsigned 32bits value
         * (the result is always modulo 32 bits even if we have
         * prod_head > cons_tail). So 'free_entries' is always between 0
         * and size(ring)-1. */
        free_entries = (mask + cons_tail - prod_head);

        /* check that we have enough room in ring */
        if (unlikely(n > free_entries)) {
            if (behavior == RING_QUEUE_FIXED) {
                __RING_STAT_ADD(r, enq_fail, n);
                return -ENOBUFS;
            }
            else {
                /* No free entry available */
                if (unlikely(free_entries == 0)) {
                    __RING_STAT_ADD(r, enq_fail, n);
                    return 0;
                }

                n = free_entries;
            }
        }

        prod_next = prod_head + n;
        success = rte_atomic32_cmpset(&r->prod.head, prod_head,
                          prod_next);
    } while (unlikely(success == 0));

    /* write entries in ring */
    for (i = 0; likely(i < n); i++)
        r->ring[(prod_head + i) & mask] = obj_table[i];
    __wmb();

    /* if we exceed the watermark */
    if (unlikely(((mask + 1) - free_entries + n) > r->prod.watermark)) {
        ret = (behavior == RING_QUEUE_FIXED) ? -EDQUOT :
                (int)(n | RING_QUOT_EXCEED);
        __RING_STAT_ADD(r, enq_quota, n);
    }
    else {
        ret = (behavior == RING_QUEUE_FIXED) ? 0 : n;
        __RING_STAT_ADD(r, enq_success, n);
    }

    /*
     * If there are other enqueues in progress that preceeded us,
     * we need to wait for them to complete
     */
    while (unlikely(r->prod.tail != prod_head))
        __relax();

    r->prod.tail = prod_next;
    return ret;
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
 *   RING_QUEUE_FIXED:    Enqueue a fixed number of items from a ring
 *   RING_QUEUE_VARIABLE: Enqueue as many items a possible from ring
 * @return
 *   Depend on the behavior value
 *   if behavior = RING_QUEUE_FIXED
 *   - 0: Success; objects enqueue.
 *   - -EDQUOT: Quota exceeded. The objects have been enqueued, but the
 *     high water mark is exceeded.
 *   - -ENOBUFS: Not enough room in the ring to enqueue, no object is enqueued.
 *   if behavior = RING_QUEUE_VARIABLE
 *   - n: Actual number of objects enqueued.
 */
static inline int
__ringbuf_sp_do_enqueue(struct ringbuf *r, void * const *obj_table,
             unsigned n, enum ringbuf_queue_behavior behavior)
{
    uint32_t prod_head, cons_tail;
    uint32_t prod_next, free_entries;
    unsigned i;
    uint32_t mask = r->prod.mask;
    int ret;

    prod_head = r->prod.head;
    cons_tail = r->cons.tail;
    /* The subtraction is done between two unsigned 32bits value
     * (the result is always modulo 32 bits even if we have
     * prod_head > cons_tail). So 'free_entries' is always between 0
     * and size(ring)-1. */
    free_entries = mask + cons_tail - prod_head;

    /* check that we have enough room in ring */
    if (unlikely(n > free_entries)) {
        if (behavior == RING_QUEUE_FIXED) {
            __RING_STAT_ADD(r, enq_fail, n);
            return -ENOBUFS;
        }
        else {
            /* No free entry available */
            if (unlikely(free_entries == 0)) {
                __RING_STAT_ADD(r, enq_fail, n);
                return 0;
            }

            n = free_entries;
        }
    }

    prod_next = prod_head + n;
    r->prod.head = prod_next;

    /* write entries in ring */
    for (i = 0; likely(i < n); i++)
        r->ring[(prod_head + i) & mask] = obj_table[i];
    __wmb();

    /* if we exceed the watermark */
    if (unlikely(((mask + 1) - free_entries + n) > r->prod.watermark)) {
        ret = (behavior == RING_QUEUE_FIXED) ? -EDQUOT :
            (int)(n | RING_QUOT_EXCEED);
        __RING_STAT_ADD(r, enq_quota, n);
    }
    else {
        ret = (behavior == RING_QUEUE_FIXED) ? 0 : n;
        __RING_STAT_ADD(r, enq_success, n);
    }

    r->prod.tail = prod_next;
    return ret;
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
 *   RING_QUEUE_FIXED:    Dequeue a fixed number of items from a ring
 *   RING_QUEUE_VARIABLE: Dequeue as many items a possible from ring
 * @return
 *   Depend on the behavior value
 *   if behavior = RING_QUEUE_FIXED
 *   - 0: Success; objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue; no object is
 *     dequeued.
 *   if behavior = RING_QUEUE_VARIABLE
 *   - n: Actual number of objects dequeued.
 */

static inline int
__ringbuf_mc_do_dequeue(struct ringbuf *r, void **obj_table,
         unsigned n, enum ringbuf_queue_behavior behavior)
{
    uint32_t cons_head, prod_tail;
    uint32_t cons_next, entries;
    const unsigned max = n;
    int success;
    unsigned i;
    uint32_t mask = r->prod.mask;

    /* move cons.head atomically */
    do {
        /* Restore n as it may change every loop */
        n = max;

        cons_head = r->cons.head;
        prod_tail = r->prod.tail;
        /* The subtraction is done between two unsigned 32bits value
         * (the result is always modulo 32 bits even if we have
         * cons_head > prod_tail). So 'entries' is always between 0
         * and size(ring)-1. */
        entries = (prod_tail - cons_head);

        /* Set the actual entries for dequeue */
        if (unlikely(n > entries)) {
            if (behavior == RING_QUEUE_FIXED) {
                __RING_STAT_ADD(r, deq_fail, n);
                return -ENOENT;
            }
            else {
                if (unlikely(entries == 0)){
                    __RING_STAT_ADD(r, deq_fail, n);
                    return 0;
                }

                n = entries;
            }
        }

        cons_next = cons_head + n;
        success = rte_atomic32_cmpset(&r->cons.head, cons_head,
                          cons_next);
    } while (unlikely(success == 0));

    /* copy in table */
    __rmb();
    for (i = 0; likely(i < n); i++) {
        obj_table[i] = r->ring[(cons_head + i) & mask];
    }

    /*
     * If there are other dequeues in progress that preceded us,
     * we need to wait for them to complete
     */
    while (unlikely(r->cons.tail != cons_head))
        __relax();

    __RING_STAT_ADD(r, deq_success, n);
    r->cons.tail = cons_next;

    return behavior == RING_QUEUE_FIXED ? 0 : n;
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
 *   RING_QUEUE_FIXED:    Dequeue a fixed number of items from a ring
 *   RING_QUEUE_VARIABLE: Dequeue as many items a possible from ring
 * @return
 *   Depend on the behavior value
 *   if behavior = RING_QUEUE_FIXED
 *   - 0: Success; objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue; no object is
 *     dequeued.
 *   if behavior = RING_QUEUE_VARIABLE
 *   - n: Actual number of objects dequeued.
 */
static inline int
__ringbuf_sc_do_dequeue(struct ringbuf *r, void **obj_table,
         unsigned n, enum ringbuf_queue_behavior behavior)
{
    uint32_t cons_head, prod_tail;
    uint32_t cons_next, entries;
    unsigned i;
    uint32_t mask = r->prod.mask;

    cons_head = r->cons.head;
    prod_tail = r->prod.tail;
    /* The subtraction is done between two unsigned 32bits value
     * (the result is always modulo 32 bits even if we have
     * cons_head > prod_tail). So 'entries' is always between 0
     * and size(ring)-1. */
    entries = prod_tail - cons_head;

    if (unlikely(n > entries)) {
        if (behavior == RING_QUEUE_FIXED) {
            __RING_STAT_ADD(r, deq_fail, n);
            return -ENOENT;
        }
        else {
            if (unlikely(entries == 0)){
                __RING_STAT_ADD(r, deq_fail, n);
                return 0;
            }

            n = entries;
        }
    }

    cons_next = cons_head + n;
    r->cons.head = cons_next;

    /* copy in table */
    __rmb();
    for (i = 0; likely(i < n); i++) {
        /* WTF??? WHY DOES THIS CODE GIVE STRICT-ALIASING WARNINGS
         * ON SOME GCC. THEY ARE FREAKING VOID* !!! */
        obj_table[i] = r->ring[(cons_head + i) & mask];
    }

    __RING_STAT_ADD(r, deq_success, n);
    r->cons.tail = cons_next;
    return behavior == RING_QUEUE_FIXED ? 0 : n;
}

/**
 * Enqueue several objects on the ring (multi-producers safe).
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
ringbuf_mp_enqueue_bulk(struct ringbuf *r, void * const *obj_table,
             unsigned n)
{
    return __ringbuf_mp_do_enqueue(r, obj_table, n, RING_QUEUE_FIXED);
}

/**
 * Enqueue several objects on a ring (NOT multi-producers safe).
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
ringbuf_sp_enqueue_bulk(struct ringbuf *r, void * const *obj_table,
             unsigned n)
{
    return __ringbuf_sp_do_enqueue(r, obj_table, n, RING_QUEUE_FIXED);
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
 *   - 0: Success; objects enqueued.
 *   - -EDQUOT: Quota exceeded. The objects have been enqueued, but the
 *     high water mark is exceeded.
 *   - -ENOBUFS: Not enough room in the ring to enqueue; no object is enqueued.
 */
static inline int
ringbuf_enqueue_bulk(struct ringbuf *r, void * const *obj_table,
              unsigned n)
{
    if (r->prod.sp_enqueue)
        return ringbuf_sp_enqueue_bulk(r, obj_table, n);
    else
        return ringbuf_mp_enqueue_bulk(r, obj_table, n);
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
ringbuf_mp_enqueue(struct ringbuf *r, void *obj)
{
    return ringbuf_mp_enqueue_bulk(r, &obj, 1);
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
ringbuf_sp_enqueue(struct ringbuf *r, void *obj)
{
    return ringbuf_sp_enqueue_bulk(r, &obj, 1);
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
ringbuf_enqueue(struct ringbuf *r, void *obj)
{
    if (r->prod.sp_enqueue)
        return ringbuf_sp_enqueue(r, obj);
    else
        return ringbuf_mp_enqueue(r, obj);
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
ringbuf_mc_dequeue_bulk(struct ringbuf *r, void **obj_table, unsigned n)
{
    return __ringbuf_mc_do_dequeue(r, obj_table, n, RING_QUEUE_FIXED);
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
ringbuf_sc_dequeue_bulk(struct ringbuf *r, void **obj_table, unsigned n)
{
    return __ringbuf_sc_do_dequeue(r, obj_table, n, RING_QUEUE_FIXED);
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
ringbuf_dequeue_bulk(struct ringbuf *r, void **obj_table, unsigned n)
{
    if (r->cons.sc_dequeue)
        return ringbuf_sc_dequeue_bulk(r, obj_table, n);
    else
        return ringbuf_mc_dequeue_bulk(r, obj_table, n);
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
ringbuf_mc_dequeue(struct ringbuf *r, void **obj_p)
{
    return ringbuf_mc_dequeue_bulk(r, obj_p, 1);
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
ringbuf_sc_dequeue(struct ringbuf *r, void **obj_p)
{
    return ringbuf_sc_dequeue_bulk(r, obj_p, 1);
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
ringbuf_dequeue(struct ringbuf *r, void **obj_p)
{
    if (r->cons.sc_dequeue)
        return ringbuf_sc_dequeue(r, obj_p);
    else
        return ringbuf_mc_dequeue(r, obj_p);
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
ringbuf_full(const struct ringbuf *r)
{
    uint32_t prod_tail = r->prod.tail;
    uint32_t cons_tail = r->cons.tail;
    return (((cons_tail - prod_tail - 1) & r->prod.mask) == 0);
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
ringbuf_empty(const struct ringbuf *r)
{
    uint32_t prod_tail = r->prod.tail;
    uint32_t cons_tail = r->cons.tail;
    return !!(cons_tail == prod_tail);
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
ringbuf_count(const struct ringbuf *r)
{
    uint32_t prod_tail = r->prod.tail;
    uint32_t cons_tail = r->cons.tail;
    return ((prod_tail - cons_tail) & r->prod.mask);
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
ringbuf_free_count(const struct ringbuf *r)
{
    uint32_t prod_tail = r->prod.tail;
    uint32_t cons_tail = r->cons.tail;
    return ((cons_tail - prod_tail - 1) & r->prod.mask);
}

/**
 * Dump the status of all rings on the console
 */
void ringbuf_list_dump(void);

/**
 * Search a ring from its name
 *
 * @param name
 *   The name of the ring.
 * @return
 *   The pointer to the ring matching the name, or NULL if not found,
 *   with rte_errno set appropriately. Possible rte_errno values include:
 *    - ENOENT - required entry not available to return.
 */
struct ringbuf *ringbuf_lookup(const char *name);

/**
 * Enqueue several objects on the ring (multi-producers safe).
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
ringbuf_mp_enqueue_burst(struct ringbuf *r, void * const *obj_table,
             unsigned n)
{
    return __ringbuf_mp_do_enqueue(r, obj_table, n, RING_QUEUE_VARIABLE);
}

/**
 * Enqueue several objects on a ring (NOT multi-producers safe).
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
ringbuf_sp_enqueue_burst(struct ringbuf *r, void * const *obj_table,
             unsigned n)
{
    return __ringbuf_sp_do_enqueue(r, obj_table, n, RING_QUEUE_VARIABLE);
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
ringbuf_enqueue_burst(struct ringbuf *r, void * const *obj_table,
              unsigned n)
{
    if (r->prod.sp_enqueue)
        return    ringbuf_sp_enqueue_burst(r, obj_table, n);
    else
        return    ringbuf_mp_enqueue_burst(r, obj_table, n);
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
ringbuf_mc_dequeue_burst(struct ringbuf *r, void **obj_table, unsigned n)
{
    return __ringbuf_mc_do_dequeue(r, obj_table, n, RING_QUEUE_VARIABLE);
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
ringbuf_sc_dequeue_burst(struct ringbuf *r, void **obj_table, unsigned n)
{
    return __ringbuf_sc_do_dequeue(r, obj_table, n, RING_QUEUE_VARIABLE);
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
ringbuf_dequeue_burst(struct ringbuf *r, void **obj_table, unsigned n)
{
    if (r->cons.sc_dequeue)
        return ringbuf_sc_dequeue_burst(r, obj_table, n);
    else
        return ringbuf_mc_dequeue_burst(r, obj_table, n);
}

int ringbuf_selftest(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___FAST_RINGBUF_H_8587652_1440385495__ */

/* EOF */
