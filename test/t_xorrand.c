/*
 * Xorshift+ and Xoroshiro PRNG Simple Tests
 *
 * (c) 2015 Sudhi Herle
 * License: GPLv2
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "error.h"
#include "utils/xorshift-rand.h"
#include "utils/xoroshiro.h"



int
main(int argc, char* argv[])
{
    size_t i, n = 100;

    if (argc > 1) {
        int x = atoi(argv[1]);
        if (x <= 0)
            error(1, 0, "Ignoring invalid count '%s'", argv[1]);

        n = x;
    }


    xs64star xs;
    xs128plus xp;
    xs1024star xss;

    xs64star_init(&xs, 0);
    xs128plus_init(&xp, 0);
    xs1024star_init(&xss, 0);

    for(i = 0; i < n; ++i) {
        uint64_t v0 = xs64star_u64(&xs);
        uint64_t v1 = xs128plus_u64(&xp);
        uint64_t v2 = xs1024star_u64(&xss);

        printf("%#.16" PRIx64 " %#.16" PRIx64 " %#.16" PRIx64 "\n", v0, v1, v2);
    }

    printf("-- xoroshiro128+ output --\n");

    xoro128plus s;
    xoro128plus_init(&s, 0);

    for(i = 0; i < n; i++) {
        uint64_t z = xoro128plus_u64(&s);
        printf("%#.16" PRIx64 "\n", z);
    }

    return 0;
}

/* EOF */
