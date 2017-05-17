/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * escape.cpp - Perform escaping of chars.
 *
 * Copyright (c) 2004-2007 Sudhi Herle <sw at herle.net>
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

#include <stdio.h>
#include <bitset>
#include "utils/strutils.h"
#include "quickbits.h"

using namespace std;

namespace putils {


/*
 * If 'src' contains any chars in
 * 'escset', escape it with 'esc_char'.
 *
 * Note: 'esc_char' can't be in 'escset'.
 */
string
string_escape(const string& src, const string& escset, char esc_char)
{
    char buf[8];
    delim_bitset delim;

    delim.reset();
    for (string::size_type i = 0; i < escset.size(); ++i)
        delim.set(escset[i]);


    string ret;
    const string::size_type n = src.size();
    for (string::size_type i = 0; i < n; ++i)
    {
        char c = src[i];
        if ( c == esc_char )
        {
            ret += esc_char;
            ret += esc_char;
        }
        else if ( delim[c] )
        {
            snprintf(buf, sizeof buf, "%c0%3.3o", esc_char, c);

            ret += buf;
        }
        else
            ret += c;
    }

    return ret;
}

}
/* EOF */
