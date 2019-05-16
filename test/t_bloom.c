/*
 * Test for bloom filters.
 *
 * (c) 2015 Sudhi Herle <sudhi-at-herle.net>
 */

#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <inttypes.h>

#include "error.h"
#include "utils/utils.h"
#include "utils/bloom.h"
#include "utils/arena.h"
#include "fast/vect.h"
#include "utils/hashfunc.h"


extern uint32_t arc4random(void);

/*
 * Input word.
 */
struct word
{
    char* w;
    size_t n;
    uint64_t h;
};
typedef struct word word;

VECT_TYPEDEF(strvect, word);


/*
 * Accumulate false-positives, etc.
 */
struct result
{
    uint64_t   fp,  // false positive
               fn,  // false negative
               n;
};
typedef struct result result;


#define now()       sys_cpu_timestamp()
#define _d(x)       ((double)(x))

#define NITER       32


// Score a test result
static void
score(int actual, int exp, result* rr, word* w)
{
    (void)w;

    if (exp) {
        if (!actual) {
            rr->fn++;   // false negative
            //printf("** False-Negative: %s %llx\n", w->w, w->h);
        }
    } else {
        if (actual) rr->fp++;   // false positive
    }
    rr->n++;
}

static void
print_results(const char* prefix, result* rr, Bloom* b)
{
    double fprate     = _d(rr->fp) / _d(rr->n);
    const char* suff0 = fprate > b->e ? "** ERR TOO HIGH **" : "";
    const char* suff1 = rr->fn > 0 ? "** ERR FALSE NEG**" : "";

    printf("    -- %s %s test results --\n"
           "    False neg: %" PRIu64 " %s\n"
           "    False pos: %" PRIu64 " %5.4f %s\n",
           b->name, prefix,
           rr->fn, suff1,
           rr->fp, fprate, suff0);
}


static void
read_words(strvect* v, arena_t a, const char* filename)
{
    unsigned char buf[1024];
    int n;
    FILE* fp = stdin;

    if (0 != strcmp("-", filename)) {
        fp = fopen(filename, "r");
        if (!fp) error(1, errno, "Can't open %s", filename);
    }

    while ((n = freadline(fp, buf, sizeof buf)) > 0) {
        if (n < 4) continue;

        // split on white space.
        unsigned char *x;
        for (x=buf; *x; x++) {
            if (isspace(*x)) {
                *x = 0;
                n  = x-buf;
                break;
            }
        }

        char* z = (char *)arena_alloc(a, n+1);
        memcpy(z, buf, n+1);

        word w  = { .w = z,
                    .n = n,
                    .h = fasthash64(z, n, 0) };
        VECT_PUSH_BACK(v, w);
    }

    printf("%s: %zu entries\n", filename, VECT_LEN(v));
    if (fp != stdin) fclose(fp);
}

static uint64_t
find_all(strvect* v, Bloom* b, int show_results)
{
    size_t i;
    word* w;
    uint64_t tot = 0;
    result rr = { 0, 0, 0};

    VECT_FOR_EACHi(v, i, w) {
        uint64_t t0 = now();
        int r = Bloom_find(b, w->h);
        tot += now() - t0;
        score(r, 1, &rr, w);
    }

    if (show_results) print_results("find-all", &rr, b);
    return tot;
}

static uint64_t
insert_words(strvect* v, Bloom* b)
{
    word* w;
    uint64_t tot = 0;

    VECT_FOR_EACH(v, w) {
        uint64_t t0 = now();
        Bloom_probe(b, w->h);
        tot += now() - t0;
    }
    return tot;
}


static void
perf_test(strvect* v, size_t Niters, int counting, int scalable)
{
    Bloom _b;
    uint64_t tins  = 0,
             tsrch = 0;
    size_t i;
    size_t n = VECT_LEN(v);
    Bloom *b;

    if (counting) {
        for (i = 0; i < Niters; ++i) {
            b      = Counting_bloom_init(&_b, n, 0.005);
            tins  += insert_words(v, b);
            tsrch += find_all(v, b, 0);
            Bloom_fini(b);
        }
    } else {
        if (scalable)   n /= 4;
        for (i = 0; i < Niters; ++i) {
            b      = Standard_bloom_init(&_b, n, 0.005, scalable);
            tins  += insert_words(v, b);
            tsrch += find_all(v, b, 0);
            Bloom_fini(b);
        }
    }

    double  ins  = (_d(tins)  / _d(Niters)) / _d(n);
    double  srch = (_d(tsrch) / _d(Niters)) / _d(n);

    const char* pref = scalable ? "scalable-" : "";
    printf("    Performance:  %s%s %8.4f cy/add %8.4f cy/search\n", pref,
            counting ? "counting-bloom" : "standard-bloom", ins, srch);
}



/*
 * Test false positive rate.
 *
 * We do this by reversing every alternate word and looking for it
 * in the filter.
 */
