/*
 * Test harness for str2hex()
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "error.h"
#include "utils/xorshift-rand.h"

extern ssize_t str2hex(uint8_t*, size_t, const char*);


#define MAXHEX      2048
struct foo
{
    char    s[1+(MAXHEX*2)];
    uint8_t h[MAXHEX];

    size_t sn;
    size_t hn;

    xs128plus xs;
};
typedef struct foo foo;

static void
append_ch(foo * f, int c)
{
    char b[16];
    snprintf(b, sizeof b, "%2.2x", c & 0xff);

    f->s[f->sn++] = b[0];
    f->s[f->sn++] = b[1];
    f->s[f->sn]   = 0;

    f->h[f->hn++] = c & 0xff;
}

#if 0
static void
append_nibble(foo *f, int c)
{
    char b[8];

    snprintf(b, sizeof b, "%x", c & 0xf);
    f->s[f->sn++] = b[0];
    f->h[f->hn++] = c & 0xf;
}
#endif


// Generate random strings of length < 255
static void
randstr(foo* f)
{
    char b[16];
    size_t n, m;
    size_t i;

    memset(f->s, 0, sizeof f->s);
    memset(f->h, 0, sizeof f->h);

    do {
        m = xs128plus_u64(&f->xs) % (MAXHEX * 2);
    } while (m == 0);

    // Divide by 2 - so we will occasionally get nibbles
    n = m / 2;
    m = m % 2;

    f->sn = f->hn = 0;
    for (i = 0; i < n; ++i) {
        uint8_t c = xs128plus_u64(&f->xs) & 0xff;

        snprintf(b, sizeof b, "%2.2x", c);
        f->s[f->sn++] = b[0];
        f->s[f->sn++] = b[1];

        f->h[f->hn++] = c;
    }

    if (m) {
        uint8_t c = xs128plus_u64(&f->xs) & 0xff;
        snprintf(b, sizeof b, "%x", c & 0xf);
        f->s[f->sn++] = b[0];
        f->h[f->hn++] = (c & 0xf) << 4;
    }

    f->s[f->sn]   = 0;
}


static void
test0()
{
    foo _f;
    foo *f = &_f;
    int i;
    uint8_t hex[MAXHEX];
    size_t n;

    f->sn = f->hn = 0;
    for (i = 0; i < 16; i++) {
        append_ch(f, i);

        n = str2hex(hex, sizeof hex, f->s);
        if (n != f->hn) error(1, 0, "%s: Expected %d, saw %d", f->s, f->hn, n);

        if (0 != memcmp(hex, f->h, n)) error(1, 0, "%s: content mismatch", f->s);
    }

    xs128plus_init(&f->xs, 0);
    for (i = 0; i < 1000; i++) {
        randstr(f);
        n = str2hex(hex, sizeof hex, f->s);
        if (n != f->hn) error(1, 0, "%d: %s: Expected %d, saw %d", i, f->s, f->hn, n);

        if (0 != memcmp(hex, f->h, n)) error(1, 0, "%d: %s: content mismatch", i, f->s);
    }

    /*
     * Finally, read anything in stdin and write to stdout
     */

}



int
main()
{
    test0();
    return 0;
}
