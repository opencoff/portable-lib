/* :vim:ts=4:sw=4:
 *
 * splitargs.c - Split string into tokens and handle embedded quoted words.
 *
 * Copyright (c) 1999-2015 Sudhi Herle <sw at herle.net>
 *
 * Copyright (c) 2016, 2017
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include "utils.h"

/*
 * Split 's' into tokens just as the shell would:
 *
 * - arguments separated by ' ' or '\t'
 * - arguments can optionally be quoted by '"' or "'"
 * - quoted arguments are unquoted
 *
 * Returns:
 *  >= 0    number of tokens
 *  < 0     on error: -errno
 */
int
strsplitargs(char **args, size_t n, char *s)
{
    int c;
    int i = 0;
    int k = n;
    int inquote = 0;
    int eow = 0;

    char *p = s;    // start of substring

#define ADDARG(z)   do {\
                        if (k == 0) return -ENOSPC;\
                        args[i++] = (z); \
                        k--;             \
                    } while (0)

    for (; (c = *s); s++) {
        if (inquote) {
            if (c == inquote) { // quote ended
                *s = 0;
                ADDARG(p+1);    // +1 to skip the starting quote.
                p       = s+1;  // +1 to skip the ending quote
                inquote = 0;
            }

            // regardless, we consume everything inside the quotes.
            continue;
        }

        if (isspace(c)) {
            *s  = 0;
            eow = 1;
            continue;
        }

        if (c == '"' || c == '\'') {
            inquote = c;
        }

        // either c is a quote-char or a non-ws char. In either
        // case, we terminate the previous word (if there is one).
        if (eow) {
            if (*p) ADDARG(p);
            p   = s;
            eow = 0;
        }
    }

    if (inquote) return -EINVAL;    // no ending quote

    if (*p) ADDARG(p);

    return i;   // total number of arguments
}


#ifdef TEST
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct testcase  testcase;
struct testcase
{
    const char * str;
    int exp_count;
    const char * expv[16];
};

#define _T0(str)                    {str,0, {}}
#define _T1(str,v1)                 {str,1, {v1}}
#define _T2(str,v1,v2)              {str,2, {v1, v2}}
#define _T3(str,v1,v2,v3)           {str,3, {v1,v2,v3}}
#define _T4(str,v1,v2,v3,v4)        {str,4, {v1,v2,v3,v4}}
#define _T5(str,v1,v2,v3,v4, v5)    {str,5, {v1,v2,v3,v4,v5}}
#define _Tx(str, err)               {str, -err}
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

#endif /* TEST */

