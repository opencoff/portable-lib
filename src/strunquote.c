/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * strunquote.c -- Remove start/end quotes of a string
 *
 * Copyright (c) 2013-2015 Sudhi Herle <sw at herle.net>
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

#include "utils/utils.h"
#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>


/*
 * Remove starting and ending quote (if any) and return pointer to
 * underlying string in 'p_ret'.
 * This function expects to see matched pair of quotes -- either '"' or '\''.
 *
 * Return value:
 *    > 0:  Quote char we encountered
 *      0:  No quote found
 *    < 0:  No closing quote found.
 *
 * NB: This function expects a trimmed string (no leading or
 *     trailing white spaces).
 */
int
strunquote(char* s, char** p_ret)
{
    if (!s) return 0;

    int q = *s;
    if (q != '"' && q != '\'') return 0;

    int n = strlen(s+1);
    if (s[n-1] != q) return -1;

    s[--n] = 0;
    if (p_ret)  *p_ret = s + 1;
    return q;
}
