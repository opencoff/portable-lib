/*
 * 32-bit Hash function benchmark against realworld input.
 * Prints hashing speed in MB/sec and cycles/byte
 *
 * Usage:
 *    t_hashbench [INPUTFILE]
 *
 * Input file is a list of tokens/keys that must be hashed.
 * Each token is on a separate line.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "error.h"
#include "utils/hashfunc.h"
#include "utils/siphash.h"
#include <time.h>
#include <sys/time.h>
#include "utils/arena.h"
#include "utils/utils.h"
#include "fast/vect.h"

#include <assert.h>


/*
 * Each string points into an arena. Don't try to free() it!
 */
struct token
{
    char * str;   // null terminated string
    size_t       len;   // length
};
typedef struct token token;

// Dynamically growing array of tokens
VECT_TYPEDEF(token_array, token);


// fold a 64 bit value into a 32 bit one. Use all 64 bits to do the
// folding.
static inline uint32_t
fold32(uint64_t v) { return 0xffffffff & (v - (v >> 32)); }


static uint32_t
city_hash32(const void* p, size_t len, uint32_t seed)
{
    return fold32(city_hash(p, len, seed));
}

static uint32_t
siphash24_32(const void* p, size_t len, uint32_t seed)
{
    uint64_t key[2] = { seed, seed };

    return fold32(siphash24(p, len, &key[0]));
}


struct hashfunc
{
    const char* name;
    uint32_t (*func)(const void*, size_t n, uint32_t seed);
};
typedef struct hashfunc hashfunc;



/*
 * Add new hash funcs here
 */
static const hashfunc Hashes[] = 
{
      {"city",     city_hash32 }
    , {"murmur3",  murmur3_hash_32 }
    , {"hsieh",    hsieh_hash }
    , {"fnv",      fnv_hash }
    , {"jenkins",  jenkins_hash }
    , {"fasthash", fasthash32 }
    , {"siphash",  siphash24_32 }
    , {"yorrike",  yorrike_hash32 }
    , { 0, 0 }
};




static uint32_t
benchmark(const token_array* tok, const hashfunc* hf)
{
    int i,
        n = VECT_SIZE(tok);
    uint64_t nbytes = 0;
    double speed, perbyte;

    volatile uint32_t hv = 0;
    uint64_t t0, tn = 0;
    uint64_t t1, tm = 0;

    for (i = 0; i < n; i++)
    {
        token* t = &VECT_ELEM(tok, i);
        nbytes  += t->len;

        t1  = timenow();
        t0  = sys_cpu_timestamp();
        hv += (*hf->func)(t->str, t->len, 0);
        tn += sys_cpu_timestamp() - t0;
        tm += timenow() - t1;
    }

    // cycles per byte
    perbyte = (double)tn / (double)nbytes;

    // this is speed: hashing bytes per sec
    //speed   = (60.0 * 1.0e6 * (double)nbytes) / (((double)tm) * 1048576.0);
    speed = ((double)nbytes) / ((double)tm);

    printf("%9s: %8.2f MB/sec %5.4f cyc/byte\n", 
            hf->name, speed, perbyte);

    return hv;
}


static int
read_tokens(token_array* tok, FILE* fp, arena_t a)
{
    unsigned char buf[1024];
    int n;
    int min = INT_MAX,
        max = 0,
        avg = 0;
    uint32_t  tot = 0;

    while ((n = freadline(fp, buf, sizeof buf)) > 0)
    {
        token t;

        t.str = (char*) arena_alloc(a, n+1);
        t.len = n;
        assert(t.str);

        memcpy(t.str, buf, n+1);
        VECT_APPEND(tok, t);

        if (n < min)
            min = n;
        if (n > max)
            max = n;

        tot += n;
    }

    avg = tot / VECT_SIZE(tok);

    printf("%u bytes in %lu tokens, min len %d, max %d, avg %d\n",
            tot, VECT_SIZE(tok), min, max, avg);
    return 0;
}



int
main(int argc, char* argv[])
{
    const hashfunc* hf;
    int i, e;
    arena_t a;
    token_array tok;

    program_name = argv[0];

    if ((e = arena_new(&a, 65536)) < 0)
        error(1, e, "Unable to initialize memory arena");

    VECT_INIT(&tok, 65536);

    if (argc < 2)
    {
        static char* sv[] = { 0, "-", 0 };
        argc = 2;
        argv = &sv[0];
    }

    for (i = 1; i < argc; i++)
    {
        char * fn = argv[i];
        FILE * fp = 0;

        if (0 == strcmp(fn, "-"))
            fp = stdin;
        else
        {
            fp = fopen(fn, "rb");
            if (!fp)
                error(1, errno, "Can't open file '%s'", fn);
        }

        read_tokens(&tok, fp, a);
        if (fp != stdin)
            fclose(fp);
    }

    for (hf=&Hashes[0]; hf->name; hf++)
    {
        benchmark(&tok, hf);
    }

    VECT_FINI(&tok);
    arena_delete(a);

    return 0;
}

