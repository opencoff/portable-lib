/*
 * Test harness for Xorfilter with user-defined input
 * Xorfilters are better than Bloom filters for membership queries.
 */

#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <unistd.h>

#include "getopt_long.h"
#include "fast/vect.h"
#include "error.h"
#include "utils/utils.h"
#include "utils/xorfilter.h"

extern void arc4random_buf(void *, size_t);

#include "ht-common.c"

static const struct option Long_options[] =
{
      {"help",                            no_argument,       0, 300}
    , {"machine-output",                  no_argument,       0, 302}

    , {0, 0, 0, 0}
};

static const char Short_options[] = "hm";

static int Machine_output = 0;


int
main(int argc, char** argv)
{
    program_name  = argv[0];
    arena_t a;
    strvect v;
    int c;

    while ((c = getopt_long(argc, argv, Short_options, Long_options, 0)) != EOF) {
        switch (c) {
        case 300:  /* help */
        case 'h':  /* help */
            printf("Usage: %s [--machine-output|-m] [inputfile ...]\n", program_name);
            exit(0);
            break;

        case 302:  /* machine-output */
        case 'm':
            Machine_output = 1;
            break;


        default:
            die("Unknown option '%c'", c);
            break;
        }
    }

    argv = &argv[optind];
    argc = argc - optind;

    if (argc < 1) {
        static char *args[] = {"-", 0};
        argv = args;
        argc = 1;
    }

    VECT_INIT(&v, 256*1024);
    arena_new(&a, 1048576);

    for (int i = 0; i < argc; ++i) {
        read_words(&v, a, argv[i]);
    }

    uint64_t t0 = 0,
             t1 = 0,
             t2 = 0,
             nn = VECT_LEN(&v);
    uint64_t *keys = NEWA(uint64_t, nn);

    // gather keys in one place
    for (uint64_t i = 0; i < nn; i++) {
        word *w = &VECT_ELEM(&v, i);
        keys[i] = w->h;
    }

    Xorfilter *x8, *x16;

    t0  = timenow();
    x8  = Xorfilter_new8(keys, nn);
    t1  = timenow();
    x16 = Xorfilter_new16(keys, nn);
    t2  = timenow();

#define _d(x)       ((double)(x))
    double d8  = _d(t1) - _d(t0),
           d16 = _d(t2) - _d(t1);

    double s8  = (_Second(1) * (_d(nn) / d8))  / 1000000.0,
           s16 = (_Second(1) * (_d(nn) / d16)) / 1000000.0;


    printf("Input: %" PRIu64 " elements; Create speed Xor8: %3.2f M keys/s, Xor16: %3.2f M keys/s\n", nn, s8, s16);

    Xorfilter_delete(x8);
    Xorfilter_delete(x16);
    arena_delete(a);
    VECT_FINI(&v);
    DEL(keys);
    return 0;
}
