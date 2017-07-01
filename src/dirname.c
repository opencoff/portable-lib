/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * dirname.c - portable, re-entrant implementation of the shell
 *             builtin dirname(1)
 *
 * Copyright (c) 2006 Sudhi Herle <sw at herle.net>
 *
 * License: GPLv2
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#include <string.h>
#include <stdlib.h>


const char *
dirname(char *buf, size_t bsize, const char *p)
{
    size_t n = strlen(p);
    if (bsize < (n+1)) return 0;

    memcpy(buf, p, n);
    buf[n] = 0;

    char *s = buf + n - 1;

    // skip trailing '/'
    for (; s > buf; s--) {
        if (*s != '/') goto found;
    }
    goto slash;

found:
    s[1] = 0;   // remove the last slash.
    for (; s > buf; s--) {
        if (*s == '/') {
            *s = 0;
            return buf;
        }
    }

slash:
    if (buf[0] != '/') buf[0] = '.';
    buf[1] = 0;
    return buf;
}

/* EOF */
