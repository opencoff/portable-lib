/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * siphash24.c - Siphash24 implementation.
 *
 * Original code Copyright (c) 2013  Marek Majkowski <marek@popcount.org>
 * Streaming interface Copyright (c) 2013 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 or MIT
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

/* <MIT License>
 
 Copyright (c) 2013  Marek Majkowski <marek@popcount.org>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 </MIT License>

 Original location:
    https://github.com/majek/csiphash/

 Solution inspired by code from:
    Samuel Neves (supercop/crypto_auth/siphash24/little)
    djb (supercop/crypto_auth/siphash24/little2)
    Jean-Philippe Aumasson (https://131002.net/siphash/siphash24.c)
*/

#include <stdint.h>
#include "utils/siphash.h"

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
        __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define _le64toh(x) ((uint64_t)(x))
#elif defined(_WIN32)
/* Windows is always little endian, unless you're on xbox360
   http://msdn.microsoft.com/en-us/library/b0084kay(v=vs.80).aspx */
#  define _le64toh(x) ((uint64_t)(x))
#elif defined(__APPLE__)
#  include <libkern/OSByteOrder.h>
#  define _le64toh(x) OSSwapLittleToHostInt64(x)
#else

/* See: http://sourceforge.net/p/predef/wiki/Endianness/ */
#  if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#    include <sys/endian.h>
#  else
#    include <endian.h>
#  endif
#  if defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
        __BYTE_ORDER == __LITTLE_ENDIAN
#    define _le64toh(x) ((uint64_t)(x))
#  else
#    define _le64toh(x) le64toh(x)
#  endif

#endif


#define ROTATE(x, b) (uint64_t)( ((x) << (b)) | ( (x) >> (64 - (b))) )

#define HALF_ROUND(a,b,c,d,s,t)                 \
        a += b; c += d;                         \
        b = ROTATE(b, s) ^ a;                   \
        d = ROTATE(d, t) ^ c;                   \
        a = ROTATE(a, 32);

#define DOUBLE_ROUND(v0,v1,v2,v3)               \
        HALF_ROUND(v0,v1,v2,v3,13,16);          \
        HALF_ROUND(v2,v1,v0,v3,17,21);          \
        HALF_ROUND(v0,v1,v2,v3,13,16);          \
        HALF_ROUND(v2,v1,v0,v3,17,21);


#if 0
uint64_t
siphash24(const void *src, unsigned long src_sz, const char key[16])
{
    const uint64_t *_key = (uint64_t *)key;
    uint64_t k0 = _le64toh(_key[0]);
    uint64_t k1 = _le64toh(_key[1]);
    uint64_t b = (uint64_t)src_sz << 56;
    const uint64_t *in = (uint64_t*)src;


    /* Init */
    uint64_t v0 = k0 ^ 0x736f6d6570736575ULL;
    uint64_t v1 = k1 ^ 0x646f72616e646f6dULL;
    uint64_t v2 = k0 ^ 0x6c7967656e657261ULL;
    uint64_t v3 = k1 ^ 0x7465646279746573ULL;


    /* Compute */

    while (src_sz >= 8)
    {
        uint64_t mi = _le64toh(*in);
        in += 1; src_sz -= 8;
        v3 ^= mi;
        DOUBLE_ROUND(v0,v1,v2,v3);
        v0 ^= mi;
    }

    uint64_t t = 0; uint8_t *pt = (uint8_t *)&t; uint8_t *m = (uint8_t *)in;
    switch (src_sz)
    {
        case 7: pt[6] = m[6];
        case 6: pt[5] = m[5];
        case 5: pt[4] = m[4];
        case 4: *((uint32_t*)&pt[0]) = *((uint32_t*)&m[0]); break;
        case 3: pt[2] = m[2];
        case 2: pt[1] = m[1];
        case 1: pt[0] = m[0];
    }
    b |= _le64toh(t);

    v3 ^= b;
    DOUBLE_ROUND(v0,v1,v2,v3);
    v0 ^= b;


    /* Finish */

    v2 ^= 0xff;
    DOUBLE_ROUND(v0,v1,v2,v3);
    DOUBLE_ROUND(v0,v1,v2,v3);
    return (v0 ^ v1) ^ (v2 ^ v3);
}
#endif


/*
 * Streaming Implementation
 */


void
siphash24_init(siphash24_state* st, const uint64_t* key)
{
    uint64_t k0  = _le64toh(key[0]);
    uint64_t k1  = _le64toh(key[1]);


    st->v0 = k0 ^ 0x736f6d6570736575ULL;
    st->v1 = k1 ^ 0x646f72616e646f6dULL;
    st->v2 = k0 ^ 0x6c7967656e657261ULL;
    st->v3 = k1 ^ 0x7465646279746573ULL;
}

void
siphash24_update(siphash24_state* st, const void* src, size_t src_sz)
{
    uint64_t b = ((uint64_t)src_sz) << 56;
    const uint64_t *in = (uint64_t*)src;

    uint64_t v0 = st->v0;
    uint64_t v1 = st->v1;
    uint64_t v2 = st->v2;
    uint64_t v3 = st->v3;


    while (src_sz >= 8)
    {
        uint64_t mi = _le64toh(*in);
        in += 1; src_sz -= 8;
        v3 ^= mi;
        DOUBLE_ROUND(v0,v1,v2,v3);
        v0 ^= mi;
    }

    uint64_t t = 0; uint8_t *pt = (uint8_t *)&t; uint8_t *m = (uint8_t *)in;
    switch (src_sz)
    {
        case 7: pt[6] = m[6]; // fallthrough
        case 6: pt[5] = m[5]; // fallthrough
        case 5: pt[4] = m[4]; // fallthrough
        case 4: *((uint32_t*)&pt[0]) = *((uint32_t*)&m[0]); break;
        case 3: pt[2] = m[2]; // fallthrough
        case 2: pt[1] = m[1]; // fallthrough
        case 1: pt[0] = m[0]; // fallthrough
    }
    b |= _le64toh(t);

    v3 ^= b;
    DOUBLE_ROUND(v0,v1,v2,v3);
    v0 ^= b;

    st->v0 = v0;
    st->v1 = v1;
    st->v2 = v2;
    st->v3 = v3;
}


uint64_t
siphash24_finish(siphash24_state* st)
{
    uint64_t v0 = st->v0;
    uint64_t v1 = st->v1;
    uint64_t v2 = st->v2;
    uint64_t v3 = st->v3;

    v2 ^= 0xff;
    DOUBLE_ROUND(v0,v1,v2,v3);
    DOUBLE_ROUND(v0,v1,v2,v3);
    return (v0 ^ v1) ^ (v2 ^ v3);
}


/* one shot */
uint64_t
siphash24(const void* src, size_t src_len, const uint64_t* key)
{
    siphash24_state st;

    siphash24_init(&st, key);
    siphash24_update(&st, src, src_len);
    return siphash24_finish(&st);
}

/* EOF */
