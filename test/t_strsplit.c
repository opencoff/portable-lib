
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/utils.h"


typedef struct testcase  testcase;
struct testcase
{
    const char * str;
    const char * delim;
    int sqz;
    int exp_count;
    const char * expv[16];
};

#define _T0(sqz,str,delim) {str,delim,sqz,0, {}}
#define _T1(sqz,str,delim,v1) {str,delim,sqz,1, {v1}}
#define _T2(sqz,str,delim,v1,v2) {str,delim,sqz,2, {v1, v2}}
#define _T3(sqz,str,delim,v1,v2,v3) {str,delim,sqz,3, {v1,v2,v3}}
#define _T4(sqz,str,delim,v1,v2,v3,v4) {str,delim,sqz,4, {v1,v2,v3,v4}}
#define _T5(sqz,str,delim,v1,v2,v3,v4, v5) {str,delim,sqz,5, {v1,v2,v3,v4,v5}}
static testcase tests[] =
{
    _T3(0, "abc def ghi",       " ",  "abc", "def", "ghi"),  // 0
    _T3(0, "abc:def.ghi",       ":.", "abc", "def", "ghi"),
    _T3(0, "abc def ghi",       " ",  "abc", "def", "ghi"),
    _T3(0, "a b c",             " ",  "a", "b", "c"),
    _T3(0, "a b.c",             ". ", "a", "b", "c"),
    _T3(0, " abc  ",            " ",  "", "abc", ""),        // 5
    //{ 0, 0, 0, 0, { 0 } },
    _T2(0, "abc  ",             " ",  "abc", ""),
    _T1(0, "abc",               " :.", "abc"),               // 7
    _T1(0, "abc ",              " ",  "abc"),
    _T1(0, " ",                 " :.", ""),
    _T0(0, "",                  " :."),                     // 10
    _T4(0, "1.2.3.4",           ".", "1", "2", "3", "4"),


    _T0(1, "",                  " \t"),                      // 11
    _T0(1, " \t",               " \t"),
    _T1(1, "   abc   ",         " \t", "abc"),
    _T1(1, "abc",               " \t", "abc"),
    _T1(1, "   abc  ",          " ",   "abc"),               // 15
    _T3(1, "abc  def\t ghi",    " \t", "abc", "def", "ghi"),
    _T3(1, "abc  def  ghi  ",   " ",   "abc", "def", "ghi"),
    _T3(1, "  abc  def  ghi  ", " ",   "abc", "def", "ghi"), // 18
    _T3(1, "  abc  def  ghi  ", " ",   "abc", "def", "ghi"),


    _T5(0, "a:::b:c",           ":",   "a", "", "", "b", "c"),  // 20
    _T5(0, "a b...c",           ". ",  "a", "b", "", "", "c"),
    { 0, 0, 0, 0, { 0 } }
};


static int
vartest()
{
    const testcase * test;
    int fail = 0;
    int i;
    char** strv;
    int    n;

    for (test = tests; test->str; ++test)
    {
        char* in  = strdup(test->str);

        n    = 0;
        strv = strsplit(&n, in, test->delim, test->sqz);

        if ( n != test->exp_count )
        {
            printf("V [%zd] in=|%s| %s ", test - tests,
                    test->str, test->sqz ? "SQZ" : "NOSQZ");
            printf("count mismatch: exp %d saw %d\n",
                    test->exp_count, n);
            for (i = 0; i < n; i++)
                printf ("    [%d]->|%s|\n", i, strv[i]);
            ++fail;
        }
        else
        {
            for (i = 0; i < n; ++i)
            {
                if ( 0 != strcmp(strv[i], test->expv[i]))
                    printf("V [%zd] Failed [|%s|, |%s|%s], at %d\n\texp |%s|, saw |%s|\n",
                              test - tests, test->str, test->delim,
                              test->sqz ? " SQZ" : "",
                              i, test->expv[i], strv[i]);
            }
        }

        free(strv);
        free(in);
    }

    return fail;
}


static int
fixedtest()
{
    const testcase * test;
    int fail = 0;
    int i;
    char* strv[16];

    for (test = tests; test->str; ++test)
    {
        char* in  = strdup(test->str);
        int n = strsplit_quick(strv, test->exp_count, in, test->delim, test->sqz);

        if ( n != test->exp_count )
        {
            printf("F [%zd] in=|%s| %s ", test - tests,
                    test->str, test->sqz ? "SQZ" : "NOSQZ");
            printf("count mismatch: exp %d saw %d\n",
                    test->exp_count, n);
            for (i = 0; i < n; i++)
                printf ("    [%d]->|%s|\n", i, strv[i]);
            ++fail;
        }
        else
        {
            for (i = 0; i < n; ++i)
            {
                if ( 0 != strcmp(strv[i], test->expv[i]))
                    printf("F [%zd] Failed [|%s|, |%s|%s], at %d\n\texp |%s|, saw |%s|\n",
                              test - tests, test->str, test->delim,
                              test->sqz ? " SQZ" : "",
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

#if 0
    char teststr[] = "abcdef";
    char buf[1024];
    int j;

    for (j = 0, i = 0xff; i >= 0xf0; i--)
        buf[j++] = (char)i;

    buf[j] = 0;
    n      = 0;
    strv   = strsplit(&n, teststr, buf, 1);
    printf("Binary tok, n = %d\n", n);
    for(i = 0; i < n; ++i)
    {
        printf(" => v[%d] = %s\n", i, strv[i]);
    }
    free(strv);
#endif

    int r = vartest();

    r += fixedtest();

    return r;
}
