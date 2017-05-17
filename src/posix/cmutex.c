/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * cmutex.c - Posix Mutex interface to lockmgr
 *
 * Copyright (c) 2005 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Creation date: Thu Oct 20 11:05:35 2005
 */

#include "utils/lockmgr.h"
#include "utils/utils.h"
#include <pthread.h>


static void
mutex_lock(void* opaq)
{
    pthread_mutex_lock((pthread_mutex_t*)opaq);
}


static void
mutex_unlock(void* opaq)
{
    pthread_mutex_unlock((pthread_mutex_t*)opaq);
}


static void
mutex_delete(void* opaq)
{
    pthread_mutex_destroy((pthread_mutex_t*)opaq);
    DEL(opaq);
}




/* posix mutex ctor */
void
lockmgr_new_mutex(lockmgr* l)
{
    pthread_mutex_t* m;

    if (!l)
        return;

    m = NEWZ(pthread_mutex_t);
    if (!m)
        return;

    if (pthread_mutex_init(m, 0) < 0)
    {
        DEL(m);
        return;
    }

    *l = Mutex_locker;
    l->opaq   = m;
}

const lockmgr Mutex_locker = 
{
    .create = lockmgr_new_mutex,
    .lock   = mutex_lock,
    .unlock = mutex_unlock,
    .dtor   = mutex_delete,
};

/* EOF */
