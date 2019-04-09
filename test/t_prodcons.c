/*
 * Producer-consumer tests with two threads.
 *
 * (c) 2015 Sudhi Herle <sw-at-herle.net> 
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

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>
#include <sys/time.h>
#include "utils/utils.h"
#include "utils/cpu.h"
#include "error.h"
#include "fast/syncq.h"


#define QSIZ    128
SYNCQ_TYPEDEF(pcq, int, QSIZ);

struct context {
    pcq q;

    uint64_t loops; // number of producer iterations

    uint64_t pcyc;  // time taken by producer
    uint64_t ccyc;  // time taken by consumer
};
typedef struct context context;

static void*
producer(void* v)
{
    context* c = v;
    pcq* q = &c->q;
    uint64_t i;

    /* Push into queue until it fills */
    for (i = 0; i < c->loops; i++) {
        uint64_t t0 = sys_cpu_timestamp();

        SYNCQ_ENQ(q, i);

        c->pcyc += (sys_cpu_timestamp() - t0);
    }

    return 0;
}



static void*
consumer(void* v)
{
    context* c = v;
    pcq* q = &c->q;

    uint64_t  i = 0;

    for (i = 0; i < c->loops; i++) {
        uint64_t t0 = sys_cpu_timestamp();
        uint64_t j = SYNCQ_DEQ(q);

        c->ccyc += (sys_cpu_timestamp() - t0);

        if (j != i)
            error(1, 0, "deq mismatch; exp %" PRIu64 ", saw %" PRIu64 " [n %d]",
                    i, j, c->loops);
    }

    return 0;
}

static void
mt_test()
{
    int ncpu = sys_cpu_getavail();
    int cpu  = 0;

    context cx = {
        .loops = QSIZ * 10,
        .pcyc  = 0,
        .ccyc  = 0,
    };

    SYNCQ_INIT(&cx.q, QSIZ);

    pthread_t p, c; // producer & consumer
    int r;

    if ((r = pthread_create(&p, 0, producer, &cx)) != 0)
        error(1, r, "Can't create producer thread");

    if (cpu < ncpu)
        sys_cpu_set_thread_affinity(p, cpu++);

    if ((r = pthread_create(&c, 0, consumer, &cx)) != 0)
        error(1, r, "Can't create consumer thread");

    if (cpu < ncpu)
        sys_cpu_set_thread_affinity(c, cpu++);

    pthread_join(p, 0);
    pthread_join(c, 0);


#define dd(x)   ((double)(x))
    uint64_t n  = cx.loops;

    printf("%" PRIu64 " items; %5.2f cyc/producer %5.2f cyc/consumer\n", n,
            dd(cx.pcyc)/dd(n), dd(cx.ccyc) / dd(n));

    SYNCQ_FINI(&cx.q);

    memset(&cx, 0x5a, sizeof cx);
}


static void
basic_test()
{
    /* Declare a local queue of 4 slots */
    SYNCQ_TYPEDEF(qq_type, int, 4);

    qq_type zq;
    qq_type* q = &zq;
    int j;

    SYNCQ_INIT(q, 4);

    SYNCQ_ENQ(q, 1);
    SYNCQ_ENQ(q, 2);
    SYNCQ_ENQ(q, 3);
    SYNCQ_ENQ(q, 4);

    j = SYNCQ_DEQ(q);   assert(j == 1);
    j = SYNCQ_DEQ(q);   assert(j == 2);
    j = SYNCQ_DEQ(q);   assert(j == 3);
    j = SYNCQ_DEQ(q);   assert(j == 4);

    SYNCQ_FINI(q);
}

int
main(int argc, const char *argv[])
{
    (void)argc;
    program_name = argv[0];

    basic_test();

#if 1
    int i = 0;
    int n = 32;
    for (i = 0; i < n; ++i) {
        mt_test();
        usleep(900 * 100);
    }
#endif
    return 0;
}
