/*
 * test harness for strtou64()
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include "utils/utils.h"

struct test
{
    char    *in;
    uint64_t out;
    int      base;
    int      error;
    char     end;
};
typedef struct test test;


#define _G0(s, base, ch)      {#s, s, base, 0, ch}
#define _G1(s, v, base, ch)   {#s, v, base, 0, ch}
#define _RNG(s, base, ch)     {#s, 0, base, -ERANGE, ch}
static const test T[] = {
      _G0(1234567890, 0, 0)
    , _G0(0755, 0, 0)
    , _G0(0x589, 0, 0)
    , _G1(123xyz, 123, 0, 'x')
    , _G1(18446744073709551615, ~0, 0, 0)
    , _G1(18446744073709551614, ~0-1, 0, 0)
    , _G1(0xyz, 0, 0, 'y')
    , _G1(zzyyyxxx, 0, 0, 'z')
    , _G1(0ggg, 0, 0, 'g')
    , _G1(0799, 07, 0, '9')

    , _RNG(18446744073709551616, 0, '6')
    , _RNG(184467440737095516198, 0, '9')
    , {0, 0, 0, 0, 0}     // always keep at the end
};


int
main()
{
    const test *t = &T[0];

#define die(fmt, ...)   do { \
                            printf(fmt, __VA_ARGS__);   \
                            exit(1);                    \
                        } while(0)

    for (; t->in; t++) {
        const char *end = 0;
        uint64_t v = 0;
        int r = strtou64(t->in, &end, t->base, &v);

        if (r != t->error) die("%s: retval exp %d, saw %d\n", t->in, t->error, r);
        if (end && *end != t->end) die("%s: endptr exp %c, saw %c\n", t->in, t->end, *end);
        if (r == 0 && v != t->out) die("%s: val exp %" PRIu64 ", saw %" PRIu64 "\n", t->in, t->out, v);
    }
    return 0;
}
