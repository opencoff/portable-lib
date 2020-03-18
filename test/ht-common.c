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

extern void arc4random_buf(void *, size_t);

static uint64_t
rand64()
{
    uint64_t z = 0;
    arc4random_buf(&z, sizeof z);
    return z;
}

static void
read_words(strvect* v, arena_t a, const char* filename)
{
    unsigned char buf[1024];
    int n;
    FILE* fp = stdin;
    uint64_t salt = rand64();

    if (0 != strcmp("-", filename)) {
        fp = fopen(filename, "r");
        if (!fp) error(1, errno, "Can't open %s", filename);
    }

    int j = 0;
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
        j++;
    }

    printf("%s: Read %d entries ..\n", filename, j);
    if (fp != stdin) fclose(fp);
}

