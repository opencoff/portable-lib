/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * basename.c - A Win32 implementation for basename().
 *
 * Copyright (C) 1998-2006 Sudhi Herle <sw at herle.net>
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
 * Creation date: Wed Jul  5 19:23:17 1998
 */

#include <string.h>
#include "utils/utils.h"

const char*
basename(const char* str)
{
    int len = strlen(str);
    const char* end = str + len - 1;

    for (; end >= str; end--)
    {
        int c = *end;
        if (c == '/' || c == '\\')
            return (char*) (end+1);
    }
    return str;
}

/* EOF */
