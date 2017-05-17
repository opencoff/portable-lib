/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * cmutex.c - Win32 Mutex interface to lockmgr
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
 *
 * Creation date: Thu Oct 20 11:05:35 2005
 */

#include "utils/lockmgr.h"
#include <windows.h>



static void
mutex_lock(void* opaq)
{
    WaitForSingleObject((HANDLE)opaq, INFINITE);
}


static void
mutex_unlock(void* opaq)
{
    ReleaseMutex((HANDLE)opaq);
}


static void
mutex_delete(void* opaq)
{
    CloseHandle((HANDLE)opaq);
}


/* create a win32 mutex abstraction */
lockmgr*
lockmgr_new_mutex(lockmgr* l)
{
    HANDLE h;

    if (!l)
        return 0;

    h = CreateMutex(0, FALSE, "cmutex");
    if (!h)
        return 0;

    l->lock   = mutex_lock;
    l->unlock = mutex_unlock;
    l->dtor   = mutex_delete;
    l->opaq   = (void*)h;

    return l;
}

/* EOF */
