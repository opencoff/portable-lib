/* :vim:ts=4:sw=4:
 *
 * humanize.c - Turn a large number into human readable string.
 *
 * Copyright (c) 2010-2014 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Creation date: Sat Jun 12 18:22:35 2010
 */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

struct divisor_pair
{
    uint64_t    sz;
    const char* suffix;
};
typedef struct divisor_pair divisor_pair;


#define kB   1024ULL
#define MB  (1024ULL * kB)
#define GB  (1024ULL * MB)
#define TB  (1024ULL * GB)
#define PB  (1024ULL * TB)
#define EB  (1024ULL * PB)


#define _P(a)  { a, #a }

/*
 * They are in strictly decreasing order - do NOT insert new
 * elements randomly!
 * */
static const divisor_pair Divisors[] =
{
      _P(EB)
    , _P(PB)
    , _P(TB)
    , _P(GB)
    , _P(MB)
    , _P(kB)
    , { 0, 0 }
};


/* 
 * Turn size into a human readable format
 */
char *
humanize_size(char * buf, size_t bufsize, uint64_t nbytes)
{
    const divisor_pair* p = &Divisors[0];

    for (; p->sz; ++p)
    {
        if (nbytes > p->sz)
        {
            uint64_t n = nbytes / p->sz;
            uint64_t m = (nbytes % p->sz);

            if (m > 0) {
                char frac[64];
                snprintf(frac, sizeof frac, "%" PRIu64, m);
                snprintf(buf, bufsize, "%" PRIu64 ".%2.2s %s", n, frac, p->suffix);
            } else {
                snprintf(buf, bufsize, "%" PRIu64 " %s", n, p->suffix);
            }
            return buf;
        }
    }

    snprintf(buf, bufsize, "%" PRIu64 " bytes", nbytes);
    return buf;
}

