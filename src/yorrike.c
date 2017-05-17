/**
 *
 * Derivative of FNV1A from [http://www.sanmayce.com/Fastest_Hash/]
 */

#include <stdint.h>
#include <stdlib.h>

#define _OFF32 2166136261
#define _P32   16777619
#define _YP32  709607


#define WORDSZ      4
#define D_WORDSZ    (WORDSZ << 1)       // 8
#define DD_WORDSZ   (WORDSZ << 2)       // 16
#define DDD_WORDSZ  (WORDSZ << 3)       // 32

/*
 * Needed for unaligned u64 decode.
 */
#define _u64(x) ((uint64_t)x)
#define _u32(x) ((uint64_t)x)
static inline uint64_t
dec_LE_u64(const uint8_t * p) {
    return         p[0]
           | (_u64(p[1]) << 8)
           | (_u64(p[2]) << 16)
           | (_u64(p[3]) << 24)
           | (_u64(p[4]) << 32)
           | (_u64(p[5]) << 40)
           | (_u64(p[6]) << 48)
           | (_u64(p[7]) << 56);
}

static inline uint32_t
dec_LE_u32(const uint8_t * p) {
    return         p[0]
           | (_u32(p[1]) << 8)
           | (_u32(p[2]) << 16)
           | (_u32(p[3]) << 24);
}

uint32_t
yorrike_hash32(const void* x, size_t n, uint32_t seed)
{
    uint32_t h32  = seed ? seed : _OFF32,
             h32b = h32;
    const uint8_t* buf = (const uint8_t*)x;
    uint64_t k1, k2;

    while (n >= DDD_WORDSZ) {
        k1   = dec_LE_u64(buf);
        k2   = dec_LE_u64(buf+4);
        h32  = _u32((_u64(h32) ^ ((k1<<5 | k1>>27) ^ k2)) * _YP32);

        k1   = dec_LE_u64(buf+8);
        k2   = dec_LE_u64(buf+12);
        h32b = _u32((_u64(h32b) ^ ((k1<<5 | k1>>27) ^ k2)) * _YP32);

        buf += DDD_WORDSZ;
        n   -= DDD_WORDSZ;
    }


    if ((n & DD_WORDSZ) > 0) {
        k1   = dec_LE_u64(buf);
        k2   = dec_LE_u64(buf+4);
        h32  = _u32((_u64(h32)  ^ k1) * _YP32);
        h32b = _u32((_u64(h32b) ^ k2) * _YP32);

        buf += DD_WORDSZ;
    }


    if ((n & D_WORDSZ) > 0) {
        uint32_t w1, w2;
        w1   = dec_LE_u32(buf);
        w2   = dec_LE_u32(buf+2);
        h32  = (h32  ^ w1) * _YP32;
        h32b = (h32b ^ w2) * _YP32;

        buf += D_WORDSZ;
    }


    if ((n & WORDSZ) > 0) {
        uint32_t w1;
        w1   = dec_LE_u32(buf);
        h32  = (h32  ^ w1) * _YP32;

        buf += WORDSZ;
    }

    if ((n & 1) > 0) {
        uint32_t w1 = _u32(*buf);

        h32  = (h32  ^ w1) * _YP32;
    }

    h32 = (h32 ^ ((h32b << 5) | (h32b >> 27))) * _YP32;
    return h32 ^ (h32 >> 16);
}

/* EOF */
