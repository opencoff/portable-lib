/*
 * Test harness for various hash functions.
 *
 * Reads input and prints hashval for various 32-bit hash functions.
 * A companion python program (hash_analyze.py) reads the output and
 * looks for collisions.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "error.h"
#include "utils/hashfunc.h"
#include "utils/siphash.h"

extern uint32_t arc4random(void);

static uint32_t
city_hash32(const void* p, size_t len, uint32_t seed)
{
    uint64_t v = city_hash(p, len, seed);

    return 0xffffffff & (v - (v >> 32));
}


static uint32_t
siphash_32(const void* p, size_t len, uint32_t seed)
{
    uint64_t z[] = { seed, seed };
    uint64_t v = siphash24(p, len, &z[0]);

    return 0xffffffff & (v - (v >> 32));
}


#define BUFSIZE     256
int
main(int argc, char* argv[])
{
    //char buf[256];
    uint64_t abuf[BUFSIZE/sizeof(uint64_t)];

    char* buf     = (char*) &abuf[0];
    FILE* fp      = stdin;
    uint32_t seed = arc4random();

    program_name = argv[0];

    if (argc > 1) {
        char* fn = argv[1];
        if (strcmp(fn, "-h") == 0) {
            printf("Usage: %s [filename]\n", argv[0]);
            exit(1);
        }
        fp = fopen(argv[1], "r");
        if (!fp)
            error(1, errno, "Can't open file '%s'", argv[1]);
    }

#define MASK    (0xffffffff)

    printf("city,murmur3,hsieh,fnv,jenkins,fasthash,yorrike,siphash,word\n");
    do
    {
        char* s = fgets(buf, BUFSIZE, fp);
        char* e;
        size_t n;
        uint32_t city, murmur, hsieh, jenkins, fnv, fhash, yorr, siph;

        if (!s)
            break;

        n = strlen(s);
        if (s[n-1] == '\n')
            s[--n] = 0;
        if (s[n-1] == '\r')
            s[--n] = 0;

        /*
         * We only consider the first word for hashing.
         */
        for (e=s; e < (s+n); ++e)
        {
            int z = *e;
            if (z == ' ' || z == '\t')
            {
                *e = 0;
                break;
            }
        }

        city    = MASK & city_hash32(s, n, seed);
        murmur  = MASK & murmur3_hash_32(s, n, seed);
        hsieh   = MASK & hsieh_hash(s, n, seed);
        fnv     = MASK & fnv_hash(s, n, seed);
        jenkins = MASK & jenkins_hash(s, n, seed);
        fhash   = MASK & fasthash32(s, n, seed);
        yorr    = MASK & yorrike_hash32(s, n, seed);
        siph    = MASK & siphash_32(s, n, seed);

        printf("%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%s\n",
                city, murmur, hsieh, fnv, jenkins, fhash, yorr, siph, s);
    } while (1);

    if (fp != stdin)
        fclose(fp);

    return 0;
}
