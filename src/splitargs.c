/* :vim:ts=4:sw=4:
 *
 * splitargs.c - Split string into tokens and handle embedded quoted words.
 *
 * Copyright (c) 1999-2015 Sudhi Herle <sw at herle.net>
 *
 * Copyright (c) 2016, 2017
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include "utils/strutils.h"

/*
 * Split 's' into tokens just as the shell would:
 *
 * - arguments separated by ' ' or '\t'
 * - arguments can optionally be quoted by '"' or "'"
 * - quoted arguments are unquoted
 *
 * Returns:
 *  >= 0    number of tokens
 *  < 0     on error: -errno
 */
int
strsplitargs(char **args, size_t n, char *s)
{
    int c;
    int i = 0;
    int k = n;
    int inquote = 0;
    int eow = 0;

    char *p = s;    // start of substring

#define ADDARG(z)   do {\
                        if (k == 0) return -ENOSPC;\
                        args[i++] = (z); \
                        k--;             \
                    } while (0)

    for (; (c = *s); s++) {
        if (inquote) {
            if (c == inquote) { // quote ended
                *s = 0;
                ADDARG(p+1);    // +1 to skip the starting quote.
                p       = s+1;  // +1 to skip the ending quote
                inquote = 0;
            }

            // regardless, we consume everything inside the quotes.
            continue;
        }

        if (isspace(c)) {
            *s  = 0;
            eow = 1;
            continue;
        }

        if (c == '"' || c == '\'') {
            inquote = c;
        }

        // either c is a quote-char or a non-ws char. In either
        // case, we terminate the previous word (if there is one).
        if (eow) {
            if (*p) ADDARG(p);
            p   = s;
            eow = 0;
        }
    }

    if (inquote) return -EINVAL;    // no ending quote

    if (*p) ADDARG(p);

    return i;   // total number of arguments
}



