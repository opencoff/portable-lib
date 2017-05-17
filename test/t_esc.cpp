/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * t_esc.cpp - Unit test harness for testing escape() and unescape()
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
 *
 *
 * Rev: $Id: escape.cpp,v 1.5 2006/01/22 08:37:38 sherle Exp $
 *
 */

#include "utils/strutils.h"

#include <stdio.h>

using namespace std;
using namespace putils;


struct testcase
{
    const char * str;
    const char * escset;
};


static const testcase Tests [] =
{
      {"",                          "\a\b\t\n\v\f\r\b"} // 0
    , {"a",                         "\a\b\t\n\v\f\r\b"}
    , {"\\\\//aabb",                "\a\b\t\n\v\f\r\b"}
    , {"\\\\aabb",                  "\\\a\b\t\n\v\f\r\b"}
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

static const test Esc_tests [] =
{
       {"a\t\b\t11\n13\n01", "a|0011|0010|001111|001213|001201"}
    ,  {"abc",               "abc"}
    ,  {"|",                 "||"}
    ,  {"a",                 "a"}
    ,  {"|0011",             "||0011"}
    ,  {0, 0}
};


static const test Unesc_tests [] =
{
       {"||",               "|"}
    ,  {"|",                "|"}
    ,  {"",                 ""}
    ,  {"|v|n|f",           "\v\n\f"}
    ,  {"|011abc",          "\tabc"}
    ,  {"|0011123",         "\t123"}
    ,  {"|1",               "\1"}
    ,  {"|1abc",            "\1abc"}
    ,  {"|x",               "|x"}
    ,  {"|x09abc",            "\tabc"}
    ,  {0, 0}
};


static string
make_printable(const string &s)
{
    string ret;

    for (string::size_type i = 0; i < s.size(); ++i)
    {
        switch (s[i])
        {
            case '\v':
                ret += "\\v";
                break;
            case '\f':
                ret += "\\f";
                break;
            case '\b':
                ret += "\\b";
                break;
            case '\n':
                ret += "\\n";
                break;
            case '\r':
                ret += "\\r";
                break;
            case '\t':
                ret += "\\t";
                break;

            default:
                if (isprint (s[i]))
                    ret += s[i];
                else
                {
                    char buf[8];
                    snprintf(buf, sizeof buf, "\\x%2.2x", s[i]);

                    ret += buf;
                }
                break;
        }
    }

    return ret;
}


static void
failed(const char * prefix, int i, const string& str, const string& exp,
       const string& saw)
{
    string s = make_printable (str);
    string e = make_printable (exp);
    string x = make_printable (saw);
    printf ("** failed %s %d; str={%s}\n  exp={%s} saw={%s}\n",
            prefix, i, s.c_str(), e.c_str(), x.c_str());
}

static bool
compare(const string& lhs, const string& rhs)
{
    if ( lhs.size() != rhs.size() )
        return false;

    for (string::size_type i = 0; i < lhs.size(); ++i)
    {
        if ( lhs[i] != rhs[i] )
            return false;
    }

    return true;
}

int
main()
{
    int fail = 0;
    const testcase * t = Tests;

    for (int i = 0; t->str; ++t, ++i)
    {
        string str    = t->str;
        string escset = t->escset;

        string escaped = string_escape (str, escset);

        string unescaped = string_unescape (escaped);

        if ( !compare (str, unescaped) )
        {
            ++fail;
            failed ("mismatch", i, str, str, unescaped);
        }
    }


    const string escset = "\f\v\b\n\r\t";
    const test * tt = Esc_tests;
    for (int i = 0; tt->str; ++tt, ++i)
    {
        string str = tt->str;
        string exp = tt->exp;

        string saw = string_escape (str, escset, '|');

        if ( !compare (saw, exp) )
        {
            ++fail;
            failed ("escape mismatch", i, str, exp, saw);
        }
    }

    tt = Unesc_tests;
    for (int i = 0; tt->str; ++tt, ++i)
    {
        string str = tt->str;
        string exp = tt->exp;

        string saw = string_unescape (str, '|');

        if ( !compare(saw, exp) )
        {
            failed ("unescape mismatch", i, str, exp, saw);
            ++fail;
        }
    }

    return fail;
}
