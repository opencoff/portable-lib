/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * unescape.cpp - Unescape a string -- remove escape chars and make the string
 * "plain".
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

#include "utils/strutils.h"

using namespace std;

namespace putils {

// convinient short hand
typedef string::size_type   size_type;

static size_type parse_esc(string& ret,            const string& src, size_type off, char esc);
static size_type xtract_decimal(unsigned char * c, const string& str, size_type off);
static size_type xtract_octal(unsigned char * c,   const string& str, size_type off);
static size_type xtract_hex(unsigned char * c,     const string& str, size_type off);

#if 0
#define DD(a)   printf a
#else
#define DD(a)
#endif


/*
 * Unescape a string 'src' where 'esc_char' is used to "escape" the
 * special chars.
 */
string
string_unescape(const string& src, char esc_char)
{
    const size_type n = src.size();
    size_type i = 0;

    string ret;

    while (i < n)
    {
        char c = src[i];
        if ( c == esc_char )
        {
            i = parse_esc(ret, src, i, esc_char);
        }
        else
        {
            ret += c;
            ++i;
        }
    }

    return ret;
}



/*
 * Parse an escaped substring.
 * Return new offset from whence to commence processing
 */
static size_type
parse_esc(string& ret, const string& src, size_type off, char esc)
{
    char c = 0;
    char v = 0;

    ++off;  // Skip the escape char

    DD(("parse-esc{%s} off=%d size=%d\n",
                src.c_str(), off, src.size()));
    if ( off < src.size() )
    {
        c = src[off];
        ++off;
    }

    switch (c)
    {
        default:
            v = c;
            break;

        case 'a':
            v  = '\a';
            break;
        case 'b':
            v = '\b';
            break;
        case 'e':
        case 'E':
            v =  033;
            break;
        case 'f':
            v = '\f';
            break;
        case 'n':
            v = '\n';;
            break;
        case 'r':
            v = '\r';
            break;
        case 't':
            v = '\t';
            break;
        case 'v':
            v = '\v';
            break;

        /* Premature EOS. Expected to see some char. But, we just
         * saw the '\\'. So, we just return the '\\'
         * But, we have to make sure that the outer-loop stops
         * processing.
         */
        case 0:
            v = esc;
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'x':
        {
            unsigned char cvt = 0;
            int n;

            switch (c)
            {
                case '0':
                    n = xtract_octal (&cvt, src, off);
                    break;

                case 'x':
                    n = xtract_hex (&cvt, src, off);
                    break;

                default:
                    // Decimal: we must count everything after the
                    // escape char -- thus, we go back one offset
                    // position in order to extract the digits.
                    --off;
                    n = xtract_decimal(&cvt, src, off);
                    break;
            }

            // If we didn't succeed in converting digits, we just
            // put back whatever we saw so far and continue past the
            // anomalies
            if ( n < 0 )
            {
                ret += esc;
                cvt  = c;
                n    = 2;
            }

            off += n;
            v = cvt;
            break;
        }
    }

    ret += v;
    return off;
}



static size_type
xtract_octal(unsigned char * c, const string& str, size_type off)
{
    size_type i,
               start = off,
               avail;

    // Only process available chars
    avail = str.size() - off;
    if ( avail > 3 )
        avail = 3;

    DD((" xtract-oct off=%d substr={%s}\n", off, str.substr(off, avail).c_str()));

    unsigned int n = 0;
    for (i = 0; i < avail; ++i, ++off)
    {
        unsigned int o = str[off];
        if ( o >= '0' && o <= '7' )
            n = (n << 3) + o - '0';
        else
            break;
    }


    n &= 0xff; // 0x7f ?
    *c = n;

    DD(("  xtract-oct{%s} i=%d, off=%d ret=%d\n",
            str.c_str(), i, off, off - start));
    return off - start;
}

static size_type
xtract_decimal(unsigned char * c, const string& str, size_type off)
{
    size_type i,
              start = off,
              avail;

    // Only process available chars
    avail = str.size() - off;
    if ( avail > 3 )
        avail = 3;

    DD((" xtract-dec off=%d substr={%s}\n", off, str.substr(off, avail).c_str()));

    unsigned int n = 0;
    for (i = 0; i < avail; ++i, ++off)
    {
        unsigned int o = str[off];
        if ( isdigit (o) )
            n = (n * 10) + (o - '0');
        else
            break;
    }

    n &= 0xff; // 0x7f ?
    *c = n;

    DD(("  xtract-dec{%s} i=%d, off=%d ret=%d\n",
            str.c_str(), i, off, off - start));
    return off - start;
}

static int
_xtoi(char c)
{
    int digit = 0;

    if (c >= '0' && c <= '9')
        digit = c - '0';
    else if (c >= 'a' && c <= 'f')
        digit = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
        digit = c - 'A' + 10;

    return digit;
}

static size_type
xtract_hex(unsigned char * c, const string& str, size_type off)
{
    size_type i,
              start = off,
              avail = str.size() - off;

    if ( avail > 2 )
        avail = 2;

    unsigned int n = 0;
    for (i = 0; i < avail; ++i, ++off)
    {
        unsigned int o = str[off];
        if ( isxdigit (o) )
            n = (n << 4) + _xtoi(o);
        else
            break;
    }

    // If we didn't find even a single hex char, we don't unescape
    // this blob and leave the leading prefix and everything after it
    // intact.
    if ( i == 0 )
        --off;

    n &= 0xff; // 0x7f ?
    *c = n;

    return off - start;
}

}

/* EOF */
