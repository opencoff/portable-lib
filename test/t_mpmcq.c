/*
 * MPMC Queue Tests
 *
 * (c) 2015 Sudhi Herle <sw-at-herle.net> 
 *
 *
 * Producer-consumer tests with two threads.
 *
 * - Producer threads iterate for 'NITER' times trying to push as
 *   many elements as it can.
 *
 * - Each pushed element is a microsecond timestamp of when it was
 *   pushed (u64).
 *
 * - Command line args decide the number of producers and number of
 *   consumers.
 *
 * - Each consumer pulls items off the queue, timestamps the moment
 *   it was pulled off and records the tuple (timestamp, delta) in a
 *   per-thread array
 *
 * - when all consumers have ended, the arrays are collated, sorted
 *   by timestamp and dumped to stdout.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include "utils/cpu.h"
#include "utils/utils.h"
#include "fast/mpmc_bounded_queue.h"
#include "fast/vect.h"
#include "error.h"
#include <semaphore.h>


#define QSIZ        1024
#define NITER       32768

// Queue of time specs
MPMCQ_TYPEDEF(pcq, uint64_t, QSIZ);


// Latency tuple recorded by the consumer thread(s).
struct lat
{
    uint64_t v;     // measured latency
    uint64_t ts;    // timestamp
};
typedef struct lat lat;


// Array of latency structs
VECT_TYPEDEF(latv, lat);

// Vect of u64
VECT_TYPEDEF(u64v, uint64_t);


sem_t                Ending;    // Used to signal thread completion
atomic_uint_fast32_t Seq;       // Used for verifying correctness
uint32_t             pad0[__mpmc_padz(4)];

atomic_uint_fast32_t Done;      // Used to signal all consumers
uint32_t             pad1[__mpmc_padz(4)];

atomic_uint_fast32_t Start;     // Used to tell all threads to "go"
uint32_t             pad2[__mpmc_padz(4)];



// Per-Thread context
struct ctx
{
    // Cached copy of the shared queue & global vars
    pcq *q;

    atomic_uint_fast32_t *done;
    atomic_uint_fast32_t *start;
    sem_t                *ending;

    // Next two fields are used by the verifiers
    u64v pseq;                  // per-CPU sequence as recorded by producer
    atomic_uint_fast32_t *seq;  // shared sequence number across all producers


    // Per CPU Var
    size_t nelem;   // number of elems pushed
    int  cpu;       // cpu# on which this thread runs
    latv lv;        // latency array (used by consumer only)

    // total CPU cycles for the ENQ or DEQ operation
    // cycles / nelem  will give us time in CPU cycles per ENQ or
    // DEQ operation.
    uint64_t cycles;

    pthread_t id;
};
typedef struct ctx ctx;


// A way to describe the test we want to run
// The performance test and correctness test are mostly the same
// except for the "ending" bits.
struct test_desc
{
    void * (*prod)(void *);
    void * (*cons)(void *);

    void   (*fini)(ctx** pp, int np, ctx** cc, int nc);

    const char* name;
};
typedef struct test_desc test_desc;


// Create a new context
static ctx*
newctx(pcq* q, size_t vsz, size_t ssz)
{
    ctx* c = NEWZ(ctx);
    assert(c);

    // Per CPU vars
    VECT_INIT(&c->lv,   vsz);
    VECT_INIT(&c->pseq, ssz);
    c->nelem  = 0;

    // Shared, cached vars
    c->q      = q;
    c->done   = &Done;
    c->ending = &Ending;
    c->seq    = &Seq;
    c->start  = &Start;

    return c;
}



static void
delctx(ctx* c)
{
    VECT_FINI(&c->lv);
    VECT_FINI(&c->pseq);
    DEL(c);
}


static void
mt_setup(pcq* q)
{
    MPMCQ_INIT(q, QSIZ);

    atomic_init(&Done,   0);
    atomic_init(&Seq,    0);
    atomic_init(&Start,  0);
    sem_init(&Ending, 0, 0);


#if 0
    printf("Sizes:  rd %d wr %d u32 %d fast32 %d\n",
            sizeof(q->q.rd), sizeof(q->q.wr), sizeof(uint32_t), sizeof(uint_fast32_t));
#endif
}

static void
mt_fini(pcq* q)
{
    MPMCQ_FINI(q);
    sem_destroy(&Ending);

    atomic_init(&Done,   0);
    atomic_init(&Seq,    0);
    atomic_init(&Start,  0);
}





// Simple busywait.
// usleep() is not granular enough
static void
upause(int n)
{
    int i;
    volatile int j = 9900;
    for (i = 0; i < n; ++i)
    {
        j += i;
    }
}


static void
waitstart(ctx* c)
{
    while (!atomic_load(c->start))
        upause(100);

    c->nelem  = 0;
    c->cycles = 0;
}



// Producer thread for perf. test
// Pushes a current timestamp.
static void*
perf_producer(void* v)
{
    ctx* c = v;
    pcq* q = c->q;
    int i = 0;


    // Wait to be kicked off
    waitstart(c);

    /* Push into queue until it fills */
    for (i = 0; i < NITER; i++) {
        uint64_t nn = timenow();
        uint64_t t0 = sys_cpu_timestamp();
        if(!MPMCQ_ENQ(q, nn)) {
            upause(100);
            continue;
        }
        c->cycles += (sys_cpu_timestamp() - t0);
        c->nelem++;
    }

    sem_post(c->ending);
    return (void*)0;
}



