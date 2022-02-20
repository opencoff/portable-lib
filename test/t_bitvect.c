#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "fast/bitset.h"

extern uint32_t arc4random_uniform(uint32_t);

int
main()
{
    bitset *bb = bitset_new(100);

    assert(bb->n == 2);
    for (int i = 0; i < 100; ++i) {
        bitset_set(bb, i);
        assert(bitset_isset(bb, i));

        bitset_clr(bb, i);
        assert(!bitset_isset(bb, i));
    }

    bitset *aa = bitset_alloca(75);
    assert(aa->n == 2);

    for (int i = 0; i < 75; i++) {
        size_t n = arc4random_uniform(75);
        size_t m = arc4random_uniform(~0);

        bitset_set(aa, n);
        if (m & 1) bitset_set(bb, (m % 75));
    }

    // keep a copy of 'a' around
    bitset *xx = bitset_dup_alloca(aa);

    // verify xx = aa
    assert(aa->n == xx->n);
    for (size_t j = 0; j < aa->n; j++) assert(aa->w[j] == xx->w[j]);

    for (int i = 0; i < 75; i++) {
        assert(bitset_value(aa, i) == bitset_value(xx, i));
    }


    bitset_or(aa, bb);

    // verify aa |= b
    for (int i = 0; i < 75; i++) {
        if (bitset_isset(bb, i)) assert(bitset_isset(aa, i));
        if (bitset_isset(xx, i)) assert(bitset_isset(aa, i));
    }

    bitset_del(bb);
    return 0;
}

