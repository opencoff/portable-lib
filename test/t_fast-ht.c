/*
 * Test for open addressed, robinhood hash table.
 *
 * (c) 2015 Sudhi Herle <sudhi-at-herle.net>
 */

#include <math.h>
#include <stdio.h>
#include <inttypes.h>

#include "error.h"
#include "utils/utils.h"
#include "utils/arena.h"
#include "fast/vect.h"
#include "utils/hashfunc.h"

#include "utils/fast-ht.h"

#include "ht-common.c"

#define now()   sys_cpu_timestamp()
#define _d(x)   ((double)(x))


extern uint32_t arc4random(void);
extern void     arc4random_buf(void *, size_t);

static uint64_t
find_all(strvect* v, ht* h, int exp)
{
    size_t i;
    word* w;
    uint64_t tot     = 0;
    uint64_t perturb = 0;

    if (!exp) arc4random_buf(&perturb, sizeof perturb);

    VECT_FOR_EACHi(v, i, w) {
        void *x = 0;
        uint64_t t0 = now();
        int r = ht_find(h, w->h + perturb, &x);
        tot += now() - t0;
        if (exp) {
            if (!r) {
                printf("** I-MISS %s\n", w->w);
                continue;
            }
        } else {
            if (r) {
                printf("** I-MISS-X %s\n", w->w);
                continue;
            }
        }

#if 0
        char* z = x;
        assert(z == w->w);
        if (0 != strcmp(z, w->w)) {
            printf("** word %s: exp %p, saw %p\n", w->w, w, x);
        }
#endif
    }

    return tot;
}

static uint64_t
insert_words(strvect* v, ht* h)
{
    word* w;
    uint64_t tot = 0;
    int r;

    VECT_FOR_EACH(v, w) {
        uint64_t t0 = now();
        r = ht_probe(h, w->h, w->w);
        tot += now() - t0;

        assert(!r);
    }
    return tot;
}



// % of buckets occupied
#define fill(h) ((100.0 * _d(h->fill)) / _d(h->n))

// expected nodes/bucket
#define exp_density(h)  (_d(h->nodes) / _d(h->n))

// actual density
#define density(h)  (h->fill ? (_d(h->nodes) / _d(h->fill)) : 0.0)

static void
print_ht(ht* h)
{
    printf("stats:\n"
           "  %" PRIu64 " buckets; %" PRIu64 " nodes, fill %4.2f density %4.2f (exp %4.2f)\n"
           "  max-bags %u, max-nodes-per-bucket %u, splits %u\n",
           h->n, h->nodes, fill(h), density(h), exp_density(h),
           h->bagmax, h->maxn, h->splits);
}


#ifdef __MAKE_OPTIMIZE__    // set by GNUmakefile.portablelib.mk

static uint64_t
del_all(strvect* v, ht* h, int exp)
{
    word* w;
    uint64_t tot = 0;
    int r;

    VECT_FOR_EACH(v, w) {
        void *x = 0;
        uint64_t t0 = now();
        r = ht_remove(h, w->h, &x);
        tot += now() - t0;

        if (r != exp) {
            printf("** DEL MISS %s\n", w->w);
            continue;
        }
    }
    return tot;
}
static void
perf_test(strvect* v, size_t Niters)
{
    uint64_t cyi = 0,
             cyd = 0,
             cyx = 0,
             cyy = 0,
             cys = 0;

    size_t i;
    size_t n = VECT_SIZE(v);
    size_t nlog2 = (size_t) log2(n)+1;
    ht _h;
    ht* h = &_h;
    uint64_t ti  = 0,
             td  = 0,
             tx  = 0,
             ty  = 0,
             ts  = 0;
    uint64_t t0;

    printf("--- Perf Test --\n");

    VECT_SHUFFLE(v, arc4random);
    for (i = 0; i < Niters; ++i) {
        ht_init(h, nlog2);
        t0    = timenow();
        cyi  += insert_words(v, h);
        ti   += timenow() - t0;


        t0    = timenow();
        cys  += find_all(v, h, 1);
        ts   += timenow() - t0;

        t0    = timenow();
        cyy  += find_all(v, h, 0);
        ty   += timenow() - t0;

        t0    = timenow();
        cyd  += del_all(v, h, 1);
        td   += timenow() - t0;


        // Finally, delete it again -- this will give us delete for
        // non existing
        t0    = timenow();
        cyx  += del_all(v, h, 0);
        tx   += timenow() - t0;

        if (i == 0) print_ht(h);

        ht_fini(h);
    }

    double tot    = _d(n) * _d(Niters);

    double ispd   = tot / _d(ti);
    double dspd   = tot / _d(td);
    double xspd   = tot / _d(tx);
    double yspd   = tot / _d(ty);
    double sspd   = tot / _d(ts);

    double ci     = _d(cyi)  / tot;
    double cd     = _d(cyd)  / tot;
    double cx     = _d(cyx)  / tot;
    double cy     = _d(cyy)  / tot;
    double cs     = _d(cys)  / tot;

    
    printf("Add-empty:      %4.1f cy/add   %8.2f M/sec\n"
           "Find-existing:  %4.1f cy/find  %8.2f M/sec\n"
           "Find-non-exist: %4.1f cy/find  %8.2f M/sec\n"
           "Del-existing:   %4.1f cy/find  %8.2f M/sec\n"
           "Del-non-exist:  %4.1f cy/find  %8.2f M/sec\n",
           ci, ispd,
           cs, sspd,
           cy, yspd,
           cd, dspd,
           cx, xspd
           );

}
#endif // __MAKE_OPTIMIZE__


int
main(int argc, char** argv)
{
    program_name  = argv[0];
    char* args[3] = { argv[0], "-", 0 };
    arena_t a;
    strvect v;
    int i;

    if (argc < 2) {
        argv = args;
        argc = 2;
    }

    VECT_INIT(&v, 256*1024);
    arena_new(&a, 1048576);

    for (i = 1; i < argc; ++i) {
        read_words(&v, a, argv[i]);
    }


    ht _h;
    ht *h = &_h;

    ht_init(h, 2);

    insert_words(&v, h);

    //VECT_SHUFFLE(&v, arc4random);

    find_all(&v, h, 1);
    print_ht(h);
    ht_fini(h);


#ifdef __MAKE_OPTIMIZE__

#define NITER   20
    perf_test(&v, NITER);

#endif // __OPTIMIZE__

    VECT_FINI(&v);

    arena_delete(a);
    return 0;
}

/* EOF */
