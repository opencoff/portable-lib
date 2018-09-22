/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/pack.h - Portable pack/unpack via template strings.
 *
 * Copyright (c) 2018 Sudhi Herle <sw at herle.net>
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

#ifndef ___UTILS_PACK_H__ESciMwzPvGQFbpd3___
#define ___UTILS_PACK_H__ESciMwzPvGQFbpd3___ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h>

/*
 * Pack takes a template string 'fmt' describing subsequent arguments
 * and packs them into a byte buffer 'buf' of capacity 'bufsize'.
 *
 * The template string is one or more type descriptors optionally
 * separated by white space. Each type descriptor describes the
 * encoding type and size of the argument.
 *
 * A type descriptor char can be preceded by an integral repeat
 * count. e.g., "4B".
 *
 * Each type descriptor (along with its repeat count) consumes
 * exactly _one_ argument in the var-arg list. i.e., "4S" is NOT the
 * same as "SSSS"; the former consumes exactly one var-arg of type
 * 'uint16_t*' while the latter consumes 4 arguments each of type
 * 'uint16_t'.

 * An optional first character in the template describes the
 * encoding endianness (default: little endian):
 *
 *   <    -- little endian encoding
 *   >    -- big endian encoding
 *
 * The type descriptors are:
 *
 *   b, B -- char, unsigned byte
 *   s, S -- short, unsigned short (16 bit)
 *   i, I -- int, unsigned int (32 bit)
 *   l, L -- long, unsigned long (64 bit)
 *   f, d -- float, double
 *   z    -- char* string
 *   x    -- pad byte (zeroes in the packed  stream; unconsumed arg
 *           while unpacking)
 *   *    -- treat the next argument as a count (32 bits) for
 *           whatever element is being encoded/decoded. While
 *           decoding, this count is returned in a uint32_t* arg.
 *
 * Types and Arguments:
 * ====================
 * o Individual type descriptors ("B", "I", etc.) require a
 *   corresponding argument of the appropriate C type (uint8_t,
 *   uint32_t etc.).
 * o Type descriptors with integral prefixes ("4B", "4I" etc.)
 *   require corresponding arguments of pointer types (uint8_t*,
 *   uint32_t* etc.). This applies ONLY to integral count of greater
 *   than 1.
 *
 * Notes:
 * ======
 * o A repeat-count of 'n' in front of 's' implies no more than n
 *   chars are encoded/decoded.
 *
 * o Template chars can be space separated. e.g., the following are
 *   valid pack/unpack templates:
 *
 *      ">B 4I 4L"
 *      "16B * L 32S"
 *
 * Return values:
 *    > 0:  Number of bytes in the encoded stream.
 *    < 0:  -errno on failure:
 *          -EINVAL invalid type descriptor string
 *          -ENOSPC input buffer is too small to encode all types
 *          -ENOENT unknown type descriptor
 *          -E2BIG  repeat count is larger than 32 bits
 */
ssize_t Pack(uint8_t *buf, size_t bsize, const char *fmt, ...);

/*
 * Unpack takes a template string 'fmt' describing subsequent arguments
 * and unpacks them from a byte buffer 'buf' of capacity 'bufsize'.
 * The template string syntax is exactly the same as Pack() above.
 *
 * In order to return decoded values, the input arguments *must* all
 * be pointers to their respective types.
 *
 * For type descriptors with integral count prefix, the pointers
 * should point to memory to accommodate the required number of
 * bytes.
 *
 * Whenever a wild-card '*' is used before a type descriptor while
 * unpacking, the subsequent argument must be a double pointer of
 * the appropriate type. e.g.,::
 *
 *      uint32_t n = 0;
 *      uint32_t *va = 0;
 *      Unpack(in, insize, "* I", &n, &va);
 *
 * In the above call, when Unpack() returns successfully, 'va' will
 * point to a dynamically allocated array of 'n' uint32_t.
 *
 * For type 's', Unpack() allocates a char string of the appropriate
 * size and unpacks into the newly allocated area. Thus, the
 * argument to 's' should be "char **".
 *
 * Return values:
 *    > 0:  Number of bytes decoded
 *    < 0:  -errno on failure:
 *          -EINVAL nil pointer encountered for a type or arg or
 *                  no '\0' found for unpacking string type
 *          -ENOSPC input buffer is too small to decode all types
 *          -ENOENT unknown type descriptor
 *          -ENOMEM out of memory
 *          -E2BIG  repeat count is larger than 32 bits
 */
ssize_t Unpack(uint8_t *buf, size_t bsize, const char *fmt, ...);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_PACK_H__ESciMwzPvGQFbpd3___ */

/* EOF */
