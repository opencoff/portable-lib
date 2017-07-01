/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * dirname.c - portable routine to the shell builtin dirname(1)
 *
 * Copyright (c) 2006 Sudhi Herle <sw at herle.net>
 *
 * License: GPLv2
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#include <string.h>
#include <stdlib.h>


const char *
dirname(char *buf, size_t bsize, const char *p)
{
    size_t n = strlen(p);
    if (bsize < (n+1)) return 0;

    memcpy(buf, p, n);
    buf[n] = 0;

    char *s = buf + n - 1;

    // skip trailing '/'
    for (; s > buf; s--) {
        if (*s != '/') goto found;
    }
    goto slash;

found:
    s[1] = 0;   // remove the last slash.
    for (; s > buf; s--) {
        if (*s == '/') {
            *s = 0;
            return buf;
        }
    }

slash:
    if (buf[0] != '/') buf[0] = '.';
    buf[1] = 0;
    return buf;
}

#ifdef TEST
#include <stdio.h>
#include <limits.h>
#include <assert.h>


struct test
{
    const char *src;
    const char *exp;
};

const struct test T[] = 
{
      { "/", "/" }
    , { "a", "." }
    , { "/a/b", "/a" }
    , { "/a/b/c", "/a/b" }
    , { "a/b/c", "a/b" }
    , { "a/b",   "a" }

    , { "/a/b////", "/a" }
    , { "/a////", "/" }
    , { 0, 0 }
};

#define die(fmt,...) do { \
    fprintf(stderr, fmt, #__VA_ARGS__); \
    exit(1); \
} while (0)


int
main()
{
    const struct test *t = &T[0];
    char p[PATH_MAX];

    for (; t->src; t++) {
        const char * r = dirname(p, sizeof p, t->src);
        if (!r)                     die("%s: exp %s, saw NULL\n", t->src, t->exp);
        if (0 != strcmp(r, t->exp)) die("%s: exp %s, saw %s\n", t->src, t->exp, r);
    }
    return 0;
}

#endif // TEST
