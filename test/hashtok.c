/*
 * Find dups after hashing a set of input tokens.
 * Use a flexible mask to generate the final hashval.
 *
 * (c) Sudhi Herle 2016; Licensed under GPLv2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <inttypes.h>

#include "getopt_long.h"
#include "error.h"
#include "utils/hashfunc.h"
#include "utils/siphash.h"
#include "fast/vect.h"

// Holds truncated hash, the hash and the source string
struct H
{
    uint64_t h;     // truncated hash
    uint64_t x;     // full hash
    const char* s;  // original string
};
typedef struct H H;

// HV is a vector of H
VECT_TYPEDEF(HV, H);


// Useful typedef
typedef uint64_t (*hashfunc_t)(const void*, size_t, uint64_t seed);

struct hash_func_pair
{
    const char* name;
    hashfunc_t fp;
};
typedef struct hash_func_pair hash_func_pair;

// Calculate siphash with _one_ seed val
static uint64_t
siphash_seed(const void* a, size_t n, uint64_t seed)
{
    uint64_t seedv[] = { seed, seed };
    return siphash24(a, n, seedv);
}


// List of available hash functions
static const hash_func_pair Hashnames [] =
{
    {"fnv",     fnv_hash64},
    {"city",    city_hash},
    {"siphash", siphash_seed},
    {"fast",    fasthash64},

    {"default", siphash_seed},
    {0, 0}
};

// Long and short options
static struct option Lopt[] =
{
    {"mask",    required_argument, 0, 'm'},
    {"hash",    required_argument, 0, 'H'},
    {"help",  no_argument, 0, 'h'},
    {0, 0,0, 0}
} ;
static char Sopt[] = "m:H:h" ;


// Map a name to a hash func
static hashfunc_t
str2hash(const char* name)
{
    const hash_func_pair* p = &Hashnames[0];

    for (; p->name; ++p) {
        if (0 == strcasecmp(name, p->name))
            return p->fp;
    }

    return 0;
}


// comparison function for VECT_SORT()
static int
h_cmp(const H* a, const H* b)
{
    if (a->h < b->h) return -1;
    if (a->h > b->h) return +1;
    return 0;
}

// Duplicate a string 's' of length 'n' and null terminate result.
static char*
dupstr(char * s, size_t n)
{
    char* x = malloc(n+1);
    assert(x);
    memcpy(x, s, n);
    x[n] = 0;
    return x;
}


// Show usage and exit
static void
Usage(int s)
{
    printf("\
Usage: %s [OPTIONS] [file ..]\n\
  file                     Input file to read data from\n\
\n\
  OPTIONS:\n\
      --mask=B -m B         Use a 'B' bits mask on the output [32]\n\
      --hash=NAME, -H NAME  Use hash function 'NAME' instead of default\n\
                            NAME must be one of 'default', 'fnv', 'city', \n\
                            'siphash[*]', 'fast'\n\
", program_name) ;

    exit(s) ;
}


// Traverse sorted vector 'hv' and print all duplicates.
// Return true if there are dups, false otherwise.
static int
finddups(HV* hv)
{
    size_t i;
    size_t n = VECT_LEN(hv);

    if (n < 2) return 0;

    size_t dups = 0;

    for (i = 0; i < (n-1); ++i) {
        H* h0 = &VECT_ELEM(hv, i);
        H* h1 = &VECT_ELEM(hv, i+1);

        if (h0->h == h1->h) {
            if (h0->x == h1->x) error(1, 0, "** Duplicate keys: %s (%" PRIu64 ")", h0->s, h0->x);
            error(0, 0, "# Dup %d [%s, %s]", h0->h, h0->s, h1->s);
            ++dups;
        }
    }

    return dups;
}



#define _u64(a)     ((uint64_t)(a))
#define BUFSIZE     256
int
main(int argc, char* argv[])
{
    program_name = argv[0];

    uint64_t abuf[BUFSIZE/sizeof(uint64_t)];

    char* buf     = (char*) &abuf[0];
    FILE* fp      = stdin;
    int maskbits  = 32;
    uint64_t mask = 0;
    int c;

    hashfunc_t hashfunc = str2hash("default");

    while ( (c=getopt_long(argc, argv, Sopt, Lopt, 0)) != EOF ) {
        switch(c)
        {
            case 0 :
                break ;

            case 'm' :
                maskbits = atoi(optarg) ;
                assert(maskbits > 0);
                break ;
            case 'H':
                hashfunc = str2hash(optarg);
                if (!hashfunc)
                    error(1, 0, "Can't find hash function '%s'", optarg);
                break;
            case 'h':
            default :
                Usage(1) ;
        }
    }

    mask = (_u64(1) << maskbits) - 1;

    if (optind < argc) {
        char* fn = argv[optind];
        fp = fopen(fn, "r");
        if (!fp)
            error(1, errno, "Can't open file '%s'", fn);
    }


    HV hv;

    VECT_INIT(&hv, 65536);
    do
    {
        char* s = fgets(buf, BUFSIZE, fp);
        size_t n;

        if (!s)
            break;

        n = strlen(s);
        if (s[n-1] == '\n')
            s[--n] = 0;
        if (s[n-1] == '\r')
            s[--n] = 0;

        H h;
        h.s = dupstr(s, n);
        h.x = (*hashfunc)(s, n, 0);
        h.h = h.x & mask;

        VECT_PUSH_BACK(&hv, h);
    } while (1);

    if (fp != stdin)
        fclose(fp);


    /* Now sort the data and write it out */
    VECT_SORT(&hv, h_cmp);

    finddups(&hv);

    H* h;
    VECT_FOR_EACH(&hv, h) {
        printf("%" PRIu64 ", %" PRIu64 ", %s\n", h->h, h->x, h->s);
    }

    VECT_FINI(&hv);

    return 0;
}

/* EOF */
