/*
 * Test harness for Xorfilter
 * Xorfilters are better than Bloom filters for membership queries.
 */

#include <stdio.h>
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

    const char *pref = is16 ? "Xor16" : "Xor8";
    printf("%s FP Prob (est): %8.7f\n"
           "%s Bits/entry:    %4.2f\n", pref, ((double)rands) / ((double)tries),
           pref, Xorfilter_bpe(x));

    Xorfilter_delete(x);
    DEL(keys);
}


int
main()
{
    basictest(0);
    basictest(1);
}
