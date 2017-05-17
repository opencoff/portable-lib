/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * pack.c - Perl pack/unpack API
 *
 * Copyright (c) 1999-2010 raf <raf@raf.org>
 * Copyright (c) 2011-2015 Sudhi Herle <sw at herle.net>
 *
 * History
 * =======
 *   - Inspired by the Perl pack()/unpack() functions.
 *   - Originally authored by raf <raf@raf.org> as part of
 *     libslack (net.c)
 *
 *   - Extracted into standalone, portable version by
 *     Sudhi Herle.
 *
 * Licensing Terms: GPLv2 (same as original libslack/net.c)
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 *
 * Usage
 * =====
 *
 * Packs data into buf as described by fmt. The arguments after fmt
 * contain the data to be packed. size is the size of buf. Returns the
 * number of bytes packed on success, or -1 on error with errno set
 * appropriately.
 *
 * Note, this is based on the pack() function in perl(1) (in fact, the
 * following documentation is from perlfunc(1)) except that the * count
 * specifier has different semantics, the ? count specifier is new, there's
 * no non nul-terminated strings or machine dependant formats or uuencoding
 * or BER integer compression, everything is in network byte order, and floats
 * are represented as strings so pack() is suitable for serialising data to
 * be written to disk or sent across a network to other hosts. OK, v and
 * w specifically aren't in network order but sometimes that's needed too.
 *
 * fmt can contain the following type specifiers:
 *
 * a    A string with arbitrary binary data
 * z    A nul terminated string, will be nul padded
 * h    A hex string (rounded out to nearest byte boundary)
 *      A hex string is of the form "37AD5b7cf102eeFD..."
 *
 * S    A 16-bit short in big-endian order (network byte order)
 * I    A 32-bit int/long in big-endian order (network byte order)
 * L    A 64-bit long long in big-endian order (network byte order)
 *
 * s    A 16-bit short in little-endian order
 * i    A 32-bit int/long in little-endian order
 * l    A 64-bit long long in little-endian order
 *
 * x    A nul byte
 * X    Back up a byte
 * @    Null fill to absolute position
 *
 * Not implemented:
 * ----------------
 * c    A char (8 bits)
 * f    A single-precision float (length byte + text + nul)
 * d    A double-precision float (length byte + text + nul)
 *
 * The following rules apply:
 *
 * Each letter may optionally be followed by a number giving a repeat count or
 * length, or by "*" or "?". A "*" will obtain the repeat count or
 * length from the next argument (like printf(3)). The count argument must
 * appear before the first corresponding data argument. When unpacking "a",
 * "z", or "h", a "?" will obtain the repeat count or length
 * from the size_t object pointed to by the next argument and the size of
 * the target buffer in the argument after that. These two arguments must
 * appear before the first corresponding target buffer argument. This enables
 * unpacking packets that contain length fields without risking target buffer
 * overflow.
 *
 * With the types "a", "z", "h", 's', 'i', 'l', the pack() and unpack()
 * functions will treat the argument as an array of "count"
 * elements.
 *
 * The "a", "z", "s", "i", "l", "p", types gobble just one value, but pack it as
 * a string of length count (specified by the corresponding number), truncating or
 * padding with nuls as necessary. It is the caller's responsibility to
 * ensure that the data arguments point to sufficient memory. When unpacking,
 * "z" strips everything after the first nul, and "a" returns data
 * verbatim.
 *
 * The "h" field packs a string that many nybbles long.
 *
 * WARNING
 * =======
 * It is the caller's responsibility to ensure that there are sufficient
 * arguments provided to satisfy the requirements of fmt.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>

#include "utils/pack.h"

#define ERR(a)  (errno = (a), -(a))




ssize_t
pack(void *buf, size_t size, const char *fmt, ...)
{
	va_list args;
	int rc;

	va_start(args, fmt);
	rc = vpack(buf, size, fmt, args);
	va_end(args);

	return rc;
}


#define GET_COUNT() do {\
	count = 1; \
	if (*fmt == '*') { \
		++fmt; \
        count = va_arg(args, size_t); \
    }\
	else if (isdigit((int)(unsigned int)*fmt)) { \
		for (count = 0; isdigit((int)(unsigned int)*fmt); ++fmt) {\
			count *= 10; \
            count += *fmt - '0'; \
        } \
    } \
	if ((ssize_t)count < 1) \
		return ERR(EINVAL); \
} while (0)

#define CHECK_SPACE(required) do { \
	if ((p + (required)) > (pkt + size)) \
		return ERR(ENOSPC); \
} while (0)