static void
false_positive_test(strvect* v, Bloom* b)
{
    word* w;
    result rr = { 0, 0, 0};
    size_t i;
    int r;

    VECT_FOR_EACHi(v, i, w) {
        if (0 == (i % 2))   Bloom_probe(b, w->h);
    }

    VECT_FOR_EACHi(v, i, w) {
        if (0 != (i % 2)) {
            r = Bloom_find(b, w->h);
            score(r, 0, &rr, w);
        }
    }
    print_results("false-positive", &rr, b);
}


static void
delete_test(strvect* v, Bloom* b)
{
    size_t i;
    result rr = { 0, 0, 0};

    for (i = 1; i < VECT_LEN(v); i += 2) {
        word* w = &VECT_ELEM(v, i);
        int   r = Bloom_remove(b, w->h);
        if (!r) error(1, 0, "Element %d: %s -- delete error", i, w->w);
    }

    /* Now, query the deleted ones.
     *
     * False Positive: An element known to NOT be in the table -
     *                 but the bloom-filter says it is ("possibly in").
     * False Negative: An element known to be in the table, but the
     *                 filter says it is NOT ("definitely not").
     */
    for (i = 1; i < VECT_LEN(v); i += 2) {
        int   r;
        word* w = &VECT_ELEM(v, i);
        word* x = &VECT_ELEM(v, i-1);


        // word[i-1] MUST be in the table.
        r = Bloom_find(b, x->h);
        score(r, 1, &rr, x);
        //if (!r) error(1, 0, "Can't find element %d: %s", i-1, x->w);

        /*
         * We deleted it in the previous loop. So, we shouldn't be
         * finding it!
         */
        r = Bloom_find(b, w->h);
        score(r, 0, &rr, w);
    }

    print_results("delete-test", &rr, b);
}



/*
 * Marshalling/Unmarshalling tests
 */
static void
marshal_tests(Bloom* b, strvect * v, const char *desc)
{
    const char *fname = "/tmp/c.dat";
    Bloom *ub = 0;

    printf("    -- %s-bloom: Marshal/Unmarshal tests --\n", desc);

    printf("    Marshal: ");
    int r = Bloom_marshal(b, fname);
    assert(r == 0);

    printf("ok\n    Unmarshal unmapped: ");
    r = Bloom_unmarshal(&ub, fname, 0);
    assert(r == 0);
    assert(ub);

    assert(Bloom_eq(b, ub));

    // Verify that all the elements are present.
    find_all(v, b, 0);
    Bloom_fini(ub); ub = 0;

    printf("ok\n    Unmarshal mem-mapped: ");

    // Now with mmap'd data
    r = Bloom_unmarshal(&ub, fname, BLOOM_BITMAP_MMAP);
    assert(r == 0);
    assert(ub);

    assert(Bloom_eq(b, ub));

    // Verify that all the elements are present.
    find_all(v, b, 0);
    Bloom_fini(ub);

    printf("ok\n");
}




static void
counting_tests(strvect* v)
{
    char buf[4096];
    Bloom _b;
    size_t n = VECT_LEN(v);
    Bloom* b;

    printf("Counting-Bloom-Tests:\n");

    b = Counting_bloom_init(&_b, n, 0.005);
    insert_words(v, b);
    VECT_SHUFFLE(v, arc4random);
    find_all(v, b, 1);

    printf("    %s\n", Bloom_desc(b, buf, sizeof buf));
    delete_test(v, b);
    Bloom_fini(b);

    b = Counting_bloom_init(&_b, n, 0.005);
    false_positive_test(v, b);
    Bloom_fini(b);

    b = Counting_bloom_init(&_b, n, 0.005);
    insert_words(v, b);
    marshal_tests(b, v, "Counting");
    Bloom_fini(b);

    perf_test(v, NITER, 1, 0);
}


static void
quick_tests(strvect* v, int scalable)
{
    char desc[64] = "";
    char buf[8192];
    Bloom _b;
    size_t n = VECT_LEN(v);
    Bloom *b;

    if (scalable) {
        n /= 4;
        snprintf(desc, sizeof desc, "Scalable");
    } else {
        snprintf(desc, sizeof desc, "Standard");
    }

    printf("%s-Bloom-Tests:\n", desc);

    b = Standard_bloom_init(&_b, n, 0.005, scalable);
    insert_words(v, b);
    VECT_SHUFFLE(v, arc4random);
    find_all(v, b, 1);

    printf("    %s\n", Bloom_desc(b, buf, sizeof buf));
    Bloom_fini(b);

    b = Standard_bloom_init(&_b, n, 0.005, scalable);
    false_positive_test(v, b);
    Bloom_fini(b);

    b = Standard_bloom_init(&_b, n, 0.005, scalable);
    insert_words(v, b);
    marshal_tests(b, v, desc);
    Bloom_fini(b);

    perf_test(v, NITER, 0, scalable);
}




int
main(int argc, char* argv[])
{
    program_name   = argv[0];
    char* filename = argc > 1 ? argv[1] : "/usr/share/dict/words";
    arena_t a;
    strvect v;

    VECT_INIT(&v, 256*1024);
    arena_new(&a, 1048576);

    read_words(&v, a, filename);

    counting_tests(&v);

    quick_tests(&v, 0);
    quick_tests(&v, 1);

    VECT_FINI(&v);
    arena_delete(a);
    return 0;
}

/* EOF */
