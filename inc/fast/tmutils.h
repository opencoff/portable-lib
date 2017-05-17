/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * tmutils.h - Time related utilities
 *
 * Copyright (c) 2015 Sudhi Herle <sw at herle.net>
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

#ifndef ___TMUTILS_H_3878059_1443219715__
#define ___TMUTILS_H_3878059_1443219715__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <time.h>
#include <sys/time.h>
#include <stdint.h>

/*
 * Time is kept in units of microseconds.
 */

static inline uint64_t
now(void)
{
    struct timeval tv;

    gettimeofday(&tv, 0);

    uint64_t z = tv.tv_sec * 1000000;
    return z + tv.tv_usec;
}

// Express 'n' seconds in our canonical format
#define _SEC(n)     ((uint64_t)((n) * 1000000))
#define _MSEC(n)    ((uint64_t)((n) * 1000))
#define _USEC(n)    ((uint64_t)(n))


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___TMUTILS_H_3878059_1443219715__ */

/* EOF */
