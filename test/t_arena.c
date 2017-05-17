/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * t_arena.c - simple test harness for lifetime allocator
 *
 * Copyright (c) 2005 Sudhi Herle <sw@herle.net>
 *
 * Licensing Terms: (See LICENSE.txt for details). In short:
 *    1. Free for use in non-commercial and open-source projects.
 *    2. If you wish to use this software in commercial projects,
 *       please contact me for licensing terms.
 *    3. By using this software in commercial OR non-commercial
 *       projects, you agree to send me any and all changes, bug-fixes
 *       or enhancements you make to this software.
 * 
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#include <stdio.h>
#include <errno.h>
#include <stdint.h>

#include "utils/utils.h"
#include "utils/arena.h"
#include "utils/xorshift-rand.h"
#include "error.h"

static void perf_test(void);

int
main()
{
    perf_test();

    return 0;
}

/*
 * Generate a random size between 'a' and 'b' inclusive.
 */
static size_t
randsize(xs1024star* xs, size_t a, size_t b)
{
    uint64_t max = b - a + 1;
    size_t n     = xs1024star_u64(xs) % max;

    return a + n;
}


static void
perf_test()
{
    const int N = 10000;
    arena_t a;
    int i;
    
    int r = arena_new(&a, 0);
    if (r < 0) error(1, -r, "Cannot allocate arena");

    uint64_t t0,
             sum = 0;

    xs1024star xs;

    xs1024star_init(&xs, 0, 0);

    for (i = 0; i < N; i++)
    {
        size_t sz = randsize(&xs, 16, 32769);

        t0 = sys_cpu_timestamp();
        void * volatile p = arena_alloc(a, sz);

        (void)p;
        sum += sys_cpu_timestamp() - t0;
    }
    
    arena_delete(a);

#define _d(x)   ((double)x)
    double speed = _d(sum) / _d(N);   // cycles/alloc
    printf("%d allocs; %6.5f cy/alloc\n", N, speed);
}

