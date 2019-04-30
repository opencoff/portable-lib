/*
 * Common code for hash tests
 */
#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include "utils/utils.h"
#include "utils/arena.h"
#include "fast/vect.h"
#include "utils/hashfunc.h"

/*
 * Input word.
 */
struct word
{
    char* w;
    size_t n;
    uint64_t h;
};
typedef struct word word;

VECT_TYPEDEF(strvect, word);

static void
read_words(strvect* v, arena_t a, const char* filename)
{
    unsigned char buf[256];
    int n;
    FILE* fp = stdin;
    uint64_t salt = 0;

    if (0 != strcmp("-", filename)) {
        fp = fopen(filename, "r");
        if (!fp) error(1, errno, "Can't open %s", filename);
    }

    while ((n = freadline(fp, buf, sizeof buf)) > 0) {
        if (n < 4) continue;

        unsigned char *x;
        for (x=buf; *x; x++) {
            if (isspace(*x)) {
                *x = 0;
                n  = x-buf;
                break;
            }
        }

        char* z = (char *)arena_alloc(a, n+1);
        memcpy(z, buf, n+1);

        word w  = { .w = z,
                    .n = (size_t)n,
                    .h = fasthash64(z, n, salt)
                  };
        VECT_PUSH_BACK(v, w);
    }

    if (fp != stdin) fclose(fp);
}

