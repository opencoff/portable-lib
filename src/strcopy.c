/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * strcopy.c -- Safe string copy.
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
 * Copy 'src' to 'dst' without ever overflowing the destination.
 * 'dst' is 'sz' bytes in capacity. Always null terminate.
 *
 * Returns:
 *    >= 0      number of bytes copied
 *    <  0      truncation happened
 */
ssize_t
strcopy(char* dst, size_t sz, const char* src)
{
    char* z = dst;

    assert(sz > 0);

    for (; --sz != 0; dst++, src++) {
        if (!(*dst = *src))
            break;
    }

    if (sz == 0)    *dst = 0;

    return *src ? -1 : dst - z;
}

/* EOF */
