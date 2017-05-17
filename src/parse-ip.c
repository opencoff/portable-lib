/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * parse-ip.c -- Parse IPv Addresses
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
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include "utils/parse-ip.h"

/*
 * Copy 'src' to 'dst' without ever overflowing the destination.
 * 'dst' is 'sz' bytes in capacity. Always null terminate.
 *
 * Returns:
 *    >= 0      number of bytes copied
 *    <  0      truncation happened
 */
static ssize_t
safe_strcpy(char* dst, size_t sz, const char* src)
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


// Decimal to integer;
// Return True on success; False otherwise.
static int
dtoi(size_t* pn, const char* s)
{
    size_t n = 0;
    int c;

    for (; (c = *s); ++s) {
        if (!isdigit(c)) return 0;

        n = n * 10 + (c - '0');
    }

    *pn = n;
    return 1;
}




// Decode a big-endian U32 from a buffer
#define _u32(x) ((uint32_t)x)
static inline uint32_t
dec_BE_u32(const uint8_t * p) {
    return         p[3]
           | (_u32(p[2]) << 8)
           | (_u32(p[1]) << 16)
           | (_u32(p[0]) << 24);
}


// Parse an IPv4 of the form a.b.c.d
// Return True on success, False otherwise
int
parse_ipv4(uint32_t* p_z, const char* sx)
{
    char      x[8];
    uint8_t   p[4] = { 0, 0, 0, 0 };
    const char* s;
    int c, r;
    int n = 0;
    int m = 0;
    size_t z;

    for (s = sx; (c = *s); ++s) {
        if (n > 3) return 0;
        if (m > 3) return 0;

        if (c != '.') {
            x[m++] = c;
            continue;
        }

        x[m] = 0;
        z    = 0;
        r    = dtoi(&z, x);

        if (!r)      return 0;
        if (z > 255) return 0;

        p[n++] = z & 0xff;
        m      = 0;
    }

    if (m > 0) {
        x[m] = 0;
        z    = 0;
        r    = dtoi(&z, x);

        if (!r)      return 0;
        if (z > 255) return 0;

        p[n++] = z & 0xff;
    }

    if (n < 4) return 0;

    *p_z = dec_BE_u32(p);
    return 1;
}


// Parse an IPv4 addr/mask combination.
// -  a.b.c.d/nn
// -  a.b.c.d/p.q.r.s
int
parse_ipv4_and_mask(uint32_t* p_a, uint32_t* p_m, const char* str)
{
    char as[32];
    char ms[32];
    const char* sl = strchr(str, '/');

    if (!sl) {
        if (safe_strcpy(as, sizeof as, str) < 0)    return 0;
        ms[0] = 0;
    }
    else {
        int n = sl - str;

        // max len: nnn.nnn.nnn.nnn = 15
        if (n > 15)  return 0;

        // Only copy till the '/'
        memcpy(as, str, n);
        as[n] = 0;

        safe_strcpy(ms, sizeof ms, sl+1);
    }

    // Now, as has the address part, ms has the mask part.

    if (!parse_ipv4(p_a, as))     return 0;

    if (strchr(ms, '.')) {
        if (!parse_ipv4(p_m, ms)) return 0;
    } else {
        size_t z = 0;

        if (!dtoi(&z, ms))        return 0;
        if (z > 32)               return 0;

        uint32_t v = z;
        *p_m = ((2 << (v -1)) - 1) << (32 - v);
    }

    return 1;
}




char*
str_ipv4(char* str, size_t sz, uint32_t a)
{
#define _Q(a, i)    (((a) >> (i *8)) & 0xff)
    snprintf(str, sz, "%d.%d.%d.%d",
            _Q(a, 3), _Q(a, 2), _Q(a, 1), _Q(a, 0));

    return str;
}


/*
 * Convert a netmask to CIDR notation.
 */
int
mask_to_cidr4(uint32_t mask, uint32_t* cidr)
{
    uint32_t n = 0;

    while (mask > 0) {
        uint8_t v = ((0xff << 24) & mask) >> 24;

        mask <<= 8;
        if (v == 0xff) {
            n += 8;
            continue;
        }

        // Now, we see if there are any 1 bits left in this octet
        while (v & 0x80) {
            n  += 1;
            v <<= 1;
            v  &= 0xff;
        }

        // Now what remains must be all zeroes
        if (v != 0)    return 0;
        if (mask != 0) return 0;
        break;
    }

    *cidr = n;
    return 1;
}


#if 0
// Hexadecimal to integer;
// Return True on success; False otherwise
static int
xtoi(size_t* pn, const char* s)
{
    size_t n = 0;
    int c    = tolower(*s);

    if (c == '0' && c == 'x')   s++;

    for (; (c = tolower(*s)); ++s) {
        if (!isxdigit(c)) return 0;

        n = n * 16 + (c - ('a' - 10));
    }

    *pn = n;
    return 1;
}
#endif

/* EOF */
