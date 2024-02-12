/*
 * MPMC2 Queue Tests
 *
 * (c) 2024 Sudhi Herle <sw-at-herle.net>
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
#include <semaphore.h>

#include "error.h"
#include "utils/cpu.h"
#include "utils/utils.h"
#include "fast/vect.h"
#include "fast/mpmc_bounded_queue.h"


#define QSIZ        65536
#define NITER       1048576


#define _CPUMASK            (~(_U64(0xff) << 56))
#define CPUID(seq)          (0xff & ((seq) >> 56))
#define SEQ(seq)            ((seq) & _CPUMASK)
#define MKSEQ(cpu, seq)     ((_U64(seq) & _CPUMASK) | (_U64(cpu) << 56))

struct qitem {
    uint64_t ts;    // tx timestamp


    // producer seq#. The top bits are the CPU#
    uint64_t seq;
};
typedef struct qitem qitem;

// latency and seq# as measured by receiver
struct lat {
    uint64_t ns;    // diff between tx and rx clocks
    uint64_t seq;   // seq# received
    uint64_t ts;    // rx clock (monotonic)
    uint64_t pad0;
};
typedef struct lat lat;

// Queue of time specs
MPMC_QUEUE_TYPE(pcq, qitem);


// Array of latency structs
VECT_TYPEDEF(latv, lat);


sem_t                Ending;    // Used to signal thread completion

atomic_uint_fast32_t Done;      // Used to signal all consumers
uint32_t             pad1[__cachepad(4)];

atomic_uint_fast32_t Start;     // Used to tell all threads to "go"
uint32_t             pad2[__cachepad(4)];



// Per-Thread context
struct ctx
{
    // Cached copy of the shared queue & global vars
    pcq *q;

    // total CPU cycles for the ENQ or DEQ operation
    // cycles / nelem  will give us time in CPU cycles per ENQ or
    // DEQ operation.
    uint64_t cycles;

    // total nanosecs for the ENQ or DEQ operation
    uint64_t ns;

    // Per CPU Var
    size_t nelem;   // number of elems pushed

    latv lv;        // latency array (used by consumer only)

    atomic_uint_fast32_t *done;
    atomic_uint_fast32_t *start;
    sem_t                *ending;

    int  cpu;       // cpu# on which this thread runs

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
newctx(pcq* q, size_t vsz)
{
    ctx* c = NEWZ(ctx);
    assert(c);

    // Per CPU vars
    if (vsz > 0) {
        VECT_INIT(&c->lv, vsz);
    }

    c->nelem  = 0;

    // Shared, cached vars
    c->q      = q;
    c->done   = &Done;
    c->ending = &Ending;
    c->start  = &Start;

    return c;
}



static void
delctx(ctx* c)
{
    VECT_FINI(&c->lv);
    DEL(c);
}


static char *
speed_desc(char *buf, size_t sz, double spd)
{
    const char *suff = "";

    if (spd >= 1.0e12) {
        spd /= 1.0e12;
        suff = " T";
    } else if (spd >= 1.0e9) {
        spd /= 1.0e9;
        suff = " B";
    } else if (spd >= 1.0e6) {
        spd /= 1.0e6;
        suff = " M";
    }
    snprintf(buf, sz, "%5.2f%s", spd, suff);
    return buf;
}

static void
mt_setup()
{
    atomic_init(&Done,   0);
    atomic_init(&Start,  0);
    sem_init(&Ending, 0, 0);
}

static void
mt_fini()
{
    sem_destroy(&Ending);

    atomic_init(&Done,   0);
    atomic_init(&Start,  0);
}


// Simple busywait.
// usleep() is not granular enough
static void
upause(int n)
{
    volatile int i;
    volatile int j = 9900;
    for (i = 0; i < n; ++i)
    {
        j += i;
    }
}


static void
waitstart(ctx* c)
{
    c->nelem  = 0;
    c->cycles = 0;
    while (!atomic_load(c->start)) {
        upause(100);
    }
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
    uint64_t seq = MKSEQ(c->cpu, 0);
    for (i = 0; i < NITER; i++) {
        qitem qi;

        qi.seq = seq++;
        qi.ts  = timenow();

        uint64_t t0 = sys_cpu_timestamp();
        MPMC_QUEUE_ENQ_WAIT(q, qi);

        c->cycles += sys_cpu_timestamp() - t0;
        c->ns     += timenow() - qi.ts;
        c->nelem++;
    }

    sem_post(c->ending);
    return (void*)0;
}



static inline void
perf_drain(ctx* c)
{
    pcq* q     = c->q;
    latv* v    = &c->lv;

    do {
        qitem qi;
        uint64_t t0 = sys_cpu_timestamp();
        uint64_t n0 = timenow();

        if (!MPMC_QUEUE_DEQ(q, qi)) {
            break;
        }

        uint64_t nn = timenow();
        c->cycles  += sys_cpu_timestamp() - t0;
        c->ns      += nn - n0;
        c->nelem++;

        lat ll = {
            .ns  = nn - qi.ts,
            .seq = qi.seq,
            .ts  = nn,
        };
        VECT_PUSH_BACK(v, ll);
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

    if (x->ns < y->ns)
        return -1;

    if (x->ns > y->ns)
        return +1;

    return 0;
}

// sort func that sorts on seq#
static int
seq_cmp(const void *a, const void *b)
{
    const lat *x = a;
    const lat *y = b;
    
    uint64_t cpu_x = CPUID(x->seq),
             cpu_y = CPUID(y->seq),
             seq_x = SEQ(x->seq),
             seq_y = SEQ(y->seq);

    // first order by cpu#
    if (cpu_x < cpu_y) {
        return -1;
    } else if (cpu_x > cpu_y) {
        return +1;
    }

    // within the same cpu, order by seq
    if (seq_x < seq_y) {
        return -1;
    } else if (seq_x > seq_y) {
        return +1;
    }
    return 0;
}


static void
print_pctile(latv *all, uint64_t tot)
{
    latv pctile;

    // We sort on latency to identify the median and 99th pctile
    VECT_INIT_FROM(&pctile, all);
    VECT_SORT(&pctile,  lat_cmp);

    // Percentiles: 99, 70, 50
    int64_t p99 = (99 * tot) / 100,
            p70 = (70 * tot) / 100,
            p50 = (70 * tot) / 100;

    uint64_t median = 0;

    if (VECT_LEN(&pctile) & 1) {
        int x  = VECT_LEN(&pctile)/2;
        median = VECT_ELEM(&pctile, x).ns;
    } else {
        int a  = VECT_LEN(&pctile)/2;
        int b  = a-1;
        median = (VECT_ELEM(&pctile, a).ns + VECT_ELEM(&pctile, b).ns)/2;
    }

    printf("# Latencies: median %" PRIu64 " ns. Percentiles:\n"
           "#     99th: %" PRIu64 "\n"
           "#     70th: %" PRIu64 "\n"
           "#     50th: %" PRIu64 "\n",
           median,
           VECT_ELEM(&pctile, p99).ns,
           VECT_ELEM(&pctile, p70).ns,
           VECT_ELEM(&pctile, p50).ns);


    VECT_FINI(&pctile);
}


static void
verify_correctness(ctx **pp, int np, latv *all)
{
    size_t i;
    latv seq;

    VECT_INIT_FROM(&seq, all);
    VECT_SORT(&seq, seq_cmp);

    for (i = 1; i < VECT_LEN(&seq); i++) {
        lat *p = &VECT_ELEM(&seq, i-1);
        lat *v = &VECT_ELEM(&seq, i);

        uint64_t pcpu = CPUID(p->seq),
                 pseq = SEQ(p->seq),
                 vcpu = CPUID(v->seq),
                 vseq = SEQ(v->seq);

        assert(pcpu < _U64(np));
        assert(vcpu < _U64(np));

        // Either we're still processing items from the current cpu
        // OR, we are changing to the next one. We should never go
        // back!
        assert((vcpu == pcpu) || (vcpu > pcpu));

        if (vcpu > pcpu) {
            ctx *c = pp[pcpu];

            // we are transitioning to next core. 
            assert(vseq == 0);

            // and the last seq# must be the final count for that cpu
            assert(pseq == (c->nelem-1));
        } else if (vcpu == pcpu) {
            if (vseq != (pseq+1)) {
                fprintf(stderr, "[%zd] cpu %zd; seq %#zx != %#zx (saw %#zx)\n",
                        i, pcpu, vseq, pseq+1, pseq);
            }
        }
    }

    VECT_FINI(&seq);
}


static void
print_latencies(latv *all)
{
    lat* v;

    // we sort on received time/clock - and output the latency
    VECT_SORT(all, ts_cmp);
    VECT_FOR_EACH(all, v) {
        printf("%" PRIu64 "\n", v->ns);
    }

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
    uint64_t ns  = 0;

#define _d(a) ((double)(a))

    char desc[128];
    for (i = 0; i < np; ++i) {
        ctx* cx = pp[i];
        double speed = _d(cx->cycles) / _d(cx->nelem);
        double pns  = _d(cx->ns) / _d(cx->nelem);
        double xops = pns * _d(__Duration);

        speed_desc(desc, sizeof desc, xops);
        printf("#    P %2d on cpu %2d, %zd elem %5.2f ns/enq %5.2f cy/enq %s enq/s\n", i, cx->cpu, cx->nelem, pns, speed, desc);
        tot += cx->nelem;
        cyc += cx->cycles;
        ns  += cx->ns;
    }

    uint64_t tot_prod = tot;
    double cyc_avg = _d(cyc) / _d(tot);
    double ns_avg  = _d(ns) / _d(tot);
    double ops     = _d(__Duration) *  ns_avg;

    speed_desc(desc, sizeof desc, ops);
    printf("# P Total %zd elem, %5.2f ns/enq %5.2f cy/enq %s enq/s\n", tot, ns_avg, cyc_avg, desc);

    latv all;

    VECT_INIT(&all, tot);

    tot = 0;
    cyc = 0;
    ns = 0;
    for (i = 0; i < nc; ++i) {
        ctx* cx = cc[i];
        double speed = _d(cx->cycles) / _d(cx->nelem);
        double cns  = _d(cx->ns) / _d(cx->nelem);
        double xops = cns * _d(__Duration);

        speed_desc(desc, sizeof desc, xops);
        printf("#    C %2d on cpu %2d, %zd elem %5.2f ns/deq %5.2f cy/enq %s deq/s\n", i, cx->cpu, cx->nelem, cns, speed, desc);

        VECT_APPEND_VECT(&all, &cx->lv);
        tot += VECT_LEN(&cx->lv);
        cyc += cx->cycles;
        ns  += cx->ns;
    }

    // we should have received exactly as many elements as were produced
    assert(tot_prod == tot);

    cyc_avg = _d(cyc) / _d(tot);
    ns_avg  = _d(ns) / _d(tot);
    ops     = _d(__Duration) *  ns_avg;

    speed_desc(desc, sizeof desc, ops);
    printf("# C Total %zd elem, %5.2f ns/deq %5.2f cy/deq %s deq/s\n", tot, ns_avg, cyc_avg, desc);

    verify_correctness(pp, np, &all);
    print_pctile(&all, tot);
    print_latencies(&all);

    VECT_FINI(&all);
}


static void
mt_test(test_desc* tt, int np, int nc)
{
    pcq *q = MPMC_QUEUE_NEW(pcq, QSIZ);
    int i, r;
    int nmax = sys_cpu_getavail();
    int cpu  = 0;

    if ((np+nc) > nmax) {
        error(1, 0, "Producers+consumers exceed max CPUs (nmax)\n", nmax);
    }

    ctx*      cc[nc];
    ctx*      pp[np];

    printf("# [%s] %d CPUs; %d P, %d C, QSIZ %d N %d\n", tt->name, nmax, np, nc, QSIZ, NITER);

    mt_setup();

    for (i = 0; i < np; ++i) {

        // Producers don't use the latency queue. So, set it up to
        // be zero.
        ctx * cx = newctx(q, 0);


        pp[i] = cx;
        if ((r = pthread_create(&cx->id, 0, tt->prod, cx)) != 0)
            error(1, r, "Can't create producer thread");

        assert(cpu < nmax);
        cx->cpu = cpu++;
        sys_cpu_set_thread_affinity(cx->id, cx->cpu);
    }

    for (i = 0; i < nc; ++i) {
        ctx* cx = newctx(q, NITER);

        cc[i] = cx;
        if ((r = pthread_create(&cx->id, 0, tt->cons, cx)) != 0)
            error(1, r, "Can't create consumer thread");

        assert(cpu < nmax);
        cx->cpu = cpu++;
        sys_cpu_set_thread_affinity(cx->id, cx->cpu);
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

    // finally dispose of the contexts

    for (i = 0; i < np; i++) {
            delctx(pp[i]);
    }

    for (i = 0; i < nc; i++) {
            delctx(cc[i]);
    }

    mt_fini();
    MPMC_QUEUE_DEL(q);
}


static void
basic_test()
{
    /* Declare a local queue of 4 slots */
    MPMC_QUEUE_TYPE(qq_type, int);

    qq_type *q = MPMC_QUEUE_NEW(qq_type, 4);
    int s = 0;
    char qdesc[256];

    MPMC_QUEUE_DESC(q, qdesc, sizeof qdesc);
    
    printf("# int-queue: %s\n", qdesc);

    s = MPMC_QUEUE_ENQ(q, 10);      assert(s == 1);
    s = MPMC_QUEUE_ENQ(q, 11);      assert(s == 1);
    s = MPMC_QUEUE_ENQ(q, 12);      assert(s == 1);

    MPMC_QUEUE_ENQ_WAIT(q, 13);

    assert(MPMC_QUEUE_SIZE(q) == 4);

    assert(MPMC_QUEUE_SIZE(q) == 4);
    assert(MPMC_QUEUE_FULL_P(q));
    assert(!MPMC_QUEUE_EMPTY_P(q));

    // 5th add should fail.
    s = MPMC_QUEUE_ENQ(q, 14);      assert(s == 0);

    // Until we drain the queue
    int j;

    MPMC_QUEUE_DEQ_WAIT(q, j);      assert(j == 10);
    s = MPMC_QUEUE_DEQ(q, j);       assert(s == 1); assert(j == 11);
    s = MPMC_QUEUE_DEQ(q, j);       assert(s == 1); assert(j == 12);

    assert(MPMC_QUEUE_SIZE(q) == 1);

    s = MPMC_QUEUE_DEQ(q, j);       assert(s == 1); assert(j == 13);

    assert(MPMC_QUEUE_SIZE(q) == 0);
    assert(MPMC_QUEUE_EMPTY_P(q));
    assert(!MPMC_QUEUE_FULL_P(q));

    // and we shouldn't be able to drain an empty queue
    s = MPMC_QUEUE_DEQ(q, j);       assert(s == 0);

    MPMC_QUEUE_DEL(q);
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

    test_desc perf = { .prod = perf_producer,
                       .cons = perf_consumer,
                       .fini = perf_finisher,
                       .name = "perf-test",
                     };

    mt_test(&perf, p, c);

    return 0;
}

/* EOF */
