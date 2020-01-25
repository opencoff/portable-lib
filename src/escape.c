/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * escape.c - Perform escaping of chars.
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
#include <assert.h>
#include "utils/gstring.h"
#include "utils/new.h"
#include "bits.h"


/*
 * If 'src' contains any chars in
 * 'escset', escape it with 'esc_char'.
 *
 * Note: 'esc_char' can't be in 'escset'.
 */
gstr *
gstr_escape(gstr *g, const char *escset, int esc_char)
{
    char buf[8];
    delim d;
    int c;

    __init_delim(&d);
    for (; (c = *escset); escset++) {
        __add_delim(&d, c);
    }

    assert(!__is_delim(&d, esc_char));

    // we stash the src pointer while we walk it.
    const char *src = g->str;
    const char *ptr = src;

    // And re-initialize the pointer
    g->str = NEWZA(char, g->cap);
    g->len = 0;

    assert(g->str);

    while ((c = *ptr++)) {
        if (c == esc_char) {
            gstr_append_ch(g, c);
            gstr_append_ch(g, c);
        } else if (__is_delim(&d, c)) {
            gstr_append_ch(g, esc_char);
            switch (c) {
                case '\a':
                    gstr_append_ch(g, 'a');
                    break;
                case '\b':
                    gstr_append_ch(g, 'b');
                    break;
                case '\f':
                    gstr_append_ch(g, 'f');
                    break;
                case '\n':
                    gstr_append_ch(g, 'n');
                    break;
                case '\r':
                    gstr_append_ch(g, 'r');
                    break;
                case '\t':
                    gstr_append_ch(g, 't');
                    break;
                case '\v':
                    gstr_append_ch(g, 'v');
                    break;
                default:
                    snprintf(buf, sizeof buf, "%c0%3.3o", esc_char, c);
                    gstr_append_str(g, buf);
            }
        } else {
            gstr_append_ch(g, c);
        }
    }

    // And now free the old ptr
    DEL(src);
    return g;
}
/* EOF */
