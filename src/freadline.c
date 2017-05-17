/* :vim:ts=4:sw=4:
 *
 * freadline.c - Robust readline() implementation that handles CR,
 * CRLF and LF line break chars
 *
 * Copyright (c) 1999-2007 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Creation date: Mon Aug 30 09:31:50 1999
 */

#include <stdio.h>
#include <errno.h>

#include "utils/utils.h"

// Returns:
//    number of chars in the buf on success
//    0 on EOF
//    -ENOSPC if input buffer is too small
//    -errno on error
int
freadline(FILE* fp, unsigned char * str, int n)
{
    unsigned char * end = str + n;

    while (1)
    {
        int c = fgetc(fp);
        if (c == EOF)
            return 0;

        if (ferror(fp))
            return -errno;

        if (c == '\r')
        {
            int next = fgetc(fp);
            if (next != EOF && next != '\n')
                ungetc(next, fp);

            break;
        }
        else if (c == '\n')
            break;

        if (str == end)
            return -ENOSPC;

        *str++ = c;
    }

    *str = 0;
    return str - (end - n);
}

