/*
 * Ringbuf tests
 *
 * (c) 2015 Sudhi Herle <sw-at-herle.net>
 *
 *
 * Producer-consumer tests with multiple threads.
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
#include "fast/vect.h"
#include "error.h"
#include <semaphore.h>

#include "fast/ringbuf.h"


#define QSIZ        8192
#define NITER       8192


typedef struct rte_ring pcq;


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

atomic_uint_fast32_t Seq __CACHELINE_ALIGNED;       // Used for verifying correctness

atomic_uint_fast32_t Done __CACHELINE_ALIGNED;      // Used to signal all consumers

atomic_uint_fast32_t Start __CACHELINE_ALIGNED;     // Used to tell all threads to "go"



// Per-Thread context
struct ctx
{
    // Cached copy of the shared queue & global vars
    pcq *q;

    atomic_uint_fast32_t *done;
    atomic_uint_fast32_t *start;
    sem_t                *ending;
    atomic_uint_fast32_t *seq;  // shared sequence number across all producers

    // Next two fields are used by the verifiers
    u64v pseq;                  // per-CPU sequence as recorded by producer

    // Per CPU Var
    size_t nelem;   // number of elems pushed
    int  cpu;       // cpu# on which this thread runs
    latv lv;        // latency array (used by consumer only)

    // total CPU cycles for the ENQ or DEQ operation
    // cycles / nelem  will give us time in CPU cycles per ENQ or
    // DEQ operation.
    uint64_t cycles;

    pthread_t id;

    // Number of producers and consumer threads
    int  np,
         nc;
};
typedef struct ctx ctx;


// A way to describe the test we want to run
// The performance test and correctness test are mostly the same
// except for the "ending" bits.
struct test_desc
{
    void * (*prod)(void *);
    void * (*cons)(void *);

    void   (*fini)(pcq*, ctx** pp, int np, ctx** cc, int nc);

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


static pcq *
mt_setup()
{
    pcq *q = rte_ring_create(QSIZ, 0);

    atomic_init(&Done,   0);
    atomic_init(&Seq,    0);
    atomic_init(&Start,  0);
    sem_init(&Ending, 0, 0);


#if 0
    printf("Sizes:  rd %d wr %d u32 %d fast32 %d\n",
            sizeof(q->q.rd), sizeof(q->q.wr), sizeof(uint32_t), sizeof(uint_fast32_t));
#endif

    return q;
}

static void
mt_fini(pcq* q)
{
    rte_ring_destroy(q);
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
    volatile uint64_t j = sys_cpu_timestamp();
    for (i = 0; i < n; ++i) {
        j += i;
        rte_pause();
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
    while (1) {
        uint64_t nn = timenow();
        uint64_t t0 = sys_cpu_timestamp();
        if (!rte_ring_mp_enqueue(q, (void *)nn)) {
            rte_pause();
            continue;
        }
        c->cycles += (sys_cpu_timestamp() - t0);
        c->nelem++;
        if (++i == NITER) break;
    }

    sem_post(c->ending);
    return (void*)0;
}


static void*
perf_consumer(void *vx)
{
    ctx* c  = vx;
    pcq* q  = c->q;
    latv* v = &c->lv;
    void *vp = 0;

    // Wait to be kicked off
    waitstart(c);

    do {
        uint64_t t0 = sys_cpu_timestamp();
        if (rte_ring_mc_dequeue(q, &vp) <= 0) {
            uint_fast32_t np = atomic_load(c->done);
            if (np) break;

            rte_pause();
            continue;
        }

        c->cycles += (sys_cpu_timestamp() - t0);
        c->nelem++;
        uint64_t j = (uint64_t)vp;
        uint64_t nn = timenow();
        lat      ll = { .v = nn - j, .ts = nn };
        VECT_PUSH_BACK(v, ll);
    } while (1);

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
perf_finisher(pcq *q, ctx** pp, int np, ctx** cc, int nc)
{
    int i;
    size_t tot   = 0;
    uint64_t cyc = 0;

    if (!rte_ring_empty(q)) {
        printf("Expected ringbuf to be empty; saw %d elements\n", rte_ring_count(q));
        assert(rte_ring_empty(q));
    }

#define _d(a) ((double)(a))

    for (i = 0; i < np; ++i) {
        ctx* cx = pp[i];
        double speed = _d(cx->cycles) / _d(cx->nelem);

        printf("#    P %d on cpu %d, %zd elem %5.2f cy/enq\n", i, cx->cpu, cx->nelem, speed);
        tot += cx->nelem;
        cyc += cx->cycles;
        delctx(cx);
    }
    printf("# P Total %zd elem, avg %5.2f cy/enq\n", tot, _d(cyc) / _d(tot));

    latv all;
    latv pctile;

    VECT_INIT(&all, tot);
    VECT_INIT(&pctile, tot);

    tot = 0;
    cyc = 0;
    for (i = 0; i < nc; ++i) {
        ctx* cx = cc[i];
        double speed = _d(cx->cycles) / _d(cx->nelem);

        printf("#    C %d on cpu %d, %zd elem %5.2f cy/deq\n", i, cx->cpu, VECT_LEN(&cx->lv), speed);
        VECT_APPEND_VECT(&all, &cx->lv);
        tot += VECT_LEN(&cx->lv);
        cyc += cx->cycles;
        delctx(cx);
    }

    assert(tot == VECT_LEN(&all));

    printf("# C Total %zd elem, avg %5.2f cy/deq\n", tot, _d(cyc) / _d(tot));

    VECT_APPEND_VECT(&pctile, &all);

    VECT_SORT(&all, ts_cmp);
    VECT_SORT(&pctile, lat_cmp);

    // Percentiles: 99, 70, 50
    int64_t p99 = 99 * tot / 100,
            p70 = 70 * tot / 100,
            p50 = 70 * tot / 100;

    uint64_t median = 0;

    if (VECT_LEN(&pctile) & 1) {
        int x  = VECT_LEN(&pctile)/2;
        median = VECT_ELEM(&pctile, x).v;
    } else {
        int a  = VECT_LEN(&pctile)/2;
        int b  = a-1;
        median = (VECT_ELEM(&pctile, a).v + VECT_ELEM(&pctile, b).v)/2;
    }


    printf("# Latencies: median %" PRIu64 " ns. Percentiles:\n"
           "#  99th: %" PRIu64 "\n"
           "#  70th: %" PRIu64 "\n"
           "#  50th: %" PRIu64 "\n",
           median,
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

// Producer thread for conformity test
// Pushes a seq#
static void*
vrfy_producer(void* v)
{
    ctx* c   = v;
    pcq* q   = c->q;
    int i    = 0;
    uint_fast32_t n = atomic_fetch_add(c->seq, 1);
    uint64_t z;

    // Wait to be kicked off
    waitstart(c);

    /* Push into queue until it fills */
    while (1) {
        z = n;

        if (rte_ring_mp_enqueue(q, (void *)z) <= 0) {
            rte_pause();
            continue;
        }

        VECT_PUSH_BACK(&c->pseq, z);
        n = atomic_fetch_add(c->seq, 1);
        if (++i == NITER) break;
    }

    sem_post(c->ending);
    return (void*)0;
}



