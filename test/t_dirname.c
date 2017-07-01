/* 
 * Test harness for dirname()
 */
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>


extern const char *dirname(char *buf, size_t bsize, const char *path);

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

#define die(fmt, ...) do { \
    fprintf(stderr, fmt, __VA_ARGS__); \
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

