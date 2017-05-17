/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * uuid2str.c - UUID bytes to printable string.
 *
 * Copyright (c) 2015 Sudhi Herle <sw at herle.net>
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

#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

// Process n bytes at a time
static char*
chunk(char* s, uint8_t* b, size_t n)
{
    static const char hex[] = "0123456789ABCDEF";
    uint8_t*  e = b + n;

    for (; b < e; ++b) {
        uint8_t z = *b;

        *s++ = hex[0xf & (z >> 4)];
        *s++ = hex[0xf & z];
    }
    return s;
}

ssize_t
uuid2str(char* out, size_t outsz, uint8_t* uuid)
{
    if (outsz < 37) return -ENOSPC;

    char * p = out;
    p = chunk(p, uuid, 4);
    *p++ = '-'; uuid += 4;

    p = chunk(p, uuid, 2);
    *p++ = '-'; uuid += 2;

    p = chunk(p, uuid, 2);
    *p++ = '-'; uuid += 2;

    p = chunk(p, uuid, 2);
    *p++ = '-'; uuid += 2;

    p  = chunk(p, uuid, 6);
    *p++ = 0;

    return p - out;
}
