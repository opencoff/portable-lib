#include <time.h>
#include <stdlib.h>
#include <sys/time.h>       /* gettimeofday */
#include <mach/mach_time.h> /* mach_absolute_time */
#include <mach/mach.h>      /* host_get_clock_service, mach_... */
#include <mach/clock.h>     /* clock_get_time */

#define BILLION   1000000000uLL

static mach_timebase_info_data_t __timebase = { 0, 0 }; /* numer = 0, denom = 0 */
static struct timespec           __inittime = { 0, 0 }; /* nanoseconds since 1-Jan-1970 to init() */
static uint64_t                  __initclock;           /* ticks since boot to init() */

/*
 * Call this once at the start of your program.
 */
static void __attribute__((constructor))
init()
{
    struct timeval  micro;      /* microseconds since 1 Jan 1970 */

    if (mach_timebase_info(&__timebase) != 0)
        abort();                            /* very unlikely error */

    if (gettimeofday(&micro, NULL) != 0)
        abort();                            /* very unlikely error */

    __initclock = mach_absolute_time();

    __inittime.tv_sec  = micro.tv_sec;
    __inittime.tv_nsec = micro.tv_usec * 1000;
}


struct timespec
gettimespec()
{
    struct timespec ts;         /* ns since 1 Jan 1970 to 1500 ms in future */
    uint64_t        clock;      /* ticks since init */
    uint64_t        nano;       /* nanoseconds since init */

    clock = mach_absolute_time() - __initclock;
    nano  = clock * (uint64_t)__timebase.numer / (uint64_t)__timebase.denom;
    ts    = __inittime;

    ts.tv_sec  += nano / BILLION;
    ts.tv_nsec += nano % BILLION;
    return ts;
}
