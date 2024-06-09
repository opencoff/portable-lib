/*
 * Test 32-bit hash func speed.
 *
 * Usage: $0 [sz..]
 *
 * The benchmark is run for each buffer size on the commandline.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include <time.h>
#include <sys/time.h>
#include "utils/utils.h"
#include "utils/hashfunc.h"
#include "utils/siphash.h"
#include "utils/xxhash.h"
#include "utils/metrohash.h"

static uint32_t
city_hash32(const void* p, size_t len, uint32_t seed)
{
    uint64_t v = city_hash(p, len, seed);

    return 0xffffffff & (v - (v >> 32));
}

static uint32_t
siphash24_32(const void* p, size_t len, uint32_t seed)
{
    uint64_t key[2] = { seed, seed };
    uint64_t v      = siphash24(p, len, &key[0]);

    return 0xffffffff & (v - (v >> 32));
}

static uint32_t
metro32(const void* p, size_t len, uint32_t seed)
{
    union {
        uint64_t v;
        uint8_t  b[8];
    } u;

    (void)seed;
    metrohash64_1(p, len, 0, u.b);
    return 0xffffffff & (u.v - (u.v >> 32));
}

struct hashfunc
{
    const char* name;
    uint32_t (*func)(const void*, size_t n, uint32_t seed);
};
typedef struct hashfunc hashfunc;



#define _x(a,b) {#a, b}

struct hresult
{
    double speed;   // hash speed (MB/s)
    char   str[256];
};
typedef struct hresult hresult;

/*
 * Add new hash funcs here
 */
static const hashfunc Hashes[] = 
{
      {"city", city_hash32 }
    , {"xxhash", XXH32 }
    , {"metro",  metro32 }
    , {"murmur3", murmur3_hash_32 }
    , {"hsieh", hsieh_hash }
    , {"fnv", fnv_hash }
    , {"fasthash", fasthash32 }
    , {"siphash",  siphash24_32 }
    , {"yorrike", yorrike_hash32 }
};



static hresult
benchmark(const void* buf, int buflen, const hashfunc* hf)
{
    uint64_t st, tm;
    int i, n = 4000;
    uint64_t tot, tmtot;

    double cy_speed, cy_perbyte;
    double tm_speed;

    volatile uint32_t hv = 0;

    st = sys_cpu_timestamp();
    tm = timenow();
    for (i = 0; i < n; i++)
    {
        hv += (*hf->func)(buf, buflen, 0);
    }

    tot   = sys_cpu_timestamp() - st;
    tmtot = timenow() - tm;
    hv = hv;

#define _d(x)   ((double)(x))

    cy_speed   = _d(tot)  / _d(n);
    cy_perbyte = cy_speed / _d(buflen);

    tm_speed   = 1000.0 * (_d(buflen) * _d(n)) / _d(tmtot);

    hresult hr = { .speed = tm_speed };

    snprintf(hr.str, sizeof hr.str, "%9s:  %4d bytes  %7.2f MB/s %7.3f cyc %5.3f cyc/byte\n", 
             hf->name, buflen, tm_speed, cy_speed, cy_perbyte);

    return hr;
}

static int
speed_cmp(const void* x, const void* y)
{
    const hresult * a = x;
    const hresult * b = y;

    if (a->speed < b->speed) return -1;
    if (a->speed > b->speed) return +1;
    return 0;
}

static void
print(hresult *hr, size_t n)
{
    size_t i;
    qsort(hr, n, sizeof hr[0], speed_cmp);

    for(i=0; i < n; i++) fputs(hr[i].str, stdout);
    fputc('\n', stdout);
}

int
main(int argc, char* argv[])
{
    uint32_t abuf[1024];
    int i;

    if (argc < 2)
    {
        static char* sv[] = { 0, "128", 0 };
        argc = 2;
        argv = &sv[0];
    }

    size_t nhash = sizeof(Hashes) / sizeof(Hashes[0]);

    hresult hr[nhash];

    for (i = 1; i < argc; i++)
    {
        size_t sz = atoi(argv[i]);
        size_t j;
        if (sz > sizeof abuf) sz = sizeof abuf;

        for (j = 0; j < nhash; j++) {
            hr[j] = benchmark(&abuf[0], sz, &Hashes[j]);
        }

        print(hr, nhash);
    }

    return 0;
}

/* EOF */
