/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * work.c - simple round robin distribution of work across
 * n-threads. Each worker has its own queue.
 *
 * Copyright (c) 2005 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>

#include "utils/cpu.h"
#include "posix/work.h"
#include "utils/utils.h"
#include "fast/syncq.h"


SYNCQ_TYPEDEF(workq, void*, WORK_MAX);

struct worker_context
{
    workq    q;
    
    workfunc_t    func;
    void* context;
    int cpunr;

    sem_t *done;
    pthread_t id;
};
typedef struct worker_context worker_context;



/* Submit a new work to a particular thread */
static void
wc_submit(worker_context* wc, void* j)
{
    SYNCQ_ENQ(&wc->q, j);
}


/* Get next wprk item for _this_ thread */
static void*
wc_get(worker_context* wc)
{
    return SYNCQ_DEQ(&wc->q);
}

/*
 * Pthread thread function. Binds to a given CPU and just executes
 * queued work one after another.
 */
static void*
thread_func(void* p)
{
    worker_context* wc = (worker_context*)p;
    int err        = 0;

    /* First set CPU affinity. */
    sys_cpu_set_my_thread_affinity(wc->cpunr);

    while (1)
    {
        void* j = wc_get(wc);

        if (!j)
            break;

        if ((*wc->func)(wc->context, j, wc->cpunr) < 0)
            err++;
    }

    sem_post(wc->done);

    return (void*)(ptrdiff_t)err;
}


/*
 * Create and initialize a work manager. Return number of threads
 * created.
 */
int
work_manager_init(work_manager* wm, int nthreads, workfunc_t func, void* ctx)
{
    int i;
    int r;

    memset(wm, 0, sizeof *wm);

    if (nthreads <= 0)
        nthreads = sys_cpu_getavail();

    wm->ctx      = NEWZA(worker_context, nthreads);
    wm->nthreads = nthreads;
    wm->next     = 0;


    if ((r = sem_init(&wm->done, 0, 0)) != 0)
        return -errno;


    if ((r = pthread_mutex_init(&wm->lock, 0)) != 0)
        return -r;


    /*
     * Now, start all the threads and have them waiting.
     */
    for (i = 0; i < nthreads; ++i)
    {
        worker_context* wc  = &wm->ctx[i];

        r = SYNCQ_INIT(&wc->q, WORK_MAX);
        if (r != 0) return -errno;

        wc->func    = func;
        wc->context = ctx;
        wc->cpunr   = i;
        wc->done    = &wm->done;

        if ((r = pthread_create(&wc->id, 0, thread_func, wc)) != 0)
            return -r;
    }

    return nthreads;
}


/*
 * Delete and destroy a work manager. Caller's responsibility to wait
 * for all threads to exit.
 */
void
work_manager_cleanup(work_manager* wm)
{
    int i;

    for (i = 0; i < wm->nthreads; ++i)
    {
        worker_context* wc = &wm->ctx[i];

        SYNCQ_FINI(&wc->q);
    }

    sem_destroy(&wm->done);
    DEL(wm->ctx);
}


/*
 * Submit a new piece of work. If next is < 0, the new work is load
 * balanced among the available threads. Otherwise, it is serialized
 * to the requested thread.
 */
int
work_manager_submit_work(work_manager* wm, int next, void* j)
{
    if (next < 0 || next >= wm->nthreads)
    {
        pthread_mutex_lock(&wm->lock);

        next     = wm->next;
        wm->next = (wm->next + 1) % wm->nthreads;

        pthread_mutex_unlock(&wm->lock);
    }
    
    wc_submit(&wm->ctx[next], j);
    return next;
}




int
work_manager_wait(work_manager* wm)
{
    long int err = 0;
    int i;
    void* r;
    int rem = wm->nthreads;


    /* first phase: Flag all threads that we are done. */
    for (i = 0; i < wm->nthreads; ++i)
    {
        worker_context* wc = &wm->ctx[i];
        wc_submit(wc, 0);
    }

    /* wait for them to acknowledge */
    while (rem > 0)
    {
        sem_wait(&wm->done);
        rem--;
    }

    /* finally, reap their exit codes. */
    for (i = 0; i < wm->nthreads; i++)
    {
        worker_context* wc = &wm->ctx[i];
        pthread_join(wc->id, &r);
        err += (int)(ptrdiff_t)r;
    }
    return err;
}



/* EOF */
