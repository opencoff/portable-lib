/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * fnvhash.c - Fowler, Noll, Vo Hash function.
 *
 * Copyright (c) 2005-2008 Sudhi Herle <sw at herle.net>
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
 * This file implements the Fowler, Noll, Vo hash function as
 * described here:
 *   http://www.isthe.com/chongo/tech/comp/fnv/
 *
 * In particular, this file implements the FNV-1a variation of the
 * hash function. 
 *
 * According to Noll, FNV-1a has slightly better dispersion for
 * small chunks of memory (< 4 octets).
 */

#include <stdint.h>
#include <sys/types.h>
#include "utils/utils.h"
#include "utils/hashfunc.h"

uint32_t
fnv_hash(const void* pdata, size_t len, uint32_t seed)
{
    const unsigned char* p = (const unsigned char*)pdata;
    uint32_t h = 2166136261U;
    size_t i;

    USEARG(seed);

    for (i = 0; i < len; ++i)
    {
        h ^= p[i];
#if 1
        h *= 16777619UL; /* this is 0x0100_0193 */
#else
        /*
         * If your system doesn't have a fast multipler, use this
         * part of the branch.
         * Most modern Xeons, Core-2, Core-i7 have very fast
         * multipliers.
         */
        h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
#endif

    }

    return h;
}


uint64_t
fnv_hash64(const void* pdata, size_t len, uint64_t seed)
{
    const unsigned char* p = (const unsigned char*)pdata;
    uint64_t h = 14695981039346656037ULL;
    size_t i;

    USEARG(seed);

    for (i = 0; i < len; ++i)
    {
        h ^= p[i];
#if 1 /* GCC optimization */
        h *= 1099511628211ULL;  /* this is 0x1000_0000_1b3 */
#else
        /*
         * If your system doesn't have a fast multipler, use this
         * part of the branch.
         * Most modern Opterons, Xeons, Core-2, Core-i7 have very fast
         * multipliers.
         */
        h += (h << 1) + (h << 4) + (h << 5) + (h << 7) + (h << 8) + (h << 40);
#endif
    }

    return h;
}

/* EOF */
