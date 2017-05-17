/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * fast/mpmc_list.h - Multi-producer, Multi-consumer list based
 * queue.
 *
 * Copyright (c) 2015 Sudhi Herle <sw at herle.net>
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

#ifndef ___FAST_MPMC_LIST_H_2499375_1434917655__
#define ___FAST_MPMC_LIST_H_2499375_1434917655__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdatomic.h>

struct mpmc_list_node
{
    struct mpmc_list_node* volatile next;
};
typedef struct mpmc_list_node mpmc_list_node;


struct mpmc_list_queue
{
    mpmc_list_node  stub;
    _Atomic mpmc_list_node  *head;
    _Atomic mpmc_list_node  *tail;
};
typedef struct mpmc_list_queue mpmc_list_queue;


static inline mpmc_list_queue*
mpmc_list_queue_init(mpmc_list_queue* q)
{
    q->stub.next = 0;
    q->head = q->tail = &q->stub;
    return q;
}


static inline void
mpmc_list_queue_enq(mpmc_list_queue* q, mpmc_list_node* n)
{
    n->next = 0;
    mpmc_list_node* prev = atomic_exchange(&q->head, n);

    prev->next = n;
}


static inline mpmc_list_node*
mpmc_list_queue_deq(mpmc_list_queue* q)
{
    mpmc_list_node* tail = q->tail;
    mpmc_list_node* next = tail->next;

    if (tail == &q->stub) {
        if (0 == next)
            return 0;

        q->tail = next;
        tail    = next;
        next    = next->next;
    }

    if (next) {
        q->tail = next;
        return tail;
    }
}

        

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___FAST_MPMC_LIST_H_2499375_1434917655__ */

/* EOF */