static inline void
perf_drain(ctx* c)
{
    pcq* q  = c->q;
    latv* v = &c->lv;
    uint64_t j;

    do {
        uint64_t t0 = sys_cpu_timestamp();
        if (!MPMCQ_DEQ(q, j)) {
            upause(100);
            break;
        }

        c->cycles += (sys_cpu_timestamp() - t0);
        c->nelem++;
        uint64_t nn = timenow();
        lat      ll = { .v = nn - j, .ts = nn };
        VECT_APPEND(v, ll);
    } while (1);
}




static void*
perf_consumer(void *v)
{
    ctx* c = v;

    // Wait to be kicked off
    waitstart(c);

    do { 
        perf_drain(c);
    } while (!atomic_load(c->done));

    // Go through one last time - the producer may have put
    // something in there between the time we drained and checked
    // the atomic_load().
    perf_drain(c);

    return (void*)0;
}



// sort func that orders by time-stamp
static int
ts_cmp(const void* a, const void* b)
{
    const lat* x = a;
    const lat* y = b;

    if (x->ts < y->ts)
        return -1;

    if (x->ts > y->ts)
        return +1;

    return 0;
}


// sort func that orders byompares latency value
static int
lat_cmp(const void* a, const void* b)
{
    const lat* x = a;
    const lat* y = b;

    if (x->v < y->v)
        return -1;

    if (x->v > y->v)
        return +1;

    return 0;
}


// finisher for the performance test
// This is called after all the producer and consumer threads have
// completed their tasks
static void
perf_finisher(ctx** pp, int np, ctx** cc, int nc)
{
    int i;
    size_t tot   = 0;
    uint64_t cyc = 0;

#define _d(a) ((double)(a))

    for (i = 0; i < np; ++i) {
        ctx* cx = pp[i];
        double speed = _d(cx->cycles) / _d(cx->nelem);
        
        printf("#    P %d on cpu %d, %zd elem %5.2f cy/enq\n", i, cx->cpu, cx->nelem, speed);
        tot += cx->nelem;
        cyc += cx->cycles;
        delctx(cx);
    }
    printf("#    P Total %zd elem, %5.2f cy/enq\n", tot, _d(cyc) / _d(tot));

    latv all;
    latv pctile;

    VECT_INIT(&all, tot);
    VECT_INIT(&pctile, tot);

    tot = 0;
    cyc = 0;
    for (i = 0; i < nc; ++i) {
        ctx* cx = cc[i];
        double speed = _d(cx->cycles) / _d(cx->nelem);

        printf("#    C %d on cpu %d, %zd elem %5.2f cy/deq\n", i, cx->cpu, VECT_SIZE(&cx->lv), speed);
        VECT_APPEND_VECT(&all, &cx->lv);
        tot += VECT_SIZE(&cx->lv);
        cyc += cx->cycles;
        delctx(cx);
    }

    printf("#    C Total %zd elem, %5.2f cy/enq\n", tot, _d(cyc) / _d(tot));

    assert(tot == VECT_SIZE(&all));

    VECT_APPEND_VECT(&pctile, &all);

    VECT_SORT(&all, ts_cmp);
    VECT_SORT(&pctile, lat_cmp);

    // Percentiles: 99, 70, 50
    int64_t p99 = 99 * tot / 100,
            p70 = 70 * tot / 100,
            p50 = 70 * tot / 100;

    printf("# Latencies: (percentiles)\n"
           "#     99th: %" PRIu64 "\n"
           "#     70th: %" PRIu64 "\n"
           "#     50th: %" PRIu64 "\n",
           VECT_ELEM(&pctile, p99).v,
           VECT_ELEM(&pctile, p70).v,
           VECT_ELEM(&pctile, p50).v);

    lat* v;
    VECT_FOR_EACH(&all, v) {
        printf("%" PRIu64 "\n", v->v);
    }

    VECT_FINI(&all);
}



// -- Correctness functions --

