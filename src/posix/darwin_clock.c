/*
 * Limited emulation of clock_gettime() for OS X Darwin
 */
#include <errno.h>
#include <sys/types.h>
#include <sys/_types/_timespec.h>
#include <mach/mach.h>
#include <mach/clock.h>


/*
 * This should be our fixed-up header file
 */
#include <time.h>


int
clock_gettime(clockid_t id, struct timespec * ts)
{
    clock_serv_t cc;
    mach_timespec_t mts;
    int r;

    switch (id)
    {
        case CLOCK_REALTIME:
        case CLOCK_MONOTONIC:
            host_get_clock_service(mach_host_self(), id, &cc);
            r = clock_get_time(cc, &mts);
            mach_port_deallocate(mach_task_self(), cc);
            ts->tv_sec  = mts.tv_sec;
            ts->tv_nsec = mts.tv_nsec;
            break;

        default:
            return -ENOSYS;
    }

    return r;
}

/* EOF */
