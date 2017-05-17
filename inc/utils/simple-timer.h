/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * inc/utils/simple-timer.h - Simple linked list of timers
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

#ifndef ___INC_UTILS_SIMPLE_TIMER_H_1091156_1445716990__
#define ___INC_UTILS_SIMPLE_TIMER_H_1091156_1445716990__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include "fast/list.h"

struct st_timeout;
DL_HEAD_TYPEDEF(st_timeout_head, st_timeout);


struct st_instance
{
    // List of pending timers - sorted by expiry of timers
    // Earliest expiry is at the head, latest one at the tail.
    st_timeout_head head;

    uint64_t    start;
    uint64_t    last;

    uint64_t    jitter;

    uint64_t   (*now)(void);
};
typedef struct st_instance st_instance;


extern int  st_init(st_instance*, uint64_t (*curtime)(void));
extern void st_fini(st_instance*);


/*
 * Call (*cb)(cookie) at or immediately after n_ms milliseconds
 * have elapsed.
 *
 * If repeat is True, re-arm the timer when the callback returns.
 */
struct st_timeout* st_add(struct st_instance*, uint32_t n_ms, void(*cb)(void*), void* cookie, int repeat);


/*
 * Delete the timeout. But, this function doesn't need the instance
 * structure because the instance is cached in the timeout struct.
 */
void st_del(struct st_timeout*);



/*
 * Called by an external event-loop to handle expired timers.
 *
 * NB:
 * 1. This should NEVER be called from multiple threads.
 */
void st_expire(struct st_instance*);


/*
 * Dump all the active timers in every wheel.
 */
void st_dumpfp(st_instance* tw, FILE* fp, const char* prefix);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___INC_UTILS_SIMPLE_TIMER_H_1091156_1445716990__ */

/* EOF */
