/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * t_mempool.c - simple test harness for fixed size allocator
 *
 * Copyright (c) 2005 Sudhi Herle <sw@herle.net>
 *
 * Licensing Terms: (See LICENSE.txt for details). In short:
 *    1. Free for use in non-commercial and open-source projects.
 *    2. If you wish to use this software in commercial projects,
 *       please contact me for licensing terms.
 *    3. By using this software in commercial OR non-commercial
 *       projects, you agree to send me any and all changes, bug-fixes
 *       or enhancements you make to this software.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Creation date: Thu Oct 20 10:58:22 2005
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include "utils/mempool.h"
#include "fast/vect.h"
#include "utils/utils.h"
#include "error.h"


struct obj
{
    int v;
    void * ptr;
};
typedef struct obj obj;

struct obj2
{
    char b[64];
};
typedef struct obj2 obj2;

VECT_TYPEDEF(ptr_vect, void*);

#define N       (65535 * 16)


#define now()   sys_cpu_timestamp()

extern uint32_t arc4random(void);
extern void arc4random_buf(void *, size_t);


static void
perf_test(int run)
{
    struct mempool* pool;
    ptr_vect        pv;

    if (0 != mempool_new(&pool, 0, sizeof(obj2), N, 0))
        error(1, 0, "Can't initialize mempool of size %zu [%d objs]", sizeof(obj2), N);


    VECT_INIT(&pv, N);

    uint64_t cy_a = 0,
             cy_f = 0,
             tm_a = 0,
             tm_f = 0;

    void * p;
    int i;
    for (i = 0; i < N; ++i) {
        uint64_t t0 = timenow(),
                 c0 = sys_cpu_timestamp();

        p = mempool_alloc(pool);

        cy_a += (sys_cpu_timestamp() - c0);
        tm_a += (timenow() - t0);

        VECT_PUSH_BACK(&pv, p);

        // scribble the memory a bit
        arc4random_buf(p, sizeof(obj2));
    }

    /*
     * Randomize the order and then free.
     * This causes severe cache hit and the rate of frees/sec goes
     * down. 
     *
     * The actual cycle count stays roughly the same with/without
     * this randomization. i.e., the CPU is stalling while
     * filling ((mru_node *)*x)
     */
    //VECT_SHUFFLE(&pv, arc4random);


    void** x;
    VECT_FOR_EACH(&pv, x) {
        volatile uint64_t t0 = timenow(),
                          c0 = sys_cpu_timestamp();

        mempool_free(pool, *x);

        cy_f  += (sys_cpu_timestamp() - c0);
        tm_f  += (timenow() - t0);
    }

#define _ops_per_sec(a) (1000.0 * ((double)N) / (double)(a))
#define _us_per_op(a)   ((double)(a) / (double)N)

    printf("%2d: %6.2f M allocs/sec (%6.4f cyc/alloc)  %6.2f M free/sec (%6.4f cyc/free)\n",
            run, _ops_per_sec(tm_a), _us_per_op(cy_a)
               , _ops_per_sec(tm_f), _us_per_op(cy_f));

    mempool_delete(pool);

}


int
main()
{
    int i;
    for(i = 0; i < 16; ++i)
        perf_test(i);

    return 0;
}

/* EOF */
