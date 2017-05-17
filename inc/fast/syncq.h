/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * syncq.h - Generic producer/consumer queue.
 *
 * Uses Posix semaphores and mutexes.
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

#ifndef ___PRODCONS_H_4792889_1447780440__
#define ___PRODCONS_H_4792889_1447780440__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <assert.h>
#include <semaphore.h>
#include <pthread.h>
#include "fast/queue.h"



// Handy typedefs
#define __rawq(x)           x ## __objtype

// Holds all the synchronization objects
struct __syncobj
{
    pthread_mutex_t lock;
    sem_t notempty;
    sem_t notfull;
};
typedef struct __syncobj __syncobj;


/** 
 * Define a new sync-queue type 'pqtype' to hold 'SZ' objects of
 * type 'objtyp'.
 */
#define SYNCQ_TYPEDEF(pqtyp, objtyp, SZ)        FQ_TYPEDEF(__rawq(pqtyp), objtyp, SZ); \
                                                struct pqtyp {             \
                                                    __rawq(pqtyp) q;       \
                                                    __syncobj     s;       \
                                                };                         \
                                                typedef struct pqtyp pqtyp


// Internal function to initialize the sync objects
// Return 0 on success, -errno on failure.
static inline int
__syncobj_init(__syncobj* s, size_t n)
{
    int r;

    if ((r = pthread_mutex_init(&s->lock, 0)) != 0)  return -errno;
    if ((r = sem_init(&s->notempty, 0, 0)) != 0)     return -errno;
    if ((r = sem_init(&s->notfull,  0, n)) != 0)     return -errno;

    return 0;
}


/**
 * Initialize a SyncQ 'q0'.
 */
#define SYNCQ_INIT(q0, SZ)       ({ \
                                    typeof(q0)  q = q0; \
                                    FQ_INIT(&q->q, SZ);     \
                                    __syncobj_init(&q->s, SZ); \
                                 })


/**
 * Finalize/delete a syncQ.
 */
#define SYNCQ_FINI(q0)          do { \
                                    typeof(q0)  q = q0; \
                                    __syncobj*  s = &q->s; \
                                    sem_destroy(&s->notfull); \
                                    sem_destroy(&s->notempty); \
                                    pthread_mutex_destroy(&s->lock); \
                                    FQ_FINI(&q->q); \
                                } while (0)


/**
 * Enqueue object 'obj' into queue 'q0'.
 */
#define SYNCQ_ENQ(q0, obj)      do { \
                                    typeof(q0)  q = q0; \
                                    __syncobj*  s = &q->s; \
                                    sem_wait(&s->notfull); \
                                    pthread_mutex_lock(&s->lock); \
                                    FQ_ENQ(&q->q, obj); \
                                    pthread_mutex_unlock(&s->lock); \
                                    sem_post(&s->notempty); \
                                } while (0)



/**
 * Dequeue an object from queue 'q0' and return it.
 */
#define SYNCQ_DEQ(q0, obj)      do { \
                                    typeof(q0)  q = q0; \
                                    __syncobj*  s = &q->s; \
                                    sem_wait(&s->notempty); \
                                    pthread_mutex_lock(&s->lock); \
                                    FQ_DEQ(&q->q, obj);    \
                                    pthread_mutex_unlock(&s->lock); \
                                    sem_post(&s->notfull); \
                                } while (0)
                                

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___PRODCONS_H_4792889_1447780440__ */

/* EOF */
