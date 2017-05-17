/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * darwin_sem.c - Anon semaphore implementatin for Darwin/OSX
 *
 * Copyright (C) 2010, Sudhi Herle <sw at herle.net>
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
#include <pthread.h>
#include <errno.h>

/* Standard headerfile */
#include <semaphore.h>

#include "utils/utils.h"




/*
 * Quick implementation of unnamed semaphores for OSX Darwin
 */



/*
 * Initialize the semaphore
 * Note: We don't support shared semaphores.
 */
int
sem_init(sem_t* s, int pshared, unsigned int val)
{
    int r;

    (void)pshared;  // unused

    s->s_val = val;
    pthread_mutex_init(&s->s_lock, 0);
    if (0 != (r = pthread_cond_init(&s->s_cond, 0)))
        return r;

    return 0;
}


/*
 * Wait for semaphore.
 */
int
sem_wait(sem_t* s)
{
    pthread_mutex_lock(&s->s_lock);


    /*
     * One or more waiters will be woken up. Whoever is woken up
     * will also get the mutex. So, this whole thing is "atomic".
     */
    while (s->s_val == 0)
        pthread_cond_wait(&s->s_cond, &s->s_lock);

    s->s_val--;

    pthread_mutex_unlock(&s->s_lock);
    return 0;
}



/*
 * Post a semaphore
 */
int
sem_post(sem_t* s)
{
    pthread_mutex_lock(&s->s_lock);

    s->s_val++;

    pthread_mutex_unlock(&s->s_lock);

    /* Wake up all waiters */
    pthread_cond_broadcast(&s->s_cond);
    return 0;
}


/*
 * Destroy a semaphore
 * Note: Doesn't try to unblock any waiters.
 */
int
sem_destroy(sem_t* s)
{
    pthread_mutex_destroy(&s->s_lock);
    pthread_cond_destroy(&s->s_cond);
    return 0;
}


/*
 * Non-blocking wait.
 */
int
sem_trywait(sem_t* s)
{
    int r = -1;

    pthread_mutex_lock(&s->s_lock);
    if (s->s_val > 0)
    {
        s->s_val--;
        r = 0;
    }
    pthread_mutex_unlock(&s->s_lock);
    
    if (r != 0)
        errno = EAGAIN;

    return r;
}

/*
 * XXX Timed wait - not done.
 */

/* EOF */
