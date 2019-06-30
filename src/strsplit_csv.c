/* :vi:ts=4:sw=4:
 *
 * strsplit_csv.c -  Inspired by
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
#include <errno.h>
#include <assert.h>
#include "utils/utils.h"
#include "bits.h"



/*
 * Split a line of text that is comma separated and honor quoted
 * words (comma's inside quoted words are ignored). We respect
 * either type of quote: '\'' or '"'; we ignore escaped quote chars,
 * ie., embedded quote chars preceded by a '\\'.
 *
 * The separator can be any of characters in 'sep'. A nil ptr
 * indicates the _default_ of ','.  If 'sep' contains more than
 * one character, then any of them can be a valid separator.
 * e.g., ",;" indicates that ',' or ';' can be a field separator.
 * NB: This function destructively modifies the input string; the
 *     output array 'strv' points to locations within 'str'.
 */
int
strsplit_csv(char *sv[], int sv_size, char *str, const char *sep)
{
    int c,
        k = 0,   // count of # of elements
        q = 0;

    char *sub = str, // start of substring.
         *p   = str; // copy ptr

    delim  v;

    __init_delim(&v);

    /*
     * TODO May 2019
     *
     * on x86_64, gcc and clang both generate XMM based code for
     * vectorized operation on the delimiter bitset.
     *
     * And, with full optimization, the old code doesn't work:
     *
     *  if (sep) {
     *      for (c = 0; (c = *sep); sep++) {
     *          ADD_DELIM(&v, c);
     *      }
     *  } else {
     *      ADD_DELIM(&v, ',');
     *  }
     *
     *
     * Both compilers don't seem to emit any reasonable code for the
     * 'else' branch; i.e., the bits appear to be unset!
     *
     * The code below is written as a workaround..
     */

    if (!sep) sep = ",";
    for (; (c = *sep); sep++) {
        __add_delim(&v, c);
        assert(__is_delim(&v, c));
    }

    while ((c = *str)) {
        switch (c) {
            case '\'': case '"':
                if (q == c) {
                    // Already in a quote. If previous char is an
                    // escape char - we unescape and keep the quote
                    // char.
                    if (str[-1] == '\\') {
                        p[-1] = q;
                    } else {
                        // Otherwise, we are done with the quoted word.
                        q = 0;
                    }
                } else if (q > 0) {
                    *p++ = *str;
                } else {
                    // Start of quote.
                    q = c;
                }

                str++;
                continue;

            default:
                if (__is_delim(&v, c)) {
                    // If we are not quoting, then we terminate word
                    // here.
                    if (q == 0) {
                        if (k == sv_size) return -ENOSPC;

                        *p++ = 0;
                        sv[k++] = sub;

                        // Start of new word;
                        sub = p;
                        str++;
                        continue;
                    }
                }

                // copy char to current pos
                *p++ = *str++;
                break;
        }
    }

    if (sub != p) {
        if (q)            return -EINVAL;
        if (k == sv_size) return -ENOSPC;

        *p++ = 0;
        sv[k++] = sub;
    }
    return k;
}

/* EOF */
