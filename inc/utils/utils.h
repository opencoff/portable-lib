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

extern int strcasecmp(const char* src, const char* dest);

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



/**
 * dump 'buflen' bytes of buffer in 'inbuf' as a series of hex bytes.
 * For each 16 bytes, call 'output' with the supplied cookie as the
 * first argument.
 *
 * @param inbuf     The input buffer to dump
 * @param buflen    Number of bytes to dump
 * @param output    Output function pointer to output hex bytes
 * @param cookie    Opaque cookie to be passed to output function
 * @param msg       Message to be prefixed to the dump
 * @param offsetonly Flag - if set will only print the offsets and
 *                   not actual pointer addresses.
 */
extern void hex_dump_bytes(const void* inbuf, int buflen,
                void (*output)(void*, const char* line, int nbytes),
                void* cookie, const char* msg, int offsetonly);


/**
 * Dump 'inlen' bytes from 'inbuf' into 'buf' which is 'bufsize'
 * bytes big.
 *
 * @param inbuf     The input buffer to dump
 * @param inlen     Number of bytes to dump
 * @param buf       Output buffer to hold the hex dump
 * @param bufsize   Size of output buffer
 * @param msg       Message to be prefixed to the dump
 * @param offsetonly Flag - if set will only print the offsets and
 *                   not actual pointer addresses.
 */
extern void hex_dump_buf(char *buf, int bufsize, const void* inbuf, int inlen, const char* msg, int offsetonly);


/**
 * dump 'buflen' bytes of buffer in 'inbuf' as a series of ascii hex
 * bytes to file 'fp'.
 *
 * @param offsetonly Flag - if set will only print the offsets and
 *                   not actual pointer addresses.
 */
extern void hex_dump_bytes_file(const void* inbuf, int buflen, FILE* fp, const char* msg, int offsetonly);



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

/*
 * Return time in nanoseconds
 */
static inline uint64_t
timenow(void)
{
    struct timespec tv = {0, 0};

    clock_gettime(CLOCK_MONOTONIC, &tv);
    return tv.tv_nsec + (1000000000 * tv.tv_sec);
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

#elif defined(__x86_64__)


static inline uint64_t sys_cpu_timestamp(void)
{
    uint64_t res;
    uint32_t hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));

    res = hi;
    return (res << 32) | lo;
}

#else

#error "I don't know how to get CPU cycle counter for this machine!"

#endif /* x86, x86_64 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __UTILS_H_1137888555_79680__ */

/* EOF */
