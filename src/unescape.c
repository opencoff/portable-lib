/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * unescape.c - Unescape a string -- remove escape chars and make the string
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

#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include "utils/gstring.h"
#include "utils/new.h"


static const char * parse_esc(char **p_out,     const char *ptr, int esc_char);
static const char * extract_octal(int *p_val,   const char *ptr);
static const char * extract_hex(int *p_val,     const char *ptr);
static const char * extract_decimal(int *p_val, const char *ptr);

#if 0
#define DD(a)   printf a
#else
#define DD(a)
#endif


/*
 * Unescape a string 'src' where 'esc_char' is used to "escape" the
 * special chars.
 */
gstr*
gstr_unescape(gstr *g, int esc_char)
{
    // stash the pointer safely while we walk it
    const char *src = g->str,
               *ptr = src,
               *end = src + g->len;
    char *out       = g->str;

    while (ptr < end) {
        int c = *ptr;
        assert(c);
        if (c == esc_char) {
            ptr = parse_esc(&out, ptr+1, esc_char);
        }
        else {
            *out++ = c;
            ptr++;
        }
    }

    *out   = 0;
    g->len = out - g->str;
    return g;
}



/*
 * Parse an escaped substring.
 * Return new offset from whence to commence processing
 */
static const char *
parse_esc(char **p_out, const char *src, int esc_char)
{
    char *out       = *p_out;
    const char *ptr = src;

    DD(("parse-esc: <%s>\n", ptr));

    int v = 0;
    int c = *ptr++;
    switch (c) {
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
            v = esc_char;
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
            int val;

            switch (c) {
                case '0':
                    ptr = extract_octal(&val, ptr);
                    break;

                case 'x':
                    ptr = extract_hex(&val, ptr);
                    break;

                default:
                    // Decimal: we must count everything after the
                    // escape char -- thus, we go back one offset
                    // position in order to extract the digits.
                    ptr = extract_decimal(&val, ptr-1);
                    break;
            }

            if (ptr) {
                v = val;
                goto out;
            }

            // If we didn't succeed in converting digits, we just
            // put back whatever we saw so far and continue past the
            // anomalies
            *out++ = esc_char;
            *out++ = c;
            *p_out = out;
            return src + 2;
        }
    }

out:
    *out++ = v;
    *p_out = out;
    return ptr;
}


static const char *
extract_octal(int *p_val, const char *ptr)
{
    int n   = 3;
    int val = 0;

    while (n-- > 0) {
        int c = *ptr;

        if (!c)                      break;
        if (!(c >= '0' && c <= '7')) break;

        val = (val << 3) + (c - '0');
        ptr++;
    }

    *p_val = val & 0xff;
    return ptr;
}


static const char *
extract_decimal(int *p_val, const char *ptr)
{
    int n   = 3;
    int val = 0;

    while (n-- > 0) {
        int c = *ptr;

        if (!c)          break;
        if (!isdigit(c)) break;

        val = (val * 10) + (c - '0');
        ptr++;
    }

    *p_val = val & 0xff;
    return ptr;
}

static const char *
extract_hex(int *p_val, const char *ptr)
{
    int n   = 2;
    int val = 0;

    while (n-- > 0) {
        int c = *ptr;
        int z = 0;

        switch (c) {
            case '0': case '1': case '2':
            case '3': case '4': case '5':
            case '6': case '7': case '8':
            case '9':
                z = c - '0';
                break;

            case 'a': case 'b': case 'c':
            case 'd': case 'e': case 'f':
                z = c - 'a';
                break;

            case 'A': case 'B': case 'C':
            case 'D': case 'E': case 'F':
                z = c - 'A';
                break;

            case 0:
            default:
                goto out;
        }

        val = (val << 4) + z;
        ptr++;
    }

out:
    *p_val = val & 0xff;
    return ptr;
}

/* EOF */
