/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * quickbits.h - simple minded implementation of bitset. Good for
 * tracking bitset of chars.
 *
 * Copyright (c) 2005-2007 Sudhi Herle <sw at herle.net>
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

#ifndef __QUICKBITS_H_1175264337__
#define __QUICKBITS_H_1175264337__ 1

#include <string.h>

namespace putils {

/**
 * Delimiter bitset. It is fixed to 256 bits - to accomodate all
 * ASCII chars in the range 0--255.
 */
struct delim_bitset
{
    delim_bitset()  {}
    ~delim_bitset() {}


    // To keep compatibility with bitset<>
    void reset() { memset(bits, 0, sizeof bits); }

    // Set bit #i in this obj
    void set(unsigned int i)
    {
        i &= 0xff;
        unsigned int bit = i % sizeof(unsigned long);
        unsigned long* p = &bits[i / sizeof(unsigned long)];

        *p |= (1 << bit);
    }


    // use like so 'if (obj[i])' to check for i'th bit set in 'obj'
    bool operator[](unsigned int i) const
    {
        i &= 0xff;
        unsigned int bit       = i % sizeof(unsigned long);
        const unsigned long* p = &bits[i / sizeof(unsigned long)];

        return !!((1 << bit) & *p);
    }


    unsigned long bits[256 / sizeof(unsigned long)];
};

} // namespace putils

#endif /* ! __QUICKBITS_H_1175264337__ */

/* EOF */
