/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * job.h - Simple multi-threaded job manager.
 *
 * Simple job manager to manage multiple threads to utilize full I/O
 * and compute bandwidth of the system.
 *
 * The job manager manages a queue of jobs. It creates "N" threads
 * to handle the jobs and assigns work to each thread as it becomes
 * available. There is only one queue shared among the N threads.
 * The implementation uses the classic producer-consumer pattern.
 * Each "job" is identified by an opaque pointer to some datum.
 *
 * On Linux, if required, the job manager knows enough to create as
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
#ifndef __JOB_33891_H__
#define __JOB_33891_H__


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <limits.h>

/* This file is local for Darwin */
#include <semaphore.h>
#include "fast/syncq.h"

#ifdef __cplusplus
extern "C"
#endif



/*
 * Queue size
 */
#define JOB_MAX     1024

/*
 * Queue of jobs
 */
SYNCQ_TYPEDEF(jobq, void*, JOB_MAX);


/*
 * Job manager modeled as a producer-consumer problem.
 */
struct job_manager
{
    jobq    q;

    /*
     * Semaphore to capture thread completion.
     */
    sem_t   done;

    /*
     * threads created by job manager.
     * This is an array of thread-id
     */
    struct job_context* threads;
    int    nthreads;
};
typedef struct job_manager job_manager;


typedef int (*jobfunc_t)(void* ctx, void* job, int threadnr);

/*
 * Initialize the job manager with 'nthreads'. If nthreads is zero,
 * it autoconfigures to the number of CPUs in the system. This may
 * or may not work on all systems.
 *
 * The job function is called for each job that is dequeued. It
 * needs to be thread-aware, and thread-safe.
 */
extern  int job_manager_init(job_manager*, int nthreads, jobfunc_t j, void* ctx);



/*
 * Delete/cleanup job manager
 */
extern void job_manager_destroy(job_manager*);



/*
 * Submit a job.
 */
extern void job_manager_submit_job(job_manager*, void* j);



/*
 * Wait for all jobs to complete. This just waits for the threads to
 * complete.
 */
extern int  job_manager_wait(job_manager*);


#ifdef __cplusplus
}
#endif

#endif /* __JOB_33891_H__ */