ssize_t
vpack(void *buf, size_t size, const char *fmt, va_list args)
{
    size_t count;
    unsigned char *pkt = buf;
    unsigned char *p = pkt;
    //char tmp[128];

    if (!pkt || !fmt)
        return ERR(EINVAL);

    while (*fmt)
    {
        switch (*fmt++)
        {
            case ' ':   /* readbility spacing */
                continue;

            case 'a': /* A string with arbitrary binary data */
                {
                    void *data;
                    GET_COUNT();
                    CHECK_SPACE(count);
                    if (!(data = va_arg(args, void *)))
                        return ERR(EINVAL);
                    memcpy(p, data, count);
                    p += count;
                    break;
                }

            case 'z': /* A nul terminated string, will be nul padded */
                {
                    char *data;
                    size_t len;
                    GET_COUNT() ;
                    if (!(data = va_arg(args, char *)))
                        return ERR(EINVAL);

                    len = strlen(data)+1;
                    len = len > count ? count : len;

                    memcpy(p, data, len);
                    p += len;
                    count -= len;
                    if (count > 0) {
                        memset(p, 0, count);
                        p += count;
                    }
                    break;
                }


            case 'h': /* A hex string (rounded out to nearest byte boundary) */
                {
                    char *data;
                    unsigned char byte;
                    int shift;
                    GET_COUNT();
                    CHECK_SPACE((count + 1) >> 1);
                    if (!(data = va_arg(args, char *)))
                        return ERR(EINVAL);
                    byte = 0x00;
                    shift = 4;
                    while (count--)
                    {
                        unsigned char nybble = *data++;
                        switch (nybble)
                        {
                            case '0': case '1': case '2': case '3': case '4':
                            case '5': case '6': case '7': case '8': case '9':
                                byte |= (nybble - '0') << shift;
                                break;
                            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                                byte |= (nybble - 'a' + 10) << shift;
                                break;
                            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                                byte |= (nybble - 'A' + 10) << shift;
                                break;
                            default:
                                return ERR(EINVAL);
                        }
                        if ((shift -= 4) == -4)
                        {
                            *p++ = byte;
                            byte = 0x00;
                            shift = 4;
                        }
                    }
                    if (shift != 4)
                        *p++ = byte;
                    break;
                }


            case 's': /* A LE short (16 bits) */
                {
                    GET_COUNT();
                    CHECK_SPACE(count << 1);
                    if (count == 1) {
                        uint16_t data = 0xffff & ((int)va_arg(args, int));
                        *p++ = data & 0xff;
                        *p++ = (data >> 8) & 0xff;
                    } else {
                        uint16_t *a = (uint16_t*)va_arg(args, uint16_t*);
                        while (count--) {
                            uint16_t data = *a++;
                            *p++ = data & 0xff;
                            *p++ = (data >> 8) & 0xff;
                        }
                    }

                    break;
                }

            case 'S': /* A BE 16 bit uint */
                {
                    GET_COUNT();
                    CHECK_SPACE(count << 1);
                    if (count == 1) {
                        uint16_t data = 0xffff & ((int)va_arg(args, int));
                        *p++ = (data >> 8) & 0xff;
                        *p++ = data & 0xff;
                    } else {
                        uint16_t *a = (uint16_t*)va_arg(args, uint16_t*);
                        while (count--) {
                            uint16_t data = *a++;
                            *p++ = (data >> 8) & 0xff;
                            *p++ = data & 0xff;
                        }
                    }

                    break;
                }


            case 'i': /* A LE 32 bit uint */
                {
                    GET_COUNT();
                    CHECK_SPACE(count << 2);
                    if (count == 1) {
                        uint32_t data = (uint32_t)va_arg(args, uint32_t);
                        *p++ = data & 0xff;
                        *p++ = (data >> 8) & 0xff;
                        *p++ = (data >> 16) & 0xff;
                        *p++ = (data >> 24) & 0xff;
                    } else {
                        uint32_t* a = (uint32_t*)va_arg(args, uint32_t*);
                        while (count--) {
                            uint32_t data = *a++;
                            *p++ = data & 0xff;
                            *p++ = (data >> 8) & 0xff;
                            *p++ = (data >> 16) & 0xff;
                            *p++ = (data >> 24) & 0xff;
                        }
                    }

                    break;
                }


            case 'I':  /* A BE 32 bit uint */
                {
                    GET_COUNT();
                    CHECK_SPACE(count << 2);
                    if (count == 1) {
                        uint32_t data = (uint32_t)va_arg(args, uint32_t);
                        *p++ = (data >> 24) & 0xff;
                        *p++ = (data >> 16) & 0xff;
                        *p++ = (data >> 8) & 0xff;
                        *p++ = data & 0xff;
                    } else {
                        uint32_t* a = (uint32_t*)va_arg(args, uint32_t*);
                        while (count--) {
                            uint32_t data = *a++;
                            *p++ = (data >> 24) & 0xff;
                            *p++ = (data >> 16) & 0xff;
                            *p++ = (data >> 8) & 0xff;
                            *p++ = data & 0xff;
                        }
                    }

                    break;
                }


            case 'l': /* A LE 64 bit uint */
                {
                    GET_COUNT();
                    CHECK_SPACE(count << 3);
                    if (count == 1) {
                        uint64_t data = (uint64_t)va_arg(args, uint64_t);
                        *p++ = data & 0xff;
                        *p++ = (data >> 8) & 0xff;
                        *p++ = (data >> 16) & 0xff;
                        *p++ = (data >> 24) & 0xff;
                        *p++ = (data >> 32) & 0xff;
                        *p++ = (data >> 40) & 0xff;
                        *p++ = (data >> 48) & 0xff;
                        *p++ = (data >> 56) & 0xff;
                    } else {
                        uint64_t* a = (uint64_t*)va_arg(args, uint64_t*);
                        while (count--) {
                            uint64_t data = *a++;
                            *p++ = data & 0xff;
                            *p++ = (data >> 8) & 0xff;
                            *p++ = (data >> 16) & 0xff;
                            *p++ = (data >> 24) & 0xff;
                            *p++ = (data >> 32) & 0xff;
                            *p++ = (data >> 40) & 0xff;
                            *p++ = (data >> 48) & 0xff;
                            *p++ = (data >> 56) & 0xff;

                        }
                    }

                    break;
                }

            case 'L': /* A BE 64 bit uint */
                {
                    GET_COUNT();
                    CHECK_SPACE(count << 3);
                    if (count == 1) {
                        uint64_t data = (uint64_t)va_arg(args, uint64_t);
                        *p++ = (data >> 56) & 0xff;
                        *p++ = (data >> 48) & 0xff;
                        *p++ = (data >> 40) & 0xff;
                        *p++ = (data >> 32) & 0xff;
                        *p++ = (data >> 24) & 0xff;
                        *p++ = (data >> 16) & 0xff;
                        *p++ = (data >> 8) & 0xff;
                        *p++ = data & 0xff;
                    } else {
                        uint64_t* a = (uint64_t*)va_arg(args, uint64_t*);
                        while (count--) {
                            uint64_t data = *a++;

                            *p++ = (data >> 56) & 0xff;
                            *p++ = (data >> 48) & 0xff;
                            *p++ = (data >> 40) & 0xff;
                            *p++ = (data >> 32) & 0xff;
                            *p++ = (data >> 24) & 0xff;
                            *p++ = (data >> 16) & 0xff;
                            *p++ = (data >> 8) & 0xff;
                            *p++ = data & 0xff;

                        }
                    }

                    break;
                }

#if 0
            case 'c': /* A char (8 bits) */
                {
                    GET_COUNT();
                    CHECK_SPACE(count);
                    while (count--)
                        *p++ = (unsigned char)va_arg(args, int);
                    break;
                }

            case 'f': /* A single-precision float (length byte + text + nul) */
            case 'd': /* A double-precision float (length byte + text + nul) */
                {
                    GET_COUNT();
                    while (count--)
                    {
                        double data = va_arg(args, double);
                        int rc = snprintf(tmp, 128, "%g", data);
                        size_t len;
                        if (rc == -1 || rc >= 128)
                            return ERR(ENOSPC);
                        len = strlen(tmp) + 1;
                        CHECK_SPACE(len + 1);
                        *p++ = len & 0xff;
                        memcpy(p, tmp, len);
                        p += len;
                    }

                    break;
                }
#endif


            case 'x': /* A nul byte */
                {
                    GET_COUNT();
                    CHECK_SPACE(count);
                    if (count > 1)
                        memset(p, 0, count);
                    else
                        *p = 0;
                    p += count;
                    break;
                }

            case 'X': /* Back up a byte */
                {
                    GET_COUNT();
                    if (p - count < pkt)
                        return ERR(EINVAL);
                    p -= count;
                    break;
                }

            case '@': /* Null fill to absolute position */
                {
                    GET_COUNT();
                    if (count > size)
                        return ERR(ENOSPC);
                    if (pkt + count < p)
                        return ERR(EINVAL);
                    memset(p, 0, count - (p - pkt));
                    p += count - (p - pkt);
                    break;
                }

            default:
                {
                    return ERR(EINVAL);
                }
        }
    }

    return p - pkt;
}



