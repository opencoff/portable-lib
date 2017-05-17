/* vim: expandtab:sw=4:ts=4:notextmode:tw=82:
 *
 * hsieh_hash.c - Paul Hsieh's fast hash function
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
 * This is Paul Hsieh's super fast hash function. For details see:
 *  http://www.azillionmonkeys.com/qed/hash.html
 *
 * Synopsis:
 *   o it beats Jenkins' hash in terms of performance
 *   o it beats FNV hash
 *   o it has identical/better collisin distribution than Jenkins
 *   o it is considered the best hash function as of Dec 2009.
 */

#include <stdint.h>
#include <sys/types.h>
#include "utils/hashfunc.h"

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif


/*
 * To make an incremental version of this hash function, pass 'hash' as a
 * parameter and initialize it to 0.
 */
uint32_t
hsieh_hash(const void * pdata, size_t len, uint32_t seed)
{
    const char* data = (const char*)pdata;
    uint32_t hash = seed,
             tmp;
    size_t rem;

    if (!len || !data)
        return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--)
    {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem)
    {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

#ifdef TEST
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <error.h>

int
main(int argc, char* argv[])
{
    uint8_t buf[256];
    int i = 0;
    int n = 1000;
    struct timespec start, end, diff;
    double d;
    uint32_t h = 0;

    if (argc > 1)
    {
        n = atoi(argv[1]);
        if (n <= 0)
            n = 1000;
    }
    srand(time(0));

    for (i = 0; i < 256; ++i)
    {
        buf[i] = 0xff & rand();
    }

    gettimeofday_ns(&start, 0);
    for (i = 0; i < n; ++i)
    {
        h = hsieh_hash(buf, 256);
    }
    gettimeofday_ns(&end, 0);

    timespecsub(&end, &start, &diff);

    d = ((double)diff.tv_nsec + ((double)diff.tv_sec * 1.0e9))/ ((double)n);
    printf("%#8.8x %5.4fns (%d)\n", h, d, n);

    return 0;
}

#endif

/* EOF */