// Producer thread for perf. test
// Pushes a seq#
static void*
vrfy_producer(void* v)
{
    ctx* c   = v;
    pcq* q   = c->q;
    int i    = 0;
    uint_fast32_t n = atomic_fetch_add(c->seq, 1);

    // Wait to be kicked off
    waitstart(c);

    /* Push into queue until it fills */
    for (i = 0; i < NITER; i++) {
        uint32_t z = n;

        if (MPMCQ_ENQ(q, z)) {
            VECT_APPEND(&c->pseq, z);
            n = atomic_fetch_add(c->seq, 1);
        }
        else
            upause(100);
    }

    c->nelem = VECT_SIZE(&c->pseq);
    sem_post(c->ending);
    return (void*)0;
}



static inline void
vrfy_drain(ctx* c)
{
    pcq* q  = c->q;
    latv* v = &c->lv;
    uint64_t j;

    do {
        if (!MPMCQ_DEQ(q, j))
            break;

        lat      ll = { .v = j, .ts = 0 };
        VECT_APPEND(v, ll);
    } while (1);
}


static void*
vrfy_consumer(void *v)
{
    ctx* c = v;

    // Wait to be kicked off
    waitstart(c);

    do { 
        vrfy_drain(c);
    } while (!atomic_load(c->done));

    // Go through one last time - the producer may have put
    // something in there between the time we drained and checked
    // the atomic_load().
    vrfy_drain(c);

    return (void*)0;
}


// Compare sequence numbers
static int
seq_cmp(const void* a, const void* b)
{
    const lat* x = a;
    const lat* y = b;

    if (x->v < y->v)
        return -1;

    if (x->v > y->v)
        return +1;

    return 0;
}




// Finisher for the correctness test
static void
vrfy_finisher(ctx** pp, int np, ctx** cc, int nc)
{
    u64v   allp;
    u64v   allc;

    VECT_INIT(&allp, np * NITER);

    int    i;
    size_t tot = 0;
    for (i = 0; i < np; ++i) {
        ctx* cx = pp[i];
        
        printf("#    P %d on cpu %d, %zd elem\n", i, cx->cpu, cx->nelem);
        tot += cx->nelem;

        VECT_APPEND_VECT(&allp, &cx->pseq);

        delctx(cx);
    }

    assert(tot > 1);
    assert(tot == VECT_SIZE(&allp));

    printf("#    Total %zd elem\n", tot);

    VECT_INIT(&allc, tot);

    for (i = 0; i < nc; ++i) {
        ctx* cx = cc[i];
        lat* ll;

        printf("#    C %d on cpu %d, %zd elem\n", i, cx->cpu, VECT_SIZE(&cx->lv));
        
        // Only use the seq# we got off the queue
        VECT_FOR_EACH(&cx->lv, ll) {
            VECT_APPEND(&allc, ll->v);
        }
        delctx(cx);
    }

    assert(tot == VECT_SIZE(&allc));


    // Now sort based on sequence#
    VECT_SORT(&allc, seq_cmp);
    VECT_SORT(&allp, seq_cmp);

    size_t j;
    for (j = 0; j < tot; ++j) {
        uint64_t ps = VECT_ELEM(&allp, j);
        uint64_t cs = VECT_ELEM(&allc, j);

        if (ps != cs)
            error(0, 0, "Missing seq# %" PRIu64 " (saw %" PRIu64 ")\n", ps, cs);
    }

    VECT_FINI(&allp);
    VECT_FINI(&allc);
}



static void
mt_test(test_desc* tt, int np, int nc)
{
    pcq *q = NEWZ(pcq);
    int i, r;
    int nmax = sys_cpu_getavail();
    int cpu  = 0;

    ctx*      cc[nc];
    ctx*      pp[np];

    printf("# [%s] %d CPUs; %d P, %d C, QSIZ %d N %d\n", tt->name, nmax, np, nc, QSIZ, NITER);

    mt_setup(q);

    for (i = 0; i < np; ++i) {

        // Producers don't use the latency queue. So, set it up to
        // be zero.
        ctx * cx = newctx(q, 0, NITER);


        pp[i] = cx;
        if ((r = pthread_create(&cx->id, 0, tt->prod, cx)) != 0)
            error(1, r, "Can't create producer thread");

        if (cpu < nmax) {
            cx->cpu = cpu++;
            sys_cpu_set_thread_affinity(cx->id, cx->cpu);
        }
    }

    for (i = 0; i < nc; ++i) {
        ctx* cx = newctx(q, NITER*2, 0);

        cc[i] = cx;
        if ((r = pthread_create(&cx->id, 0, tt->cons, cx)) != 0)
            error(1, r, "Can't create consumer thread");

        if (cpu < nmax) {
            cx->cpu = cpu++;

            sys_cpu_set_thread_affinity(cx->id, cx->cpu);
        }
    }

    // Now kick off all threads
    atomic_store(&Start, 1);

    int rem = np;

    /*
     * Now wait for all producers to finish
     */
    for (rem = np; rem > 0; --rem) {
        sem_wait(&Ending);
    }
    atomic_store(&Done, 1);

    void* x = 0;

    // Harvest the Producer threads
    for (i = 0; i < np; ++i) {
        ctx* cx = pp[i];
        pthread_join(cx->id, &x);
    }

    // Now wait and harvest consumer threads
    for (i = 0; i < nc; ++i) {
        ctx* cx = cc[i];

        pthread_join(cx->id, &x);
    }

    (*tt->fini)(pp, np, cc, nc);


    mt_fini(q);
    DEL(q);
}


