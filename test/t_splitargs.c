#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "utils/strutils.h"

struct testcase
{
    const char * str;
    int exp_count;
    const char * expv[16];
};
typedef struct testcase  testcase;

#define _T0(str)                    {str,0, {}}
#define _T1(str,v1)                 {str,1, {v1}}
#define _T2(str,v1,v2)              {str,2, {v1, v2}}
#define _T3(str,v1,v2,v3)           {str,3, {v1,v2,v3}}
#define _T4(str,v1,v2,v3,v4)        {str,4, {v1,v2,v3,v4}}
#define _T5(str,v1,v2,v3,v4, v5)    {str,5, {v1,v2,v3,v4,v5}}
#define _Tx(str, err)               {str, -err, {}}
static testcase tests[] =
{
    _T3("abc def ghi",       "abc", "def", "ghi"),  // 0
    _T3("abc def ghi",        "abc", "def", "ghi"),
    _T3("abc def   ghi",      "abc", "def", "ghi"),
    _T3("abc \tdef   ghi",    "abc", "def", "ghi"),
    _T3("abc   def  \tghi",   "abc", "def", "ghi"),
    _T3("abc   def  \tghi  ", "abc", "def", "ghi"),
    _T3("a b c",             "a", "b", "c"),
    _T2("a b.c",             "a", "b.c"),
    _T1("abc  ",             "abc"),
    _T1("abc",               "abc"),
    _T1("a  ",               "a"),
    _T0(" "),

    _T1("'abc'",                "abc"),
    _T2("'abc  ' d",            "abc  ", "d"),
    _T2("'abc  ' \"def  \"",    "abc  ", "def  "),
    _T2("'abc  ' \"def \t\"",   "abc  ", "def \t"),
    _T2("abc \"def\t\"",        "abc", "def\t"),

    _T1("\"abc\"",              "abc"),
    _Tx("\"abc",                EINVAL),
    _Tx("abc\'",                EINVAL),

    // Always keep last
    { 0, 0, { 0 } }
};

static int
fixedtest()
{
    const testcase * test;
    int fail = 0;
    int i;
    char* strv[16];

    for (test = tests; test->str; ++test) {
        char* in = strdup(test->str);
        int n    = strsplitargs(strv, test->exp_count, in);

        if (test->exp_count < 0) {
            if (n != test->exp_count) {
                printf("[%zd] in=|%s|: exp to fail, saw %d", test-tests, test->str, n);
            }
            free(in);
            continue;
        }


        if (n != test->exp_count) {
            printf("[%zd] in=|%s| ", test - tests, test->str);
            printf("count mismatch: exp %d saw %d\n", test->exp_count, n);
            for (i = 0; i < n; i++)
                printf ("    [%d]->|%s|\n", i, strv[i]);
            ++fail;
        }
        else {
            for (i = 0; i < n; ++i) {
                if ( 0 != strcmp(strv[i], test->expv[i]))
                    printf("[%zd] Failed [|%s| at %d exp |%s|, saw |%s|\n",
                              test - tests, test->str,
                              i, test->expv[i], strv[i]);
            }
        }

        free(in);
    }

    return fail;
}


int
main ()
{
    return fixedtest();
}


