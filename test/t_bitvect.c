#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "fast/bitset.h"


int
main()
{
    bitset *bb = BITSET_NEW(100);

    int i;
    for (i = 0; i < 100; ++i) {
        BITSET_SET(bb, i);
        assert(BITSET_IS_SET(bb, i));

        BITSET_CLR(bb, i);
        assert(!BITSET_IS_SET(bb, i));
    }

    return 0;
}

