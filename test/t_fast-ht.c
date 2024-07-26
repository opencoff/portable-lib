/*
 * Test for cache-friendly, super-fast hash table.
 *
 * (c) 2015 Sudhi Herle <sudhi-at-herle.net>
 */

#include <math.h>
#include <stdio.h>
#include <inttypes.h>

#include "error.h"
#include "getopt_long.h"
#include "utils/utils.h"
#include "utils/arena.h"
#include "fast/vect.h"
#include "utils/hashfunc.h"

#include "utils/fast-ht.h"

#include "ht-common.c"

#define now()   sys_cpu_timestamp()
#define _d(x)   ((double)(x))

static const struct option Long_options[] =
{
      {"help",                            no_argument,       0, 300}
    , {"machine-output",                  no_argument,       0, 302}

    , {0, 0, 0, 0}
};

static const char Short_options[] = "hm";

static int Machine_output = 0;

extern uint32_t arc4random(void);
extern void     arc4random_buf(void *, size_t);


static uint64_t
find_all(strvect* v, ht* h, int exp)
{
    size_t i;
    word* w;
    uint64_t tot     = 0;
    uint64_t perturb = !exp;

    VECT_FOR_EACHi(v, i, w) {
        void *x = 0;
        uint64_t t0 = now();
        int r = ht_find(h, w->h >> perturb, &x);
        tot += now() - t0;
        if (exp) {
            if (!r) {
                printf("** expected to find %s\n", w->w);
                continue;
            }
        } else {
            if (r) {
                printf("** didn't expect to find %s\n", w->w);
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

    VECT_FOR_EACH(v, w) {
        uint64_t t0 = now();
        void *r = ht_probe(h, w->h, w->w);
        tot += now() - t0;

        if (r) {
            printf("# Duplicate %s\n", w->w);
        }
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
    if (Machine_output) return;

    printf("stats: %d items/bag; %" PRIu64 "%% max fill\n"
           "  %" PRIu64 " buckets; %" PRIu64 " items, fill %4.2f%% density %4.2f (exp %4.2f)\n"
           "  max-bags %u, max-items-per-bucket %u, splits %u\n",
           FASTHT_BAGSZ, h->maxfill,
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
    uint64_t perturb = !exp;

    VECT_FOR_EACH(v, w) {
        void *x = 0;
        uint64_t t0 = now();
        r = ht_remove(h, w->h >> perturb, &x);
        tot += now() - t0;

        if (exp) {
            if (!r) {
                printf("** expected to del %s\n", w->w);
                continue;
            }
        } else {
            if (r) {
                printf("** didn't expect to del %s\n", w->w);
                continue;
            }
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
    size_t n = VECT_LEN(v);
    ht _h;
    ht* h = &_h;
    uint64_t ti  = 0,
             td  = 0,
             tx  = 0,
             ty  = 0,
             ts  = 0;
    uint64_t t0;

    if (!Machine_output)
        printf("--- Perf Test [%zu iters] --\n", Niters);

    VECT_SHUFFLE(v, arc4random);
    for (i = 0; i < Niters; ++i) {
        ht_init(h, n*2, 85);
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

        // Re-populate the table so we can probe for non-existent
        // entries
        insert_words(v, h);

        // delete it again
        t0    = timenow();
        cyx  += del_all(v, h, 0);
        tx   += timenow() - t0;

        if (i == 0) print_ht(h);

        ht_fini(h);
    }

    double tot    = _d(n) * _d(Niters);

    // this captures speed of operation (ns/op)
    double ispd = _d(ti) / tot,
           dspd = _d(td) / tot,
           xspd = _d(tx) / tot,
           yspd = _d(ty) / tot,
           sspd = _d(ts) / tot;

    // timenow() returns time in nanoseconds. So, to get in units of
    // "Million ops/sec", we multiply by 1000.
    double irate  = 1000 * (tot / _d(ti)),
           drate  = 1000 * (tot / _d(td)),
           xrate  = 1000 * (tot / _d(tx)),
           yrate  = 1000 * (tot / _d(ty)),
           srate  = 1000 * (tot / _d(ts));

    double ci = _d(cyi)  / tot,
           cd = _d(cyd)  / tot,
           cx = _d(cyx)  / tot,
           cy = _d(cyy)  / tot,
           cs = _d(cys)  / tot;

    if (Machine_output) {
        printf("add %4.1f cy %4.2f M ops/s %4.2f ns/op,"
               "find %4.1f %4.2f M ops/s cy %4.2f ns/op,"
               "findx %4.1f cy %4.2f M ops/s %4.2f ns/op,"
               "del %4.1f %4.2f M ops/s cy %4.2f ns/op,"
               "delx %4.1f cy %4.2f M ops/s %4.2f ns/op\n",
               ci, irate, ispd,
               cs, srate, sspd,
               cy, yrate, yspd,
               cd, drate, dspd,
               cx, xrate, xspd);
    } else {
        printf("Add-empty:      %4.1f cy/add   %4.2f M/s %8.2f ns/op\n"
               "Find-existing:  %4.1f cy/find  %4.2f M/s %8.2f ns/op\n"
               "Find-non-exist: %4.1f cy/find  %4.2f M/s %8.2f ns/op\n"
               "Del-existing:   %4.1f cy/find  %4.2f M/s %8.2f ns/op\n"
               "Del-non-exist:  %4.1f cy/find  %4.2f M/s %8.2f ns/op\n",
               ci, irate, ispd,
               cs, srate, sspd,
               cy, yrate, yspd,
               cd, drate, dspd,
               cx, xrate, xspd
               );
    }

}
#endif // __MAKE_OPTIMIZE__


int
main(int argc, char** argv)
{
    program_name  = argv[0];
    arena_t a;
    strvect v;
    int i;
    int c;

    while ((c = getopt_long(argc, argv, Short_options, Long_options, 0)) != EOF) {
        switch (c) {
        case 300:  /* help */
        case 'h':  /* help */
            printf("Usage: %s [--machine-output|-m] [inputfile ...]\n", program_name);
            exit(0);
            break;

        case 302:  /* machine-output */
        case 'm':
            Machine_output = 1;
            break;


        default:
            die("Unknown option '%c'", c);
            break;
        }
    }

    argv = &argv[optind];
    argc = argc - optind;

    if (argc < 1) {
        static char *args[] = {"-", 0};
        argv = args;
        argc = 1;
    }

    VECT_INIT(&v, 256*1024);
    arena_new(&a, 1048576);

    for (i = 0; i < argc; ++i) {
        read_words(&v, a, argv[i]);
    }

    if (!Machine_output) printf("Total input: %zu words\n", VECT_LEN(&v));

    ht _h;
    ht *h = &_h;

    ht_init(h, 128, 90);

    insert_words(&v, h);

    //VECT_SHUFFLE(&v, arc4random);

    find_all(&v, h, 1);
    print_ht(h);
    ht_fini(h);


#ifdef __MAKE_OPTIMIZE__

#define NITER   4
    perf_test(&v, NITER);

#endif // __OPTIMIZE__

    VECT_FINI(&v);

    arena_delete(a);
    return 0;
}

/* EOF */
