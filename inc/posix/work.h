/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * work.h - simple work assignment on a per thread basis
 *
 * The work manager manages a pool of "N" threads - each of which
 * has a queue for submitting work to it. The caller can queue work
 * to the same thread (e.g., serialize "context") or let the system
 * pick the next thread in a round-robin fashion. A "work" is
 * identified by an opaque pointer to some datum.
 *
 * On Linux, if required, the work manager knows enough to create as
 * many threads as there are available CPUs (_SC_NPROCESSORS_ONLN).
 *
 * Copyright (c) 2005 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#ifndef ___WORK_H_9647044_1364315356__
#define ___WORK_H_9647044_1364315356__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <limits.h>

/* This file is local for Darwin */
#include <semaphore.h>



/*
 * Queue size
 */
#define WORK_MAX     32768


/*
 * Work manager
 */
struct work_manager
{
    struct worker_context* ctx;
    int   nthreads;

    pthread_mutex_t lock;
    int   next;

    sem_t done;
};
typedef struct work_manager work_manager;

typedef int (*workfunc_t)(void* ctx, void* work, int threadnr);

extern int work_manager_init(work_manager*, int nthreads, workfunc_t f, void* ctx);
extern int work_manager_submit_work(work_manager*, int next, void* work);
extern int work_manager_wait(work_manager*);
extern int work_manager_destroy(work_manager*);

extern int work_manager_get(work_manager*, void** p_work);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___WORK_H_9647044_1364315356__ */

/* EOF */
