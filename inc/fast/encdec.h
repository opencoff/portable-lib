/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * encdec.h - Portable LE/BE encoding for 16/32/64 bit qty
 *
 * Copyright (c) 2014 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2  or MIT
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#ifndef ___ENCDEC_H_9036021_1416800315__
#define ___ENCDEC_H_9036021_1416800315__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stddef.h>


/*
 * Encoding Routines for 16, 32, 48 bit quantities
 */

static inline uint8_t*
enc_LE_u16(uint8_t* buf, uint16_t v) {
    *buf++ = 0xff & v;
    *buf++ = 0xff & (v >> 8);
    return buf;
}

static inline uint8_t*
enc_BE_u16(uint8_t* buf, uint16_t v) {
    *buf++ = 0xff & (v >> 8);
    *buf++ = 0xff & v;
    return buf;
}


static inline uint8_t*
enc_LE_u32(uint8_t* buf, uint32_t v) {
    *buf++ = 0xff & v;
    *buf++ = 0xff & (v >> 8);
    *buf++ = 0xff & (v >> 16);
    *buf++ = 0xff & (v >> 24);
    return buf;
}

static inline uint8_t*
enc_BE_u32(uint8_t* buf, uint32_t v) {
    *buf++ = 0xff & (v >> 24);
    *buf++ = 0xff & (v >> 16);
    *buf++ = 0xff & (v >> 8);
    *buf++ = 0xff & v;
    return buf;
}


static inline uint8_t*
enc_LE_u64(uint8_t* buf, uint64_t v) {
    *buf++ = 0xff & v;
    *buf++ = 0xff & (v >> 8);
    *buf++ = 0xff & (v >> 16);
    *buf++ = 0xff & (v >> 24);
    *buf++ = 0xff & (v >> 32);
    *buf++ = 0xff & (v >> 40);
    *buf++ = 0xff & (v >> 48);
    *buf++ = 0xff & (v >> 56);
    return buf;
}

static inline uint8_t*
enc_BE_u64(uint8_t* buf, uint64_t v) {
    *buf++ = 0xff & (v >> 56);
    *buf++ = 0xff & (v >> 48);
    *buf++ = 0xff & (v >> 40);
    *buf++ = 0xff & (v >> 32);
    *buf++ = 0xff & (v >> 24);
    *buf++ = 0xff & (v >> 16);
    *buf++ = 0xff & (v >> 8);
    *buf++ = 0xff & v;
    return buf;
}


static inline uint8_t*
enc_LE_float64(uint8_t *buf, double d)
{
    union {
        double d;
        uint64_t v;
    } un;

    un.d = d;
    return enc_LE_u64(buf, un.v);
}


static inline uint8_t*
enc_BE_float64(uint8_t *buf, double d)
{
    union {
        double d;
        uint64_t v;
    } un;

    un.d = d;
    return enc_BE_u64(buf, un.v);
}



/*
 * Decoding routines for 16, 32, 64 bit quantities
 */
#define _u16(x) ((uint16_t)x)
#define _u32(x) ((uint32_t)x)
#define _u64(x) ((uint64_t)x)

static inline uint16_t
dec_LE_u16(const uint8_t * p) {
    return p[0] | (_u16(p[1]) << 8);
}


static inline uint16_t
dec_BE_u16(const uint8_t * p) {
    return p[1] | (_u16(p[0]) << 8);
}


static inline uint32_t
dec_LE_u32(const uint8_t * p) {
    return         p[0]
           | (_u32(p[1]) << 8)
           | (_u32(p[2]) << 16)
           | (_u32(p[3]) << 24);
}


static inline uint32_t
dec_BE_u32(const uint8_t * p) {
    return         p[3]
           | (_u32(p[2]) << 8)
           | (_u32(p[1]) << 16)
           | (_u32(p[0]) << 24);
}

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


static inline uint64_t
dec_BE_u64(const uint8_t * p) {
    return         p[7]
           | (_u64(p[6]) << 8)
           | (_u64(p[5]) << 16)
           | (_u64(p[4]) << 24)
           | (_u64(p[3]) << 32)
           | (_u64(p[2]) << 40)
           | (_u64(p[1]) << 48)
           | (_u64(p[0]) << 56);
}


static inline double
dec_LE_float64(uint8_t *p)
{
    union {
        double d;
        uint64_t v;
    } un;

    un.v = dec_LE_u64(p);
    return un.d;
}


static inline double
dec_BE_float64(uint8_t *p)
{
    union {
        double d;
        uint64_t v;
    } un;

    un.v = dec_BE_u64(p);
    return un.d;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___ENCDEC_H_9036021_1416800315__ */

/* EOF */
