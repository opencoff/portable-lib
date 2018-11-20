#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "utils/utils.h"

struct testcase
{
    size_t sz;          // buffer size
    ssize_t retval;     // expected retval
    const char* exp;    // expected string
    const char* in;     // input string
};
typedef struct testcase testcase;

static const testcase T[] = 
{
      { 1, -1, "",      "hello" }
    , { 1, 0,  "",      ""      }
    , { 2, 0,  "",      ""      }
    , { 2, -1, "x",     "xyz"   }
    , { 2, -1, "xy",    "xy"    }
    , { 3, 2,  "xy",    "xy"    }
    , { 4, 3,  "abc",   "abc"   }
    , { 5, 3,  "ABC",   "ABC"   }

    // Always in the end
    , { 0, 0, 0, 0 }
};

int
main()
{
    const testcase* t = T;
    char outbuf[256];

    for (; t->exp; ++t) {
        assert(t->sz < sizeof outbuf);

        ssize_t r = strcopy(outbuf, t->sz, t->in);
        if (r != t->retval)
            printf("Failed <%s>: exp-ret %zd, saw-ret %zd\n", t->in, t->retval, r);

        if (r > 0) {
            if (0 != memcmp(outbuf, t->exp, r+1))
                printf("Failed <%s>: exp-str <%s>, saw-str <%s>\n", t->in, t->exp, outbuf);
        }
    }
    return 0;
}

