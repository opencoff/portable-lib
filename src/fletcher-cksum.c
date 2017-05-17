/* vim: ts=4:sw=4:expandtab:tw=72:
 * 
 * fletcher_checksum.c - Fletcher checksum for 32 bit values.
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
#include <stdint.h>

uint32_t
fletcher32(uint16_t *data, size_t len)
{
    uint32_t sum1 = 0xffff, sum2 = 0xffff;

    while (len)
    {
        size_t tlen = len > 360 ? 360 : len;
        len -= tlen;
        do
        {
            sum1 += *data++;
            sum2 += sum1;
        } while (--tlen);
        sum1 = (sum1 & 0xffff) + (sum1 >> 16);
        sum2 = (sum2 & 0xffff) + (sum2 >> 16);
    }
    /* Second reduction step to reduce sums to 16 bits */
    sum1 = (sum1 & 0xffff) + (sum1 >> 16);
    sum2 = (sum2 & 0xffff) + (sum2 >> 16);
    return (sum2 << 16) | sum1;
}

