/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * t_esc.c - Unit test harness for testing escape() and unescape()
 * functions.
 *
 * Copyright (c) 2005 Sudhi Herle <sw@herle.net>
 *
 * Licensing Terms: (See LICENSE.txt for details). In short:
 *    1. Redistribution in source and binary form permitted
 *       provided:
 *          a) The copyright remains Sudhi Herle's
 *          b) Any such copyright notices are retained in the source
 *             and are not removed
 *    2. Free for use in non-commercial and open-source projects.
 *    3. If you wish to use this software in commercial projects,
 *       please contact me for licensing terms.
 *    4. By using this software in commercial OR non-commercial
 *       projects, you agree to send me any and all changes, bug-fixes
 *       or enhancements you make to this software.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#include <stdio.h>
#include <ctype.h>
#include "utils/gstring.h"


struct testcase
{
    const char * str;
    const char * escset;
};
typedef struct testcase testcase;


static const testcase Tests [] =
{
      {"",                          "\a\b\t\n\v\f\r\b"} // 0
    , {"a",                         "\a\b\t\n\v\f\r\b"}
    , {"\\\\//aabb",                "\a\b\t\n\v\f\r\b"}
    , {"\\\\aabb",                  "\a\b\t\n\v\f\r\b"}
    , {"abc",                       "\a\b\t\n\v\f\r\b"}
    , {"a\tb\rc\nd\fe\af\fg\vh\b",  "\a\b\t\n\v\f\r\b"} // 5
    , {"\a\b\t\n\v\f\r\b",          "\a\b\t\n\v\f\r\b"}
    , {"a\t1\r2\n3\f4\a5\f6\v7\b8", "\a\b\t\n\v\f\r\b"}
    , {0, 0}
};

struct test
{
    const char * str;
    const char * exp;
};
typedef struct test test;

static const test Esc_tests [] =
{
       {"a\t\b\t11\n13\n01", "a|t|b|t11|n13|n01"}
    ,  {"abc",               "abc"}
    ,  {"|",                 "||"}
    ,  {"a",                 "a"}
    ,  {"|0011",             "||0011"}
    ,  {0, 0}
};


static const test Unesc_tests [] =
{
#if 1
       {"||",               "|"}
    ,  {"|",                "|"}
    ,  {"",                 ""}
    ,  {"|v|n|f",           "\v\n\f"}
    ,  {"|011abc",          "\tabc"}
    ,  {"|0011123",         "\t123"}
#endif
    ,  {"|1",               "\1"}
    ,  {"|1abc",            "\1abc"}
    ,  {"|x09abc",            "\tabc"}
    ,  {0, 0}
};


static gstr
make_printable(const char *s)
{
    int c;
    gstr g;

    gstr_init(&g, 0);

    for (;  (c = *s); s++) {
        switch (c) {
            case '\v':
                gstr_append_str(&g, "\\v");
                break;
            case '\f':
                gstr_append_str(&g, "\\f");
                break;
            case '\b':
                gstr_append_str(&g, "\\b");
                break;
            case '\n':
                gstr_append_str(&g, "\\n");
                break;
            case '\r':
                gstr_append_str(&g, "\\r");
                break;
            case '\t':
                gstr_append_str(&g, "\\t");
                break;

            default:
                if (isprint(c)) {
                    gstr_append_ch(&g, c);
                } else {
                    char buf[8];
                    snprintf(buf, sizeof buf, "\\x%2.2x", c);

                    gstr_append_str(&g, buf);
                }
                break;
        }
    }

    return g;
}


static void
failed(const char *prefix, int i, const char *str, const char *exp, const char *saw)
{
    gstr s = make_printable(str);
    gstr e = make_printable(exp);
    gstr w = make_printable(saw);
    printf ("** failed %s %d; str={%s}\n  exp={%s} saw={%s}\n",
            prefix, i, gstr_str(&s), gstr_str(&e), gstr_str(&w));

    gstr_fini(&s);
    gstr_fini(&e);
    gstr_fini(&w);
}


int
main()
{
    int fail = 0;
    const testcase * t = Tests;

    for (int i = 0; t->str; ++t, ++i) {
        gstr src;
        gstr str;

        gstr_init_from(&src, t->str);
        gstr_dup(&str, &src);

        gstr_escape(&str, t->escset, '\\');
        gstr_unescape(&str, '\\');

        if (!gstr_eq(&src, &str)) {
            ++fail;
            failed("mismatch", i, t->str, t->str, gstr_str(&str));
        }

        gstr_fini(&src);
        gstr_fini(&str);
    }


    const char *escset = "\f\v\b\n\r\t";
    const test * tt    = Esc_tests;
    for (int i = 0; tt->str; ++tt, ++i) {
        gstr src;
        gstr exp;

        gstr_init_from(&src, tt->str);
        gstr_init_from(&exp, tt->exp);

        gstr_escape(&src, escset, '|');

        if (!gstr_eq(&src, &exp)) {
            ++fail;
            failed("escape mismatch", i, tt->str, tt->exp, gstr_str(&src));
        }
        gstr_fini(&src);
        gstr_fini(&exp);

    }

    tt = Unesc_tests;
    for (int i = 0; tt->str; ++tt, ++i) {
        gstr src;
        gstr exp;

        gstr_init_from(&src, tt->str);
        gstr_init_from(&exp, tt->exp);

        gstr_unescape(&src, '|');

        if (!gstr_eq(&src, &exp)) {
            ++fail;
            failed("unescape mismatch", i, tt->str, tt->exp, gstr_str(&src));
        }
        gstr_fini(&src);
        gstr_fini(&exp);
    }

    return fail;
}
