/*
 * Test harness for strsplit_csv()
 */
#include <stdio.h>
#include <errno.h>
#include "utils/utils.h"

#define __xstr(a) #a
#define __str(a)  __xstr(a)

#define __pos       __FILE__ ":" __str(__LINE__) ": "

#define die(fmt,...) do {\
    fprintf(stderr, __pos "Failed: " fmt, __VA_ARGS__);\
    exit(1); \
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
    _T("\"abc\",def,xyz", "abc;def;xyz", 3),
    _T("\"abc\\\"x\",def,ghi", "abc\"x;def;ghi", 3),
    _T("\"abc\\\"x,y,z\",def,ghi", "abc\"x,y,z;def;ghi", 3),
    _T("\"abc\"x,def,ghi", "abcx;def;ghi", 3),      /* 5 */
    _T(",,", ";;", 3),
    _T("\"\",,", ";;", 3),
    _T("\"\\\"\",,", "\";;", 3),
    _T("", "", 3),
    _T("\"abc", "abc", -EINVAL),
    { 0, {0}, 0 }
};

static void
cmpv(int idx, char *strv [], int strv_size, char *str_exp)
{
    char * expv[10];
    int i,
        n = strsplit_quick(expv, 10, str_exp, ";", 0);

    if (n != strv_size) {
        printf("%d: n=%d; exp %d\n", idx, strv_size, n);
        for (i = 0; i < strv_size; i++) {
            printf("[%d] <%s>\n", i, strv[i]);
        }
        die("%d: Count %d does not match exp %d\n", idx, strv_size, n);
    }
    for (i = 0; i < n; i++) {
        if ( 0 != strcmp(strv[i], expv[i]) )
            die("%d: String <%s> does not match exp <%s>\n", idx, strv[i], expv[i]);
    }
}

int
main ()
{
    char buf[64];
    char * strv[16];
    testcase * t;
    int i = 0;

    for (t = tests; t->str; i++, t++) {
        int n;

        strcpy(buf, t->str);
        n = strsplit_csv(strv, t->strv_size, buf, 0);
        if (n < 0) {
            if (n != t->strv_size)
                die("%d: <%s> exp %d, saw %d\n", i, t->str, t->strv_size, n);
        } else {
            cmpv(i, strv, n, t->exp);
        }
    }
    return 0;

}


