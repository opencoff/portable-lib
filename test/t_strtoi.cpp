/*
 * Test harness for strtoi<T>() template function
 */
#include "utils/strutils.h"
#include <stdio.h>

using namespace putils;


struct testcase
{
    const char * str;
    bool result;
    int base;
    unsigned long long val;
};

#define _y(a)       { #a, true,  0, (unsigned long long)a }
#define _yy(a,b,c)  { #a, true,  b, (unsigned long long)c }
#define _n(a,b)     { a,  false, 0, b }

static const testcase Tests [] =
{
      _y(123)
    , _y(-123)
    , _y(0555)
    , _y(0)
    , _y(0x1234)
    , _y(0x1234abcd)
    , _yy(123, 10, 123)
    , _yy(123, 16, 0x123)
    , _yy(777, 8, 0777)
    , _n("0x1234z", 0)
    , _n("abc", 0)
};

#define NTESTS (sizeof Tests/ sizeof Tests[0])

int
main ()
{
    for (size_t i = 0; i < NTESTS; ++i)
    {
        const testcase * t = &Tests[i];
        std::string s(t->str);

        std::pair<bool, unsigned long long> r = strtoi<unsigned long long>(s, t->base);

        if ( r.first != t->result || r.second != t->val )
        {
            printf ("** %zu: exp %llu, saw %llu\n", i, t->val, r.second);
        }
    }
    return 0;
}
