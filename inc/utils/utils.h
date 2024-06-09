/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils.h - simple C utilities
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
 * Creation date: Sat Jan 21 18:09:15 2006
 */

#ifndef __UTILS_H_1137888555_79680__
#define __UTILS_H_1137888555_79680__ 1


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h> /* ptrdiff_t */
#include <time.h>
#include <sys/time.h>

#include "utils/new.h"
#include "utils/strutils.h"
#include "utils/typeutils.h"

extern int strcasecmp(const char* src, const char* dest);

/**
 * Dump the contents of 'buf' to file 'fp' in the style of
 * 'hexdump -C'.
 *
 * Returns:
 *      0       on success
 *      -errno  on error
 */
extern int fhexdump(FILE *fp, void *buf, size_t bufsz);


/*
 * hex_dumper holds the state for a hexdump session.
 * Each session starts with hex_dumper_init() and ends
 * with hex_dumper_close().
 *
 * Callers can call hex_dumper_write() multiple times before calling
 * hex_dumper_close().
 */
struct hex_dumper {
    int (*writer)(void *ctx, void *buf, size_t nbytes);
    void *ctx;

    int used;   // # of used bytes
    int n;      // # of total bytes
    char right[18];
    char buf[32];
};
typedef struct hex_dumper hex_dumper;


/**
 * initialize a dumper with a custom write function.
 *
 * The 'writer' should return 0 on success and negative number
 * (-errno) on error. Errors are passed back to the caller intact.
 */
extern void hex_dumper_init(hex_dumper *d, int (*writer)(void *ctx, void *buf, size_t nbytes), void *ctx);


/**
 * Dump 'n' bytes of data from buffer 'buf'
 *
 * Returns 0 on success or the error from the writer.
 */
extern int hex_dumper_write(hex_dumper *d, void *buf, size_t n);


/**
 * flush any pending writes and close the dumper.
 *
 * Returns 0 on success or the error from the writer.
 */
extern int hex_dumper_close(hex_dumper *d);


#if defined(_WIN32) || defined(WIN32)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return basename of a path. The return value will be pointer to
 * the basename component of 'p'. i.e., no new string will be
 * allocated.
 *
 * @param p - path whose basename will be returned.
 */

extern const char* basename(const char* p);

/**
 * Return dirname of a path. This function will _modify_ the
 * original string and return a pointer to the dirname.
 *
 * @param p - path whose dirname will be returned.
 */
extern char* dirname(char* p);


#ifdef __cplusplus
}
#endif

#else

#include <strings.h>    /* strcasecmp */
#include <libgen.h>     /* basename, dirname */

#endif /* WIN32 */


    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */






/*
 * Other utility functions
 */


#ifdef __GNUC__
#define likely(x)     __builtin_expect(!!(x),1)
#define unlikely(x)   __builtin_expect(!!(x),0)
#else
#define likely(x)      (x)
#define unlikely(x)    (x)
#endif /* __GNUC__ */



/*
 * Return time in microseconds
 */
static inline uint64_t
timenow_us(void)
{
    struct timeval tv = {0, 0};

    gettimeofday(&tv, 0);
    return tv.tv_usec + (1000000 * tv.tv_sec);
}

typedef uint64_t duration_t;

// Number of duration units in a second
#define __Duration       ((duration_t)1000000000)
#define __dur(a)         ((duration_t)(a))

#define _Hour(n)         (_Minute(n) * 24)
#define _Minute(n)       (_Second(n) * 60)
#define _Second(n)       (__dur(n) * __Duration)
#define _Millisecond(n)  (_Second(n) / 1000)
#define _Microsecond(n)  (_Millisecond(n) / 1000)
#define _Nanosecond(n)   __dur(n)

/*
 * Return time in nanoseconds
 */
static inline duration_t
timenow(void)
{
    struct timespec tv = {0, 0};

    clock_gettime(CLOCK_MONOTONIC, &tv);
    return tv.tv_nsec + _Second(tv.tv_sec);
}


/*
 * Performance counter access.
 *
 * NB: Relative cycle counts and difference between two
 *     cpu-timestamps are meaningful ONLY when run on the _same_ CPU.
 */
#if defined(__i386__)

static inline uint64_t sys_cpu_timestamp(void)
{
    uint64_t x;
    __asm__ volatile ("rdtsc" : "=A" (x));
    return x;
}

static inline void sys_cpu_pause(void)
{
    __asm__ volatile ("pause" ::: "memory");
}

#elif defined(__x86_64__)


static inline uint64_t sys_cpu_timestamp(void)
{
    uint64_t res;
    uint32_t hi, lo;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));

    res = hi;
    return (res << 32) | lo;
}


static inline void sys_cpu_pause(void)
{
    __asm__ volatile ("pause" ::: "memory");
}

#elif defined(__arm64__) || defined(__aarch64__)

// apple silicon
//
// cyc_count in cntvct_el0
// cyc_freq  in cntfreq_el0

#define __sys_read_reg(reg_) ({ \
                                    uint64_t __res = 0;                                     \
                                    __asm__ __volatile__("mrs %0," #reg_ : "=r"(__res));    \
                                    __res;                                                  \
                              })


static inline uint64_t sys_cpu_timestamp(void)
{
    // XXX do we worry about freq? For short benchmarks, this is
    // fine I guess?
    return __sys_read_reg(cntvct_el0);
}


static inline void sys_cpu_pause(void)
{
    __asm__ volatile ("isb\n");
}

#else

#error "I don't know how to get CPU cycle counter for this machine!"

#endif /* x86, x86_64 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __UTILS_H_1137888555_79680__ */

/* EOF */
