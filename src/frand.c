/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * frand.c - High speed, random floating point numbers in the range
 *           [0.0, 1.0).
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
 *
 *
 * Notes
 * =====
 * IEEE 754 double precision format:
 *   bit 64: sign
 *   bit 63-52: exponent (11 bits)
 *   Bit 51-0:  fraction.
 *
 * So, when we set sign = 0 and exponent = 0x3ff, then the format
 * represents a normalized number in the range [1, 2).
 *
 * So, if we can manage to fill the 52 bits with random bits, we
 * will have a normalized random number in the range [1, 2). Then,
 * we subtract 1.0 and voila - we have a random number in the range
 * [0, 1.0).
 */

#include "utils/xorshift-rand.h"


double
frand()
{
    static xs128plus rs;
    static int init = 0;

    if (!init) {
        xs128plus_init(&rs, 0);
        init = 1;
    }

    union {
        double   d;
        uint64_t v;
    } un;

    uint64_t r = xs128plus_u64(&rs) & ~0xfff0000000000000;

    un.d  = 1.0;
    un.v |= r;

    return un.d - 1.0;
}

