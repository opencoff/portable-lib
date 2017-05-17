/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * t_fixedsize.cpp - simple test harness for fixed size allocator
 *
 * Copyright (c) 2005 Sudhi Herle <sw@herle.net>
 *
 * Licensing Terms: (See LICENSE.txt for details). In short:
 *    1. Free for use in non-commercial and open-source projects.
 *    2. If you wish to use this software in commercial projects,
 *       please contact me for licensing terms.
 *    3. By using this software in commercial OR non-commercial
 *       projects, you agree to send me any and all changes, bug-fixes
 *       or enhancements you make to this software.
 * 
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Creation date: Thu Oct 20 10:58:22 2005
 */

#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <string>
#include <algorithm>
#include <time.h>
#include <sys/time.h>

#include "utils/mempool.h"
#include "utils/utils.h"
#include "error.h"


extern "C" uint32_t arc4random(void);

using namespace putils;
using namespace std;

struct obj
{
    int v;
    string s;
    void * ptr;
};

struct obj2
{
    char b[64];
};

typedef Mempool<obj>  obj_allocator_type;
typedef Mempool<obj2> obj2_allocator_type;
typedef vector<obj *> vector_type;

#define N       (65535 * 16)


static size_t
cpp_rand(size_t n)
{
    return arc4random() % n;
}

struct stopwatch
{
    uint64_t n;
    stopwatch() : n(0) {}
    ~stopwatch() {}

    void     start() { n = timenow(); }
    uint64_t stop()  { return timenow() - n; }
};

struct cyclectr
{
    uint64_t t0;
    cyclectr() : t0(0) {}
    ~cyclectr() {}

    void start() { t0 = sys_cpu_timestamp(); }
    uint64_t stop() { return sys_cpu_timestamp() - t0; }
};


static void
perf_test(int run)
{
    mempool* st;
    obj2_allocator_type aa(0, N);
    vector<obj2 *> va;
    vector<void *> vb;

    if (0 != mempool_new(&st, 0, sizeof(obj2), N, 0))
        return ;

    va.reserve(N);
    vb.reserve(N);

    stopwatch tm;
    cyclectr  cy;

    uint64_t  cpp_tm_a = 0,
              cpp_tm_f = 0,
              cpp_cy_a = 0,
              cpp_cy_f = 0,
              c_tm_a   = 0,
              c_tm_f   = 0,
              c_cy_a   = 0,
              c_cy_f   = 0;

    for (int i = 0; i < N; ++i)
    {
        obj2* o;
        void * p;

        tm.start();
        cy.start();
            p = aa.Alloc();
        cpp_cy_a += cy.stop();
        cpp_tm_a += tm.stop();

        o = new(p) obj2();

        tm.start();
        cy.start();
            p = mempool_alloc(st);
        c_cy_a += cy.stop();
        c_tm_a += tm.stop();

        va.push_back(o);
        vb.push_back(p);
    }

    random_shuffle(va.begin(), va.end(), cpp_rand);
    random_shuffle(vb.begin(), vb.end(), cpp_rand);

    vector<obj2 *>::iterator vai = va.begin();
    for (; vai != va.end(); ++vai)
    {
        obj2* o = *vai;

        tm.start();
        cy.start();
            aa.Free(o);
        cpp_cy_f += cy.stop();
        cpp_tm_f += tm.stop();
    }

    vector<void *>::iterator vbi = vb.begin();
    for (; vbi != vb.end(); ++vbi)
    {
        void* o = *vbi;

        tm.start();
        cy.start();
            mempool_free(st, o);
        c_cy_f += cy.stop();
        c_tm_f += tm.stop();
    }

#define _speed(x)     (double(x)/double(N))
#define _rate(x)      (double(N)/double(x))

    printf("%2d: C++ %4.2f M alloc/s %4.2f cy/alloc %4.2f M free/s %4.2f cy/free"
                "  C %4.2f M alloc/s %4.2f cy/alloc %4.2f M free/s %4.2f cy/free\n",
        run, _rate(cpp_tm_a), _speed(cpp_cy_a), _rate(cpp_tm_f), _speed(cpp_cy_f),
             _rate(c_tm_a),   _speed(c_cy_a),   _rate(c_tm_f),   _speed(c_cy_f));

    mempool_delete(st);
}

#if 1
static void
simple_test()
{
    // max=0, chunksize=4096
    obj_allocator_type a(0, 4096);

    printf("Infinite-mem-allocator block size: %d\n", a.Blocksize());

    vector_type  v;

    v.reserve(N);

    for(int i = 0; i < N; ++i)
    {
        try
        {
            void* p = a.Alloc();
            obj * o = new(p) obj();
    
            v.push_back(o);
            //printf("Alloc: [%5d] %p\n", i, o);
        }
        catch(std::bad_alloc& e)
        {
            error(0, 0, "Out of memory while allocating block# %d\n", i);
            break;
        }
    }

    printf("Infinite-mem-allocator: completed allocating %u blocks; freeing ..\n", N);

    random_shuffle(v.begin(), v.end(), cpp_rand);

    vector_type::iterator vi = v.begin();
    for (int i = 0; vi != v.end(); ++vi, ++i)
    {
        obj * o = *vi;
        //printf("Free: [%5d] %p\n", i, o);
        a.Free(o);
    }

    printf("Infinite-mem-allocator: OK\n");
}


static void
pool_test()
{
    unsigned long ubuf[4096];
    obj_allocator_type a(ubuf, sizeof ubuf);
    int i = 0;

    printf("Fixed-mem-allocator: avail=%u blocks %u bytes each\n",
            a.Totalblocks(), a.Blocksize());

    vector_type  v;

    v.reserve(N);

    while (1)
    {
        try
        {
            void* p = a.Alloc();
            obj * o = new(p) obj();
            v.push_back(o);
            ++i;
            //printf("Alloc: [%5d] %p\n", i, o);
        }
        catch(std::bad_alloc& e)
        {
            if (v.size() != a.Totalblocks())
                error(1, 0, "** Abnormal OOM while allocating block # %u from fixed sized pool\n",
                        v.size());
            break;
        }

    }

    printf("fixed-mem-allocator: Allocated %lu blocks; freeing ..\n", v.size());

    random_shuffle(v.begin(), v.end(), cpp_rand);

    vector_type::iterator vi = v.begin();
    for (i = 0; vi != v.end(); ++vi, ++i)
    {
        obj * o = *vi;
        //printf("Free: [%5d] %p\n", i, o);
        a.Free(o);
    }
    printf("fixed-mem-allocator: OK\n");
}
#endif

int
main()
{
    pool_test();
    simple_test();

    for(int i = 0; i < 8; ++i)
        perf_test(i);
}

/* EOF */
