/* vim: ts=4:sw=4:expandtab:
 *
 * gettimeofday.c - Win32 implementation of a popular POSIX function
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
 */
#include <time.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <errno.h>
#include <windows.h>



/* Get the current time of day and timezone information,
 * putting it into *TV and *TZ.  If TZ is NULL, *TZ is not filled.
 * Returns 0 on success, -1 on errors. 
 */
int
gettimeofday_ns(struct timespec *tv, struct timezone *z)
{
    int res = -1;

    if (z != NULL)
    {
        TIME_ZONE_INFORMATION  tz_struct;
        DWORD tzi = GetTimeZoneInformation(&tz_struct);

        if (tzi != TIME_ZONE_ID_INVALID)
        {
            z->tz_minuteswest = tz_struct.Bias;
            if (tzi == TIME_ZONE_ID_DAYLIGHT)
                z->tz_dsttime = 1;
            else
                z->tz_dsttime = 0;
        }
        else
        {
            z->tz_minuteswest = 0;
            z->tz_dsttime = 0;
            errno = -EINVAL;
            return -1;
        }
    }

    if (tv != NULL)
    {
        ULARGE_INTEGER fti;

#define FILETIME_1970         0x019db1ded53e8000LL
#define HECTONANOSEC_PER_SEC  10000000LL

        /*
         * fti will hold 100-nanoseconds since 1-1-1601; the actual
         * accuracy on XP seems to be 125,000 nanoseconds.
         */
        GetSystemTimeAsFileTime((LPFILETIME) &fti);

        fti.QuadPart -= FILETIME_1970;

        /*
         * Now, fti.QuadPart holds 100 nano-seconds since 1-1-1970.
         * We want tp.tv_sec to hold _seconds_ since 1-1-1970.
         */
        tv->tv_sec = fti.QuadPart / HECTONANOSEC_PER_SEC ;
        tv->tv_nsec = (long) (fti.QuadPart % HECTONANOSEC_PER_SEC) * 100 ; /* nanoseconds */
        res = 0;
    }
    return res;
}

int
gettimeofday(struct timeval *tv, struct timezone *z)
{
    struct timespec tp;
    int res = gettimeofday_ns(&tp, z);

    if (res >= 0)
        TIMESPEC_TO_TIMEVAL(tv, &tp);
    return res;
}


#ifdef TEST
#include <stdio.h>

int main()
{
    struct timeval p;
    struct timezone z;

    gettimeofday(&p,&z);
    printf("Time now: %lu sec & %lu microsec\n", p.tv_sec, p.tv_usec);
    printf("TZ: %d min; DST: %d\n", z.tz_minuteswest, z.tz_dsttime);
}
#endif /* TEST */