/*
 * Unpack data in 'buf' which was previously packed by pack(). 
 * 'size' is the size of buf
 * 'fmt' has same semantics as the one in pack()
 * Remaining arguments must be pointers to vars that will hold the
 * unpacked data or NULL.  If any pointers are NULL, then, the
 * corresponding data will be skipped.
 *
 * Returns: number of bytes unpacked.
 *
 * Note:
 *      Unpacked 'z', 'b', 'h' strings are _always_ nul terminated.
 *
 *      The caller is responsible to ensure that the output buffers
 *      contain enough memory.
 */


ssize_t
unpack(void *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    int rc;

    va_start(args, fmt);
    rc = vunpack(buf, size, fmt, args);
    va_end(args);

    return rc;
}




#define GET_COUNT_LIMIT()  do {\
	limit = count = 1; \
	if (*fmt == '*') { \
		++fmt; \
        limit = count = va_arg(args, size_t); \
    } else if (*fmt == '?') { \
		size_t *countp = va_arg(args, size_t *); \
		if (!countp) \
			return ERR(EINVAL); \
		count = *countp; \
		limit = va_arg(args, size_t); \
		++fmt; \
	} else if (isdigit((int)(unsigned int)*fmt)) \
	{ \
		for (count = 0; isdigit((int)(unsigned int)*fmt); ++fmt) {\
			count *= 10; \
            count += *fmt - '0'; \
        }\
		limit = count; \
	} \
	if ((ssize_t)count < 1 || (ssize_t)limit < 1) \
		return ERR(EINVAL); \
	if (count > limit) \
		return ERR(ENOSPC); \
} while (0)

