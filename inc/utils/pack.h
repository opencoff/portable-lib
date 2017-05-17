/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/pack.h - Perl like pack/unpack API
 *
 * Copyright (c) 1998-2011 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: Perl Artistic License
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * << This section is a direct copy from the perlfunc manpage >>
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
 * a   A string with arbitrary binary data
 * z   A nul terminated string, will be nul padded
 * b   A bit string (rounded out to nearest byte boundary)
 * h   A hex string (rounded out to nearest byte boundary)
 * c   A char (8 bits)
 * s   A short (16 bits)
 * i   An int (32 bits)
 * l   A long (32 bits)
 * q   A long long (64 bits)
 * p   A pointer (32/64 bits) - system dependent
 * f   A single-precision float (length byte + text + nul)
 * d   A double-precision float (length byte + text + nul)
 * x   A nul byte
 * X   Back up a byte
 * @   Null fill to absolute position
 *
 * The following rules apply:
 *
 * Each letter may optionally be followed by a number giving a repeat count or
 * length, or by "*" or "?". A "*" will obtain the repeat count or
 * length from the next argument (like printf(3)). The count argument must
 * appear before the first corresponding data argument. When unpacking "a",
 * "z", "b" or "h", a "?" will obtain the repeat count or length
 * from the size_t object pointed to by the next argument and the size of
 * the target buffer in the argument after that. These two arguments must
 * appear before the first corresponding target buffer argument. This enables
 * unpacking packets that contain length fields without risking target buffer
 * overflow.
 *
 * With all types except "a", "z", "b" and "h" the pack()
 * function will gobble up that many arguments.
 *
 * The "a" and "z" types gobble just one value, but pack it as a string
 * of length count (specified by the corresponding number), truncating or
 * padding with nuls as necessary. It is the caller's responsibility to
 * ensure that the data arguments point to sufficient memory. When unpacking,
 * "z" strips everything after the first nul, and "a" returns data
 * verbatim.
 *
 * Likewise, the "b" field packs a string that many bits long.
 *
 * The "h" field packs a string that many nybbles long.
 *
 * The "p" type packs a pointer. You are responsible for ensuring the memory
 * pointed to is not a temporary value (which can potentially get deallocated
 * before you get around to using the packed result). A null pointer is
 * unpacked if the corresponding packed value for "p" is null. Of course,
 * "p" is useless if the packed data is to be sent over a network to another
 * process.
 *
 * The integer formats "c", "s", "i" and "l" are all on network
 * byte order and so can safely be packed for sending over a network to another
 * process. However, "l" relies on a non-ISO C 89 language feature (namely,
 * the long long int type which is in ISO C 99) and so should not be used in
 * portable code, even if it is supported on the local system. There is no
 * guarantee that a long long packed on one system will be unpackable on
 * another. At least not until C99 is more widespread.
 * Real numbers (floats and doubles) are packed in text format. Due to the
 * multiplicity of floating point formats around, this is done to safely
 * transport real numbers across a network to another process.
 * 
 * It is the caller's responsibility to ensure that there are sufficient
 * arguments provided to satisfy the requirements of fmt.
 */

#ifndef ___UTILS_PACK_H_5470508_1313002187__
#define ___UTILS_PACK_H_5470508_1313002187__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdarg.h>

/*
 * All functions return:
 *  Success: Number of bytes written to or consumed in the input buffer.
 *  Failure:
 *     -ENOSPC: Not enough memory in the output buffer
 *     -EINVAL: Invalid value for format
 *     -ENOSYS: Implementation not available for specific type on
 *              _this_ system
 */
ssize_t pack(void* buf, size_t size, const char* fmt, ...);
ssize_t unpack(void* buf, size_t size, const char* fmt, ...);
ssize_t vpack(void *buf, size_t size, const char *fmt, va_list args);
ssize_t vunpack(void *buf, size_t size, const char *fmt, va_list args);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_PACK_H_5470508_1313002187__ */

/* EOF */
