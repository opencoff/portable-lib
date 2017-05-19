/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * timer_wheel.h - Hashed Timer Wheel Class
 *
 * Copyright (c) 2010 Sudhi Herle <sw at herle.net>
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
 * Creation date: Oct 20, 2010 09:55:01 CDT
 *
 * Implementation based on Scheme-7 of the design outlined
 * in George Vargehese's paper: 
 *    www.cs.columbia.edu/~nahum/w6998/papers/ton97-timing-wheels.pdf
 *
 * This implementation is inspired by the NetBSD callout
 * implementation (sys/callout.h, kern_timeout.c).
 *
 * The internal counters are maintained in units of "ticks".
 *
 * Each instance can be initialized with a different "tick-factor"
 * that provides conversion from caller's time scale to the module's
 * time scale.
 *
 * Internally, timekeeping is in microseconds (gettimeofday()). The
 * elapsed times are converted into ticks. Similarly, expiry times
 * in the future are converted into "future ticks" and stored in
 * "tw_timeout".
 */

#ifndef ___UTILS_TW_WHEEL_H_8663194_1440635622__
#define ___UTILS_TW_WHEEL_H_8663194_1440635622__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include "fast/list.h"

/*
 * - There are WHEELS number of timer wheels
 * - Each wheel counts upto WHEEL_BITS units of time.
 * - Number of slots in _each_ wheel is WHEEL_SIZE
 * - Total number of slots (buckets) is WHEEL_BUCKETS
 */
#define WHEELS            4
#define WHEEL_BITS        3
#define WHEEL_SIZE        (1 << WHEEL_BITS)
#define WHEEL_MASK        (WHEEL_SIZE - 1)
#define WHEEL_BUCKETS     (WHEELS * WHEEL_SIZE)


struct tw_timeout;
struct tw_instance;
struct tw_bucket;
typedef struct tw_timeout tw_timeout;

DL_HEAD_TYPEDEF(tw_timeout_head, tw_timeout);

struct tw_bucket
{
    tw_timeout_head head;  // back pointer

    // XXX per bucket lock goes here
};
typedef struct tw_bucket tw_bucket;


/*
 * Instance of a timer-wheel.
 */
struct tw_instance
{
    tw_bucket     wheel[WHEEL_BUCKETS];

    // Starting microsecond timestamp when this instance was
    // initialized. This is effectively the ZERO value for the
    // timer.
    uint64_t start;

    // time when expire() was called previously
    uint64_t last;

    uint64_t us_per_tick;   // microseconds per tick


    // Counters for interesting events
    uint64_t nlate;
    uint64_t ntimers;

    // Function to fetch the current time
    uint64_t (*now)(void);
};
typedef struct tw_instance tw_instance;

/*
 * Initialize a new instance of a timer-wheel.
 *
 * ticks_per_sec - is the number of ticks per second. A larger
 * number gets finer grained timers. Internally, every expiry time
 * is converted into ticks.
 *
 * Each instance can have a different rate of ticks.
 */
int tw_init(struct tw_instance*, uint32_t ticks_per_sec, uint64_t (*curtime)(void));


/*
 * Stop and cleanup all the timers
 */
void tw_fini(struct tw_instance*);


/*
 * Call (*cb)(cookie) at or immediately after n_ms milliseconds
 * have elapsed.
 *
 * If repeat is True, re-arm the timer when the callback returns.
 */
struct tw_timeout* tw_add(struct tw_instance*, uint64_t n_ms, void(*cb)(void*), void* cookie, int repeat);


/*
 * Delete the timeout. But, this function doesn't need the instance
 * structure because the instance is cached in the timeout struct.
 */
void tw_del(struct tw_timeout*);


/*
 * Called by an external event-loop to handle expired timers.
 * This function obtains the current time (via gettimeofday()) and
 * calculates the elapsed time.
 *
 * NB:
 * 1. This should NEVER be called from multiple threads.
 * 2. This 
 */
void tw_expire(struct tw_instance*);


/*
 * Dump all the active timers in every wheel.
 */
void tw_dumpfp(tw_instance* tw, FILE* fp, const char* prefix);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_TW_WHEEL_H_8663194_1440635622__ */

/* EOF */
