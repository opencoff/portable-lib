/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * rand_marsaglia.cpp - Marsaglia Random Generator
 *
 * A Pseudo Random Number Generator of type Multiply with Carry.
 * Based on the k=2 PRNG described in
 *    http://www.ms.uky.edu/~mai/RandomNumber
 *
 * In a simple timing test on a 2.3GHz PowerPC G5, this code produced
 * approximately 130,000 random integers per second.
 *
 * Based on Python implementation by Mark Mitchell <visteya@visteya.net>
 *
 * Copyright (c) Sudhi Herle <sw at herle.net>
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Creation date: Thu Apr 30 22:11:32 2009
 */


#include "utils/random.h"
#include "utils/sysrand.h"


namespace putils {

static const unsigned long _Multipliers[] = {
    18000, 18030, 18273, 18513, 18879, 19074, 19098, 19164, 19215, 19584,
    19599, 19950, 20088, 20508, 20544, 20664, 20814, 20970, 21153, 21243,
    21423, 21723, 21954, 22125, 22188, 22293, 22860, 22938, 22965, 22974,
    23109, 23124, 23163, 23208, 23508, 23520, 23553, 23658, 23865, 24114,
    24219, 24660, 24699, 24864, 24948, 25023, 25308, 25443, 26004, 26088,
    26154, 26550, 26679, 26838, 27183, 27258, 27753, 27795, 27810, 27834,
    27960, 28320, 28380, 28689, 28710, 28794, 28854, 28959, 28980, 29013,
    29379, 29889, 30135, 30345, 30459, 30714, 30903, 30963, 31059, 31083,
};
#define _NMULT  ((sizeof _Multipliers)/(sizeof (_Multipliers[0])))


rand_marsaglia::rand_marsaglia(unsigned long z, unsigned long w, bool random)
    : randgen("marsaglia"),
      m_z(z),
      m_w(w)
{
    unsigned char buf[10];
    unsigned char * entropy = (unsigned char*)sys_randbytes(buf, sizeof buf);

    for(int i = 0; i < 4; ++i)
    {
        m_z <<= 8;
        m_z += entropy[i];

        m_w <<= 8;
        m_w += entropy[4+i];
    }

    if (random)
    {
        unsigned long im = entropy[8];
        m_zm = _Multipliers[im % _NMULT];
        m_wm = _Multipliers[(im+1) % _NMULT];
    }
    else
    {
        m_zm = 36969;
        m_wm = 18000;
    }
    
}


rand_marsaglia::~rand_marsaglia()
{
}

unsigned long
rand_marsaglia::generate_long()
{
    m_z = (m_zm * (m_z & 65535) + (m_z >> 16)) & 0xffffffff;
    m_w = (m_wm * (m_w & 65535) + (m_w >> 16)) & 0xffffffff;

    return ((m_z << 16) + (m_w & 65535)) & 0xffffffff;
}

} // namespace putils
