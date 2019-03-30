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
#include "utils/utils.h"

/*
 * We store token delimiters as bits in a 256-wide bitarray. We
 * compose this large bitarray from an array of uint64_t.
 *
 * Checking to see if a character is a delimiter then boils down to
 * checking if the appropriate bit within the long-word is set.
 */
#define LONGBYTES     (sizeof(uint64_t))
#define LONGBIT       (LONGBYTES * 8)   // 8 bits per byte
#define BITVECTSIZE   (256 / LONGBIT)

typedef uint64_t      DELIM_TYPE[BITVECTSIZE];

#define INIT_DELIM(v)  memset(v, 0, sizeof v)

#define ADD_DELIM(v, c) do { \
                            uint64_t c_ = (uint64_t)(c);\
                            int w_ = c_ / LONGBIT; \
                            int b_ = c_ % LONGBIT; \
                            v[w_] |= (1 << b_);\
                        } while (0)

#define IS_DELIM(v,c) ({ \
                            uint64_t c_ = (uint64_t)(c);\
                            int w_ = c_ / LONGBIT; \
                            int b_ = c_ % LONGBIT; \
                            v[w_] & (1 << b_);\
                        })



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

    DELIM_TYPE  v;

    INIT_DELIM(v);

    if (!sep) {
        ADD_DELIM(v, ',');  // default delimiter
    } else {
        int x;
        for (x = 0; (x = *sep); sep++) {
            ADD_DELIM(v, x);
        }
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
                } else {
                    // Start of quote.
                    q = c;
                }

                str++;
                continue;

            default:
                if (IS_DELIM(v, c)) {
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
        if (q)              return -EINVAL;
        if (k == sv_size) return -ENOSPC;

        *p++ = 0;
        sv[k++] = sub;
    }
    return k;
}

/* EOF */