static void
basic_test()
{
    /* Declare a local queue of 4 slots */
    MPMCQ_TYPEDEF(qq_type, int, 4);

    qq_type* q = NEWZ(qq_type);
    int s;

    MPMCQ_INIT(q, 4);

    s = MPMCQ_ENQ(q, 10);      assert(s == 1);
    s = MPMCQ_ENQ(q, 11);      assert(s == 1);
    s = MPMCQ_ENQ(q, 12);      assert(s == 1);

    assert(MPMCQ_SIZE(q) == 3);

    s = MPMCQ_ENQ(q, 13);      assert(s == 1);

    assert(MPMCQ_SIZE(q) == 4);
    assert(MPMCQ_FULL_P(q));
    assert(!MPMCQ_EMPTY_P(q));

    // 5th add should fail.
    s = MPMCQ_ENQ(q, 14);      assert(s == 0);

    // consistency check should pass
    assert(__MPMCQ_CONSISTENCY_CHECK(q) == 0);

    // Until we drain the queue
    int j;
    s = MPMCQ_DEQ(q, j);       assert(s == 1); assert(j == 10);
    s = MPMCQ_DEQ(q, j);       assert(s == 1); assert(j == 11);
    s = MPMCQ_DEQ(q, j);       assert(s == 1); assert(j == 12);

    assert(MPMCQ_SIZE(q) == 1);

    s = MPMCQ_DEQ(q, j);       assert(s == 1); assert(j == 13);

    assert(MPMCQ_SIZE(q) == 0);
    assert(MPMCQ_EMPTY_P(q));
    assert(!MPMCQ_FULL_P(q));

    // and we shouldn't be able to drain an empty queue
    s = MPMCQ_DEQ(q, j);       assert(s == 0);

    MPMCQ_FINI(q);
    DEL(q);
}



static void
basic_dyn_test()
{
    MPMCQ_DYN_TYPEDEF(dq_type, int);

    dq_type* q = NEWZ(dq_type);
    int s;

    MPMCQ_DYN_INIT(q, 4);

    s = MPMCQ_ENQ(q, 10);      assert(s == 1);
    s = MPMCQ_ENQ(q, 11);      assert(s == 1);
    s = MPMCQ_ENQ(q, 12);      assert(s == 1);
    s = MPMCQ_ENQ(q, 13);      assert(s == 1);

    s = MPMCQ_ENQ(q, 14);      assert(s == 0);


    int j;
    s = MPMCQ_DEQ(q, j);       assert(s == 1); assert(j == 10);
    s = MPMCQ_DEQ(q, j);       assert(s == 1); assert(j == 11);
    s = MPMCQ_DEQ(q, j);       assert(s == 1); assert(j == 12);
    s = MPMCQ_DEQ(q, j);       assert(s == 1); assert(j == 13);

    s = MPMCQ_DEQ(q, j);       assert(s == 0);

    MPMCQ_DYN_FINI(q);
    DEL(q);
}

int
main(int argc, char** argv)
{
    int ncpu = sys_cpu_getavail(),
        half = ncpu >> 1;
    int p = 0,
        c = 0;


    if (half == 0)
        half = 1;

    program_name = argv[0];

    if (argc > 2) {
        int z = atoi(argv[2]);
        if (z < 0)
            error(0, 0, "Ignoring invalid consumer count '%d'; defaulting to %d", z, half);
        else if (z > 0)
            c = z;
    }

    if (argc > 1) {
        int z = atoi(argv[1]);
        if (z < 0)
            error(0, 0, "Ignoring invalid producer count '%d'; defaulting to %d", z, half);
        else if (z > 0)
            p = z;
    }

    if (c == 0)
        c = half;
    if (p == 0)
        p = half;

    basic_test();
    basic_dyn_test();


    test_desc perf = { .prod = perf_producer,
                       .cons = perf_consumer,
                       .fini = perf_finisher,
                       .name = "perf-test",
                     };

    test_desc vrfy = { .prod = vrfy_producer,
                       .cons = vrfy_consumer,
                       .fini = vrfy_finisher,
                       .name = "self-test",
                     };

    mt_test(&vrfy, p, c);
    mt_test(&perf, p, c);

    return 0;
}

/* EOF */
