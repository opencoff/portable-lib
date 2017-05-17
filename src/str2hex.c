/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * str2hex.c -- Decode hex string to byte stream
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

#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/types.h>

#include "utils/utils.h"


/*
 * Decode hex chars in string 'str' and put them into 'out'.
 *
 * Return:
 *   > 0    Number of decoded bytes
 *   < 0    -errno on failure
 */
ssize_t
str2hex(uint8_t* out, size_t outlen, const char* str)
{
    uint8_t* start = out;
    size_t n       = strlen(str);

    if (outlen < ((n+1)/2))  return -ENOMEM;

    uint8_t byte = 0;
    int shift    = 4;

    while (n--) {
        unsigned char nybble = *str++;
        switch (nybble) {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                byte |= (nybble - '0') << shift;
                break;
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                byte |= (nybble - 'a' + 10) << shift;
                break;
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                byte |= (nybble - 'A' + 10) << shift;
                break;
            default:
                return -EINVAL;
        }

        if ((shift -= 4) == -4) {
            *out++ = byte;
            byte = 0x00;
            shift = 4;
        }
    }

    if (shift != 4) *out++ = byte;

    return out - start;
}

