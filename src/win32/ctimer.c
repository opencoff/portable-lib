/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * ctimer.c - Win32 timer thread support routines.
 *
 * Copyright (C) 1998-2006 Sudhi Herle <sw at herle.net>
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
 * Creation date: Wed Jul  5 19:23:17 1998
 */

#include <stdint.h>
#include <windows.h>

static HANDLE Event = 0;

int
sys_thread_start(void (*fp)(void*), void* cookie)
{
    DWORD tid;
    HANDLE h;

    Event = CreateEvent(0, FALSE, 0, 0);
    if (!Event)
        return -1;

    h = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)fp, cookie, 0, &tid);
    if (!h)
        return -1;

    return 0;
}


uint64_t
sys_get_timestamp(void)
{
    ULARGE_INTEGER fti;
    uint64_t secs;
    uint64_t nanosecs;

#define FILETIME_1970         0x019db1ded53e8000LL
#define HECTONANOSEC_PER_SEC  10000000LL

    /*
     * fti will hold 100-nanoseconds since 1-1-1601; the actual
     * accuracy on XP seems to be 125,000 nanoseconds.
     */
    GetSystemTimeAsFileTime ((LPFILETIME) &fti);

    fti.QuadPart -= FILETIME_1970;

    /*
     * Now, fti.QuadPart holds 100 nano-seconds since 1-1-1970.
     * We want tp.tv_sec to hold _seconds_ since 1-1-1970.
     */
    secs     = fti.QuadPart / HECTONANOSEC_PER_SEC ;
    nanosecs = (fti.QuadPart % HECTONANOSEC_PER_SEC) * 100 ; /* nanoseconds */

    return (secs * 1000000) + (nanosecs / 1000);
}

void
sys_sleep(int64_t ms)
{
    WaitforSingleObject(Event, ms < 0 ? INFINITE : (DWORD)ms);
}

void
sys_abort_sleep()
{
    SetEvent(Event);
}


/* EOF */
