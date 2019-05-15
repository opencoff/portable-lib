/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * strtosize.c -- Convert a string with a size suffix into a number.
 *
 * strtou64(): Copyright (c) OpenBSD Foundation
 * strtosize(): Copyright (c) 2013-2015 Sudhi Herle <sw at herle.net>
 *
 * The strtou64() is from OpenBSD libc; the strtosize() is mine.
 * Both are licensed under the original terms of strtou64().
 *
 * Licensing Terms: BSD
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Notes
 * =====
 * - We only deal with units in multiples of 1024
 */

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>

#include "utils/utils.h"

int
strtou64(const char *str, const char **endptr, int base, uint64_t *p_ret)
{
    const char *s = str;
    uint64_t val  = 0,
             mult = base;
    int c,
        neg = 0;

    assert(str);
    if (base < 0 || base > 36 || base == 1) return -EINVAL;

    /* skip leading whitespace */
    for (; (c = *s); s++) {
        if (!isspace(c)) goto start;
    }
    goto done;

start:
    if (c == '-') {
        s++;
        neg = 1;
    } else {
        if (c == '+') ++s;
        neg = 0;
    }

    c = *s;
    if ((base == 0 || base == 16) && c == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s   += 2;
        mult = 16;
    } else if (base == 0) {
        mult = c == '0' ? 8 : 10;
    }

    assert(mult);

    /*
     * We pre-compute the largest legal value that is possible
     * before addition of another digit leads to overflow. We call
     * this the "cutoff point" delineating legal vs. illegal
     * numbers.
     *
     * We do this by looking at the maximum value of uint64_t
     * divided by the base. And, then the last digit of the max-value
     * tells us the character beyond which the value will overflow.
     *
     * For example, let us suppose we are doing base 10:
     *   max = 18446744073709551615
     *
     *   cutoff = 1844674407370955161 (max / 10)
     *   cutchar = 5 (the last digit of max)
     */
#define _u(a)   ((uint64_t)(a))

    uint64_t cutoff  = _u(~0) / mult,
             cutchar = _u(~0) % mult;

    for (; (c = *s); s++) {
        if (isdigit(c))
            c -= '0';
        else if (isalpha(c))
            c -= isupper(c) ? 'A' - 10: 'a' - 10;
        else
            goto bad;

        if (_u(c) >= mult) break;

        if (val > cutoff || (val == cutoff && _u(c) > cutchar)) goto erange;

        val *= mult;
        val += c;
    }

    if (neg) val = -val;

done:
    if (endptr) *endptr = s;
    *p_ret = val;
    return 0;

erange:
    if (endptr) *endptr = s;
    return -ERANGE;

bad:
    if (endptr) *endptr = s;
    return -EINVAL;
}

int
strtosize(const char* str, int base, uint64_t* ret)
{
    const char * endptr = 0;
    uint64_t mult = 1;
    uint64_t v    = 0;
    int      r    = strtou64(str, &endptr, base, &v);

    if (r < 0) return r;

#define KB_  (1024ULL)
#define MB_  (KB_ * 1024)
#define GB_  (MB_ * 1024)
#define TB_  (GB_ * 1024)
#define PB_  (TB_ * 1024)
#define EB_  (PB_ * 1024)


    if (endptr && *endptr) {
        switch (*endptr++) {
            case ' ':
                break;

            case 'k': case 'K':
                mult = KB_;
                break;

            case 'M':
                mult = MB_;
                break;

            case 'G':
                mult = GB_;
                break;

            case 'T':
                mult = TB_;
                break;

            case 'P':
                mult = PB_;
                break;

            case 'E':
                mult = EB_;
                break;

            default:
                return -EINVAL;
        }

        switch (*endptr) {
            case 0:
            case 'B':
                break;
            case 'b':
                mult /= 8;
                break;
            default:
                if (!isspace(*endptr)) return -EINVAL;
        }
    }

    uint64_t res = v * mult;

    if (v > 0 && (res < v || res < mult)) return -ERANGE;

    if (ret) *ret = res;
    return 0;
}