#define CHECK_SKIP(count, action)  \
	if (!data) \
	{ \
		p += (count); \
		action; \
	} \
    (void)0

// Simple functional casts
#define _U32(x)     ((uint32_t)(x))
#define _U64(x)     ((uint64_t)(x))

ssize_t
vunpack(void *buf, size_t size, const char *fmt, va_list args)
{
    unsigned char *pkt = buf;
    unsigned char *p = pkt;
    size_t count, limit;

    if (!pkt || !fmt)
        return ERR(EINVAL);

    while (*fmt)
    {
        switch (*fmt++)
        {
            case ' ':   /* readbility spacing */
                continue;

            case 'a': /* A string with arbitrary binary data */
                {
                    void *data;
                    GET_COUNT_LIMIT();
                    CHECK_SPACE(count);
                    data = va_arg(args, void *);
                    CHECK_SKIP(count, break);
                    memcpy(data, p, count);
                    p += count;
                    break;
                }

            case 'z': /* A nul terminated string, will be nul padded */
                {
                    char *data;
                    size_t len = 0;
                    size_t rem = pkt + size - p;
                    GET_COUNT_LIMIT();
                    CHECK_SPACE(count);
                    data = va_arg(args, char *);
                    CHECK_SKIP(count, break);
                    for (;rem-- > 0; ++len)
                    {
                        if (!p[len])
                            break;
                    }
                    if (len > count)
                        len = count;
                    memcpy(data, p, len);
                    data  += len;
                    p     += len;
                    count -= len;
                    if (count > 0)
                        memset(data, 0, count);
                    p += count;
                    break;
                }


            case 'h': /* A hex string (rounded out to nearest byte boundary) */
                {
                    static const char hex[] = "0123456789abcdef";
                    char *data;
                    int shift;
                    GET_COUNT_LIMIT();
                    CHECK_SPACE((count + 1) >> 1);
                    data = va_arg(args, char *);
                    CHECK_SKIP((count + 1) >> 1, break);
                    shift = 4;
                    while (count--)
                    {
                        *data++ = hex[(*p & (0x0f << shift)) >> shift];
                        if ((shift -= 4) == -4)
                            ++p, shift = 4;
                    }
                    if (shift != 4)
                        ++p;
                    *data = '\0';
                    break;
                }


            case 's': /* A LE 16 bit uint */
                {
                    uint16_t* data = (uint16_t*)va_arg(args, uint16_t*);
                    GET_COUNT();
                    CHECK_SPACE(count << 1);
                    while (count--)
                    {
                        CHECK_SKIP(2, continue);
                        *data = p[0] | (((uint16_t)p[1]) << 8);
                        p    += 2;
                        data++;
                    }
                    break;
                }


            case 'i': /* A LE 32 bit uint */
                {
                    uint32_t* data = (uint32_t*)va_arg(args, uint32_t*);
                    GET_COUNT();
                    CHECK_SPACE(count << 2);
                    while (count--)
                    {
                        CHECK_SKIP(4, continue);
                        *data = p[0] | (_U32(p[1]) << 8)
                                     | (_U32(p[2]) << 16)
                                     | (_U32(p[3]) << 24);

                        p += 4;
                        data++;
                    }

                    break;
                }

            case 'I':   /* A BE 32 bit uint */
                {
                    uint32_t* data = (uint32_t*)va_arg(args, uint32_t*);
                    GET_COUNT();
                    CHECK_SPACE(count << 2);
                    while (count--)
                    {
                        CHECK_SKIP(4, continue);
                        *data = p[3] | (_U32(p[2]) << 8)
                                     | (_U32(p[1]) << 16)
                                     | (_U32(p[0]) << 24);
                        p += 4;
                        data++;
                    }

                    break;
                }


            case 'l': /* A LE 64 bit uint */
                {
                    uint64_t* data = (uint64_t*)va_arg(args, uint64_t*);
                    GET_COUNT();
                    CHECK_SPACE(count << 3);
                    while (count--)
                    {
                        CHECK_SKIP(8, continue);
                        *data = p[0] | (_U64(p[1]) << 8)
                                     | (_U64(p[2]) << 16)
                                     | (_U64(p[3]) << 24)
                                     | (_U64(p[4]) << 32)
                                     | (_U64(p[5]) << 40)
                                     | (_U64(p[6]) << 48)
                                     | (_U64(p[7]) << 56);

                        p += 8;
                        data++;
                    }

                    break;
                }

            case 'L': /* A BE 64 bit uint */
                {
                    uint64_t* data = (uint64_t*)va_arg(args, uint64_t*);
                    GET_COUNT();
                    CHECK_SPACE(count << 3);
                    while (count--)
                    {
                        CHECK_SKIP(8, continue);
                        *data = p[7] | (_U64(p[6]) << 8)
                                     | (_U64(p[5]) << 16)
                                     | (_U64(p[4]) << 24)
                                     | (_U64(p[3]) << 32)
                                     | (_U64(p[2]) << 40)
                                     | (_U64(p[1]) << 48)
                                     | (_U64(p[0]) << 56);

                        p += 8;
                        data++;
                    }

                    break;
                }

#if 0
            case 'c': /* A char (8 bits) */
                {
                    GET_COUNT();
                    CHECK_SPACE(count);
                    while (count--)
                    {
                        unsigned char *data = va_arg(args, unsigned char *);
                        CHECK_SKIP(1, continue);
                        *data = (unsigned char)*p++;
                    }
                    break;
                }

            case 'f': /* A single-precision float (length byte + text + nul) */
                {
                    GET_COUNT();
                    while (count--)
                    {
                        float *data = va_arg(args, float *);
                        size_t len;
                        CHECK_SPACE(1);
                        len = (size_t)*p++;
                        CHECK_SPACE(len);
                        CHECK_SKIP(len, continue);
                        sscanf((const char *)p, "%g", data);
                        p += len;
                    }

                    break;
                }

            case 'd': /* A double-precision float (length byte + text + nul) */
                {
                    GET_COUNT();
                    while (count--)
                    {
                        double *data = va_arg(args, double *);
                        size_t len;
                        CHECK_SPACE(1);
                        len = (size_t)*p++;
                        CHECK_SPACE(len);
                        CHECK_SKIP(len, continue);
                        sscanf((const char *)p, "%lg", data);
                        p += len;
                    }

                    break;
                }
#endif

            case 'x': /* A nul byte */
                {
                    GET_COUNT();
                    CHECK_SPACE(count);
                    p += count;
                    break;
                }

            case 'X': /* Back up a byte */
                {
                    GET_COUNT();
                    if (p - count < pkt)
                        return ERR(EINVAL);
                    p -= count;
                    break;
                }

            case '@': /* Null fill to absolute position */
                {
                    GET_COUNT();
                    if (count > size)
                        return ERR(ENOSPC);
                    if (pkt + count < p)
                        return ERR(EINVAL);
                    p += count - (p - pkt);
                    break;
                }

            default:
                {
                    return ERR(EINVAL);
                }
        }
    }

    return p - pkt;
}

