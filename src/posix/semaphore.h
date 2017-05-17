/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * darwin/semaphore.h - Anon semaphores for Darwin/OS X
 *
 * Copyright (c) 2010 Sudhi Herle <sw at herle.net>
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

#ifndef ___DARWIN_SEMAPHORE_H_8031745_1364254411__
#define ___DARWIN_SEMAPHORE_H_8031745_1364254411__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 * A semaphore can be constructed out of a mutex and condition
 * variable. This struct captures the minimum needed data.
 */
struct darwin_semaphore
{
    /*
     * This has to be marked volatile - otherwise the compiler may
     * optimize its use in loops.
     */
    volatile unsigned int     s_val;

    pthread_mutex_t  s_lock;
    pthread_cond_t   s_cond;

};
typedef struct darwin_semaphore sem_t;


/*
 * Initialize a semaphore.
 * Return 0 on success, -1 on failure; set errno.
 */
extern int sem_init(sem_t*, int pshared, unsigned int val);

/*
 * Destroy a semaphore
 * Return 0 on success, -1 on failure; set errno.
 */
extern int sem_destroy(sem_t*);


/*
 * Wait for semaphore
 * Return 0 on success, -1 on failure; set errno.
 */
extern int sem_wait(sem_t*);

/*
 * Post a semaphore
 * Return 0 on success, -1 on failure; set errno.
 */
extern int sem_post(sem_t*);

/*
 * Non blocking wait on semaphore.
 * Return 0 on success, -1 on failure; errno to EAGAIN
 */
extern int sem_trywait(sem_t*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___DARWIN_SEMAPHORE_H_8031745_1364254411__ */

/* EOF */
