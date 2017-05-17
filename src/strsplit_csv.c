/* :vi:ts=4:sw=4:
 * 
 * strsplit_csv.c -  Borrowed heavily from
 *  "Practice of Programming" --  (Kernighan and Pike)
 *
 * Copyright (c) 2005 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#include <stdlib.h>
#include <string.h>
#include "utils/utils.h"

#if 0
#define K_AND_R 1
#endif

int
strsplit_csv (char * strv [], int strv_size, char * str)
{
    int c,
        sep,
        n;
    char * start;

    n = 0;
    do
    {
        if ( n >= strv_size )
            return -1;

        start = str;
        if ( *str == '"' )
        {
            /* Skip the initial quote */
            char * dest = start = ++str;

            for (; (c = *str); *dest++ = *str++)
            {
                if ( c == '"' )
                {
                    /* Skip embedded '"' */
                    if ( *++str == '"' ) continue;

                    /* Not an embedded '"'. i.e., we are no longer in
                     * quote mode. We copy chars till we see a ',' or a
                     * null char. This will end the processing of quoted
                     * chars. */
                    for (; (c = *str); *dest++ = *str++)
                        if ( c == ',' )
                            break;
                    break;
                }
            }
            *dest = 0;
        }
        else
        {
            /* Skip until we find a ',' */
            for (; (c = *str); str++)
                if ( c == ',' )
                    break;
        }
        sep = *str;
        *str++ = 0;
        strv[n++] = start;
    } while (sep == ',');
    return n;
}

#ifdef K_AND_R
static char * adv_quoted (char *);

int
strsplit_csv2 (char * strv [], int strv_size, char * str)
{
    char * p,
         * p_sep;
    int n,
        sep;
    
    if ( ! (str && *str) )
        return 0;

    n = 0;
    p = str;
    do
    {
        if ( n >= strv_size )
            return -1;  /* XXX WHAT ELSE? */

        if ( *p == '"' )
            p_sep = adv_quoted (++p);
        else
            p_sep = p + strcspn (p, ",");

        sep = *p_sep;
        *p_sep = 0;

        strv[n++] = p;
        p = p_sep + 1;
    } while (sep == ',');
    return n;
}

static char *
adv_quoted (char * p)
{
    int c;
    char * dest;

    for (dest = p; (c = *p); *dest++ = *p++)
    {
        if ( c == '"')
        {
            if ( *++p == '"' )
                continue;

            /* Copy till we see a ',' and quit the
             * outer loop. */
            for (; (c = *p); *dest++ = *p++)
                if ( c == ',' )
                    break;
            break;
        }
    }

    /* Null terminate the fragment */
    *dest = 0;
    return p;
}

#endif /* K_AND_R */

#ifdef TEST

/* Ugly. Ugly. Ugly. */
#undef TEST
#include "strsplit.c"
#include "strtrim.c"
#define TEST

#include <stdio.h>

#define __xstr(a) #a
#define __str(a)  __xstr(a)

#define __pos       __FILE__ ":" __str(__LINE__) ": "

#define FAIL3(fmt,v1,v2,v3) do { \
    fprintf (stderr, __pos "Failed: " fmt, v1, v2, v3); \
    exit (1); \
} while (0)

typedef struct testcase  testcase;
struct testcase
{
    char * str;
    char exp[64];
    int strv_size;
};

#define _T(str,exp,vsize) {str,exp,vsize}
static testcase tests[] =
{
    _T("abc", "abc", 10),                           /* 0 */
    _T("abc,def,ghi", "abc;def;ghi", 10),
    _T("abc,def,ghi", "abc;def;ghi", 3),
    _T("\"abc\",def,ghi", "abc;def;ghi", 3),
    _T("\"abc\"\"x\",def,ghi", "abc\"x;def;ghi", 3),
    _T("\"abc\"\"x,y,z\",def,ghi", "abc\"x,y,z;def;ghi", 3),
    _T("\"abc\"x,def,ghi", "abcx;def;ghi", 3),      /* 5 */
    _T(",,", ";;", 3),
    _T("\"\",,", ";;", 3),
    _T("\"\"\"\",,", "\";;", 3),
    _T("", "", 3),
    { 0, 0, 0 }
};

static void
cmpv (int idx, char * strv [], int strv_size, char * str_exp)
{
    char * expv[10];
    int i,
        n = strsplit (expv, 10, str_exp, ";", 0);

    if (n != strv_size)
    {
        for (i = 0; i < strv_size; i++)
        {
            printf ("[%d] <%s>\n", i, strv[i]);
        }
        FAIL3("%d: Count %d does not match exp %d\n", idx, strv_size, n);
    }
    for (i = 0; i < n; i++)
    {
        if ( 0 != strcmp (strv[i], expv[i]) )
            FAIL3("%d: String <%s> does not match exp <%s>\n", idx, strv[i], expv[i]);
    }
}

int
main ()
{
    char buf[64];
    char * strv[16];
    testcase * test;

    for (test = tests; test->str; test++)
    {
        int i, n;

        strcpy (buf, test->str);
#ifdef K_AND_R
        n = strsplit_csv2 (strv, test->strv_size, buf);
#else
        n = strsplit_csv (strv, test->strv_size, buf);
#endif /* K_AND_R */
        cmpv (test - tests, strv, n, test->exp);
    }
    return 0;

}


#endif /* TEST */

/* EOF */
