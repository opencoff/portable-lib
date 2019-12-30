/*
 * Test harness for Xorfilter
 * Xorfilters are better than Bloom filters for membership queries.
 */

#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include "error.h"
#include "utils/utils.h"
#include "utils/xorfilter.h"

extern void arc4random_buf(void *, size_t);

static uint64_t
rand64()
{
    uint64_t z = 0;
    arc4random_buf(&z, sizeof z);
    return z;
}

static void
basictest(int is16)
{
    size_t   n = 10000;
    uint64_t *keys = NEWA(uint64_t, n);

    for (size_t i = 0; i < n; i++) keys[i] = i;

    Xorfilter* x = is16 ? Xorfilter_new16(keys, n) : Xorfilter_new8(keys, n);
    assert(x);

    for (size_t i = 0; i < n; i++) {
        assert(Xorfilter_contains(x, keys[i]));
    }

    uint64_t rands = 0;
    uint64_t tries = 10000000;
    for (uint64_t i = 0; i < tries; i++) {
        uint64_t v = rand64();
        if (Xorfilter_contains(x, v)) {
            if (v > n) rands++;
        }
    }

    const char *pref = is16 ? "Xor16" : "Xor8 ";
    printf("%s FP Prob (est): %8.7f %4.2f bits/entry\n",
           pref, ((double)rands) / ((double)tries), Xorfilter_bpe(x));

    Xorfilter_delete(x);
    DEL(keys);
}

#define now()       sys_cpu_timestamp()
#define _d(x)       ((double)(x))

static void
perftest(int is16)
{
    const size_t trials = 100;
    const size_t n = 100000;
    uint64_t *keys = NEWA(uint64_t, n);

    for (size_t i = 0; i < n; i++) keys[i] = rand64();
    Xorfilter *x;
    const char *pref;

    uint64_t t_create = 0,
             t_find = 0;

    if (is16) {
        pref = "Xor16";
        for (size_t k = 0; k < trials; k++) {
            uint64_t t0 = now();

            x = Xorfilter_new16(keys, n);
            t_create += now() - t0;
            assert(x);
            Xorfilter_delete(x);
        }

        x = Xorfilter_new16(keys, n);
    } else {
        pref = "Xor8 ";
        for (size_t k = 0; k < trials; k++) {
            uint64_t t0 = now();

            x = Xorfilter_new8(keys, n);
            t_create += now() - t0;
            assert(x);
            Xorfilter_delete(x);
        }
        x = Xorfilter_new8(keys, n);
    }

    // shuffle all the items
    // fisher-yates shuffle
    for (size_t i = n-1; i > 0; i--) {
        size_t j = rand64() % (i+1);
        uint64_t e = keys[i];
        keys[i]    = keys[j];
        keys[j]    = e;
    }

    // now look for items
    for (size_t k = 0; k < trials; k++) {
        for (size_t i = 0; i < n; i++) {
            uint64_t t0 = now();
            int ok = Xorfilter_contains(x, keys[i]);

            t_find += now() - t0;
            assert(ok);
        }
    }

    Xorfilter_delete(x);

    double cr = (_d(t_create) / _d(trials)) / _d(n);
    double ff = (_d(t_find) / _d(trials)) / _d(n);

    printf("%s %lu items: create %6.3f cy/elem, find %6.3f cy/elem\n",
            pref, n, cr, ff);
}

int
main()
{
    basictest(0);
    basictest(1);

    perftest(0);
    perftest(1);
}