static void*
vrfy_consumer(void *vx)
{
    ctx* c  = vx;
    pcq* q  = c->q;
    latv* v = &c->lv;
    void *vp;

    // Wait to be kicked off
    waitstart(c);

    while (1) {
        if (rte_ring_mc_dequeue(q, &vp) <= 0) {
            uint_fast32_t np = atomic_load(c->done);
            if (np) break;
            rte_pause();
            continue;
        }

        lat ll = { .v = (uint64_t)vp, .ts = 0 };
        VECT_PUSH_BACK(v, ll);
    }

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
vrfy_finisher(pcq *q, ctx** pp, int np, ctx** cc, int nc)
{
    u64v   allp;
    u64v   allc;

    if (!rte_ring_empty(q)) {
        printf("Expected ringbuf to be empty; saw %d elements\n", rte_ring_count(q));
        assert(rte_ring_empty(q));
    }

    VECT_INIT(&allp, np * NITER);

    int    i;
    size_t tot = 0;
    for (i = 0; i < np; ++i) {
        ctx* cx = pp[i];

        printf("#    P %d on cpu %d, %zd elem\n", i, cx->cpu, VECT_LEN(&cx->pseq));
        tot += VECT_LEN(&cx->pseq);

        VECT_APPEND_VECT(&allp, &cx->pseq);

        delctx(cx);
    }

    assert(tot > 1);
    assert(tot == VECT_LEN(&allp));

    printf("# P Total %zd elem\n", VECT_LEN(&allp));

    VECT_INIT(&allc, tot);

    for (i = 0; i < nc; ++i) {
        ctx* cx = cc[i];
        lat* ll;

        printf("#    C %d on cpu %d, %zd elem\n", i, cx->cpu, VECT_LEN(&cx->lv));

        // Only use the seq# we got off the queue
        VECT_FOR_EACH(&cx->lv, ll) {
            VECT_PUSH_BACK(&allc, ll->v);
        }
        delctx(cx);
    }

    if (tot != VECT_LEN(&allc)) {
        printf("total mismatch; exp %zu, saw %zu\n", tot, VECT_LEN(&allc));
        assert(tot == VECT_LEN(&allc));
    }

    printf("# C Total %zd elem\n", VECT_LEN(&allc));

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
    int i, r;
    int nmax = sys_cpu_getavail();
    int cpu  = 0;

    ctx*      cc[nc];
    ctx*      pp[np];

    printf("# [%s] %d avail CPUs: %d P, %d C\n"
           "# QSIZ %d, %d elem/producer (total %d elem)\n",
            tt->name, nmax, np, nc, QSIZ, NITER, np * NITER);

    pcq *q = mt_setup();

    for (i = 0; i < np; ++i) {

        // Producers don't use the latency queue. So, set it up to
        // be zero.
        ctx * cx = newctx(q, 0, NITER);

        cx->np = np;
        cx->nc = nc;

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

        cx->np = np;
        cx->nc = nc;

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

    for (i = 0; i < nc; ++i) {
        ctx* cx = cc[i];
        pthread_join(cx->id, &x);
    }

    (*tt->fini)(q, pp, np, cc, nc);


    mt_fini(q);
}


static void
basic_sp_test()
{
    int s;
    struct rte_ring *q;
    union {
        void *vp;
        uint64_t v;
    } u;


    q = rte_ring_create(4, 0);

    s = rte_ring_sp_enqueue(q, (void *)10); assert(s == 1);
    s = rte_ring_sp_enqueue(q, (void *)11); assert(s == 1);
    s = rte_ring_sp_enqueue(q, (void *)12); assert(s == 1);

    assert(rte_ring_count(q) == 3);

    s = rte_ring_sp_enqueue(q, (void *)13); assert(s < 0);
    assert(rte_ring_full(q));
    assert(!rte_ring_empty(q));

    s = rte_ring_mc_dequeue(q, &u.vp); assert(s == 1);
    assert(u.v == 10);

    s = rte_ring_mc_dequeue(q, &u.vp); assert(s == 1);
    assert(u.v == 11);

    s = rte_ring_mc_dequeue(q, &u.vp); assert(s == 1);
    assert(u.v == 12);

    assert(rte_ring_count(q) == 0);
    s = rte_ring_mc_dequeue(q, &u.vp); assert(s < 0);

    assert(rte_ring_empty(q));
    assert(!rte_ring_full(q));

    rte_ring_destroy(q);
}

static void
basic_mp_test()
{
    int s;
    struct rte_ring *q;
    union {
        void *vp;
        uint64_t v;
    } u;


    q = rte_ring_create(4, 0);

    s = rte_ring_mp_enqueue(q, (void *)10); assert(s == 1);
    s = rte_ring_mp_enqueue(q, (void *)11); assert(s == 1);
    s = rte_ring_mp_enqueue(q, (void *)12); assert(s == 1);

    assert(rte_ring_count(q) == 3);

    s = rte_ring_mp_enqueue(q, (void *)13); assert(s < 0);
    assert(rte_ring_full(q));
    assert(!rte_ring_empty(q));

    s = rte_ring_mc_dequeue(q, &u.vp); assert(s == 1);
    assert(u.v == 10);

    s = rte_ring_mc_dequeue(q, &u.vp); assert(s == 1);
    assert(u.v == 11);

    s = rte_ring_mc_dequeue(q, &u.vp); assert(s == 1);
    assert(u.v == 12);

    assert(rte_ring_count(q) == 0);
    s = rte_ring_mc_dequeue(q, &u.vp); assert(s < 0);

    assert(rte_ring_empty(q));
    assert(!rte_ring_full(q));

    rte_ring_destroy(q);
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

    basic_sp_test();
    basic_mp_test();

    test_desc vrfy = { .prod = vrfy_producer,
                       .cons = vrfy_consumer,
                       .fini = vrfy_finisher,
                       .name = "self-test",
                     };

    mt_test(&vrfy, p, c);

    // set by GNUmakefile when we do "release" builds with
    // optimization on
    test_desc perf = { .prod = perf_producer,
                       .cons = perf_consumer,
                       .fini = perf_finisher,
                       .name = "perf-test",
                     };

    USEARG(perf);
#ifdef __OPTIMIZE__
    mt_test(&perf, p, c);
#endif

    return 0;
}

/* EOF */
