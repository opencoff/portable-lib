/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * uuid.c - Generic v4/v5 UUID generation, stringification
 *
 * Copyright (c) 2017 Sudhi Herle <sw at herle.net>
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
#include <errno.h>

#include "utils/uuid.h"

extern int arc4random_buf(uint8_t* buf, size_t bsiz);

// Generate a random UUID
int
uuid_generate(uint8_t *buf, size_t bsiz)
{
    if (bsiz == 0) return -EINVAL;
    if (bsiz < 16) return -ENOSPC;

    arc4random_buf(buf, 16);

    buf[6] = 0x4f & (buf[6] | 0x40);
    buf[8] = 0xbf & (buf[8] | 0x80);
    return 16;
}


// Parse a UUID from a string
int
uuid_fromstring(uint8_t *buf, size_t bsiz, const char *str)
{
    if (bsiz < 16)         return -ENOSPC;
    if (strlen(str) != 36) return -EINVAL;
    
    uint8_t byte  = 0;
    int     shift = 4,
            c;

    bsiz = 16;  // this is all we need. Don't decode more.
    while ((c = *str++)) {
        switch (c) {
            default:
                return -EINVAL;

            case '-':
                continue;

            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                byte |= (c - '0') << shift;
                break;

            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                byte |= (c - 'a' + 10) << shift;
                break;

            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                byte |= (c - 'A' + 10) << shift;
                break;
        }
        if ((shift -= 4) < 0) {
            *buf++ = byte;
            byte   = 0;
            shift  = 4;
            bsiz--;
        }
    }

    if (bsiz != 0)  return -EINVAL;
    return 16;
}


// Process n bytes at a time
static char*
chunk(char* s, const uint8_t* b, size_t n)
{
    static const char hex[] = "0123456789ABCDEF";
    const uint8_t  *e = b + n;

    for (; b < e; ++b) {
        uint8_t z = *b;

        *s++ = hex[0xf & (z >> 4)];
        *s++ = hex[0xf & z];
    }
    return s;
}


// Convert a UUID to string
int
uuid_tostring(char* out, size_t outsz, const uint8_t* uuid)
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

/* EOF */
