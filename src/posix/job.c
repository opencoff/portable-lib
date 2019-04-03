/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * job.c - Simple multi-threaded job manager.
 *
 * Copyright (c) 2005 Sudhi Herle <sw at herle.net>
 *
p * Licensing Terms: GPLv2
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Simple job manager to manage multiple threads to utilize full I/O
 * and compute bandwidth of the system.
 *
 * A Job manager starts N threads (one per CPU). Each thread gets
 * its quanta from a common queue. A producer-consumer paradigm is
 * used to get/put jobs from the queue.
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
#include "posix/job.h"
#include "utils/utils.h"
#include "error.h"


struct job_context
{
    job_manager* jm;
    jobfunc_t    func;
    void* context;
    int cpunr;

    pthread_t    id;
};
typedef struct job_context job_context;

/*
 * Get the next job for processing
 * Return 0 if no job, job_ptr otherwise.
 */
static void* job_manager_get(job_manager*);


/*
 * Pthread thread function. Binds to a given CPU and just runs jobs
 * one after another.
 */
static void*
thread_func(void* p)
{
    job_context* tj = (job_context*)p;
    int err        = 0;

    /* First set CPU affinity. */
    sys_cpu_set_my_thread_affinity(tj->cpunr);

    while (1)
    {
        void* j = job_manager_get(tj->jm);

        if (!j)
            break;

        if ((*tj->func)(tj->context, j, tj->cpunr) < 0)
            err++;
    }

    sem_post(&tj->jm->done);

    return (void*)(ptrdiff_t)err;
}


/*
 * Create and initialize a job manager. Return number of threads
 * created.
 */
int
job_manager_init(job_manager* jm, int nthreads, jobfunc_t func, void* ctx)
{
    int i;
    int r;

    memset(jm, 0, sizeof *jm);

    if (nthreads <= 0)
        nthreads = sys_cpu_getavail();

    jm->threads  = NEWZA(job_context, nthreads);
    jm->nthreads = nthreads;


    r = SYNCQ_INIT(&jm->q, JOB_MAX);
    if (r != 0) return r;

    if ((r = sem_init(&jm->done, 0, 0)) != 0)
        return -errno;


    /*
     * Now, start all the threads and have them waiting.
     */
    for (i = 0; i < nthreads; ++i)
    {
        job_context* t  = &jm->threads[i];

        t->jm      = jm;
        t->func    = func;
        t->context = ctx;
        t->cpunr   = i;


        if ((r = pthread_create(&t->id, 0, thread_func, t)) != 0)
        {
            error(0, r, "job manager coulnd't create thread-%d", i);
            return -r;
        }
    }

    return nthreads;
}


/*
 * Delete and destroy a job manager. Caller's responsibility to wait
 * for all threads to exit.
 */
void
job_manager_cleanup(job_manager* jm)
{
    SYNCQ_FINI(&jm->q);
    sem_destroy(&jm->done);

    DEL(jm->threads);
}


void
job_manager_submit_job(job_manager* jm, void* j)
{
    SYNCQ_ENQ(&jm->q, j);
}




int
job_manager_wait(job_manager* jm)
{
    long int err = 0;
    int i;
    void* r;
    int rem = jm->nthreads;


    //printf("Waiting for %d threads to complete ..\n", jm->nthreads);

    while (rem > 0)
    {
        /* Flag each thread that there are no more jobs. */
        job_manager_submit_job(jm, 0);
        sem_wait(&jm->done);
        rem--;
    }

    /*
     * Now, reap the status of each departed thread.
     */
    for (i = 0; i < jm->nthreads; i++)
    {
        pthread_join(jm->threads[i].id, &r);
        err += (int)(ptrdiff_t)r;
    }
    return err;
}



/*
 * This is an internal function. No need for outsiders to see this.
 */
static void*
job_manager_get(job_manager* jm)
{
    return SYNCQ_DEQ(&jm->q);
}

/* EOF */
