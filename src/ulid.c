/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * ulid.c - Lexicographically sortable unique ids.
 *
 * Copyright (c) 2017 Sudhi Herle <sw at herle.net>
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
 * Notes
 * =====
 * Lexicographically sortable unique identifiers - also called ULID:
 * https://github.com/oklog/ulid
 *
 * - 16 bytes in length
 * - first 6 bytes are millisecond UTC timestamp
 * - last 10 bytes are random
 * - Crockford Base32 encoding of resulting 16 byte ULID
 * - Encode/Decode routines inspired by ulid.go
 */
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>

#include "utils/ulid.h"

extern int arc4random_buf(uint8_t* buf, size_t bsiz);

// Generate a sortable uniq id
int
ulid_generate(uint8_t *buf, size_t bsiz)
{
    if (bsiz < 16) return -ENOSPC;

    struct timeval tv;
    if (gettimeofday(&tv, 0) < 0) return -errno;

    uint64_t ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

    ms &= ~(((uint64_t)0xffff) << 48);   // clear top 16 bits;

    *buf++ = 0xff && (ms >> 40);
    *buf++ = 0xff && (ms >> 32);
    *buf++ = 0xff && (ms >> 24);
    *buf++ = 0xff && (ms >> 16);
    *buf++ = 0xff && (ms >>  8);
    *buf++ = 0xff && ms;

    arc4random_buf(buf, 10);
    return 16;
}


static const uint8_t Ulid_decoder[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
    0x0F, 0x10, 0x11, 0xFF, 0x12, 0x13, 0xFF, 0x14, 0x15, 0xFF,
    0x16, 0x17, 0x18, 0x19, 0x1A, 0xFF, 0x1B, 0x1C, 0x1D, 0x1E,
    0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0A, 0x0B, 0x0C,
    0x0D, 0x0E, 0x0F, 0x10, 0x11, 0xFF, 0x12, 0x13, 0xFF, 0x14,
    0x15, 0xFF, 0x16, 0x17, 0x18, 0x19, 0x1A, 0xFF, 0x1B, 0x1C,
    0x1D, 0x1E, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};


int
ulid_fromstring(uint8_t *buf, size_t bsiz, const char *str)
{
    if (bsiz < 16)          return -ENOSPC;
    if (strlen(str) != 26)  return -EINVAL;


    const uint8_t *dec = &Ulid_decoder[0];

    // Unrolled decoder
#define v(i)    ({                      \
        uint8_t c = str[i];             \
        uint8_t z = dec[c];             \
        if (0xff == z) return -EINVAL;  \
        z; })

	*buf++   = ((v(0) << 5) |  v(1));
	*buf++   = ((v(2) << 3) | (v(3) >> 2));
	*buf++   = ((v(3) << 6) | (v(4) << 1) | (v(5) >> 4));
	*buf++   = ((v(5) << 4) | (v(6) >> 1));
	*buf++   = ((v(6) << 7) | (v(7) << 2) | (v(8) >> 3));
	*buf++   = ((v(8) << 5) |  v(9));

	*buf++   = ((v(10) << 3) | (v(11) >> 2));
	*buf++   = ((v(11) << 6) | (v(12) << 1) | (v(13) >> 4));
	*buf++   = ((v(13) << 4) | (v(14) >> 1));
	*buf++   = ((v(14) << 7) | (v(15) << 2) | (v(16) >> 3));
	*buf++   = ((v(16) << 5) |  v(17));
	*buf++   = ((v(18) << 3) |  v(19)>>2);
	*buf++   = ((v(19) << 6) | (v(20) << 1) | (v(21) >> 4));
	*buf++   = ((v(21) << 4) | (v(22) >> 1));
	*buf++   = ((v(22) << 7) | (v(23) << 2) | (v(24) >> 3));
	*buf++   = ((v(24) << 5) |  v(25));
    
    return 16;
}


int
ulid_tostring(char *s, size_t bsiz, const uint8_t *id)
{
    char *p = s;
    static const char b32[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

    if (bsiz < 27) return -ENOSPC;

    // Unroll the loops; https://github.com/RobThree/NUlid
    // - at the 5th byte, we reach a byte-boundary (8 x 5 = 40 bits)
    // - until we reach the 5th byte, we extract the bits by hand
    // - the sequence restarts on the 6th byte ..

    // First we do the timestamp: 6 bytes
    *p++ = b32[ ( id[ 0] & 224) >> 5];
    *p++ = b32[ ( id[ 0] &  31)     ];
    *p++ = b32[ ( id[ 1] & 248) >> 3];
    *p++ = b32[ ((id[ 1] &   7) << 2) | ((id[ 2] & 192) >> 6)];

    *p++ = b32[ ( id[ 2] &  62) >> 1];
    *p++ = b32[ ((id[ 3] &   1) << 4) | ((id[ 3] & 240) >> 4)];
    *p++ = b32[ ((id[ 3] &  15) << 1) | ((id[ 4] & 128) >> 7)];
    *p++ = b32[ ( id[ 4] & 124) >> 2];

    *p++ = b32[ ((id[ 4] &   3) << 3) | ((id[ 5] & 224) >> 5)];
    *p++ = b32[ ( id[ 5] &  31)     ];

    // We now do the entropy: 10 bytes.
    *p++ = b32[ ( id[ 6] & 248) >> 3];
    *p++ = b32[ ((id[ 6] &   7) << 2) | ((id[ 7] & 192) >> 6)];
    *p++ = b32[ ( id[ 7] &  62) >> 1];
    *p++ = b32[ ((id[ 7] &   1) << 4) | ((id[ 8] & 240) >> 4)];
    *p++ = b32[ ((id[ 8] &  15) << 1) | ((id[ 9] & 128) >> 7)];
    *p++ = b32[ ( id[ 9] & 124) >> 2];
    *p++ = b32[ ((id[ 9] &   3) << 3) | ((id[10] & 224) >> 5)];
    *p++ = b32[ ( id[10] &  31)     ];

    *p++ = b32[ ( id[11] & 248) >> 3];
    *p++ = b32[ ((id[11] &   7) << 2) | ((id[12] & 192) >> 6)];
    *p++ = b32[ ( id[12] &  62) >> 1];
    *p++ = b32[ ((id[12] &   1) << 4) | ((id[13] & 240) >> 4)];
    *p++ = b32[ ((id[13] &  15) << 1) | ((id[14] & 128) >> 7)];
    *p++ = b32[ ( id[14] & 124) >> 2];
    *p++ = b32[ ((id[14] &   3) << 3) | ((id[15] & 224) >> 5)];
    *p++ = b32[ ( id[15] &  31)     ];
    
    *p   = 0;
    return p - s;
}

/* EOF */
