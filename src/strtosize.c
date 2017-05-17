/*
 * Convert a string with a size suffix into a number.
 *
 * We only deal with units in multiples of 1024
 */

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef _MSC_VER
// MS is weird. They deliberately choose NOT to use function names that
// the rest of the world uses.
#define strtoull(a,b,c)  _strtoui64(a,b,c)
#define _ULLCONST(n) n##ui64
#else

#define _ULLCONST(n) n##ULL

#endif
#define UL_MAX__   _ULLCONST(18446744073709551615)

int
strtosize(const char* str, int base, uint64_t* ret)
{
    char * endptr = 0;
    uint64_t mult = 1;
    uint64_t v    = strtoull(str, &endptr, base);

#define KB_  _ULLCONST(1024)
#define MB_  (KB_ * 1024)
#define GB_  (MB_ * 1024)
#define TB_  (GB_ * 1024)
#define PB_  (TB_ * 1024)
#define EB_  (PB_ * 1024)


    if ( endptr && *endptr )
    {
        switch (*endptr)
        {
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
    }

    if ( v == UL_MAX__ && errno == ERANGE )
        return -ERANGE;

    if (ret)
        *ret = v * mult;
    return 0;
}


