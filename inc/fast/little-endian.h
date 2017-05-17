/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * little-endian.h - Portable byteorder macros for little-endian systems.
 *
 * Copyright (c) 2004-2008  Sudhi Herle <sw at herle.net>
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

/*
 * Inspired by similar facilities in the linux kernel.
 */

#ifndef __FAST_LITTLE_ENDIAN_H__
#define __FAST_LITTLE_ENDIAN_H__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

#define __constant_htonll(x) (__swab64((uint64_t)(x)))
#define __constant_htonl(x)  (__swab32((uint32_t)(x)))
#define __constant_ntohll(x) (__swab64((uint64_t)(x)))
#define __constant_ntohl(x)  (__swab32(((uint32_t)(x)))
#define __constant_htons(x)  (__swab16(((uint16_t)(x)))
#define __constant_ntohs(x)  (__swab16(((uint16_t)(x)))

#define __cpu_to_le16(x)     ((uint16_t)(x))
#define __cpu_to_le32(x)     ((uint32_t)(x))
#define __cpu_to_le64(x)     ((uint64_t)(x))

#define __cpu_to_be16(x)     ((uint16_t)__swab16((x)))
#define __cpu_to_be32(x)     ((uint32_t)__swab32((x)))
#define __cpu_to_be64(x)     ((uint64_t)__swab64((x)))

#define __le16_to_cpu(x)     ((uint16_t)(x))
#define __le32_to_cpu(x)     ((uint32_t)(x))
#define __le64_to_cpu(x)     ((uint64_t)(x))

#define __be16_to_cpu(x)     (__swab16((uint32_t)(x)))
#define __be32_to_cpu(x)     (__swab32((uint32_t)(x)))
#define __be64_to_cpu(x)     (__swab64((uint64_t)(x)))


/* Handy utility for 64 bit  type */
#ifndef htonll
#define htonll(x)            (__swab64((uint64_t)(x)))
#endif

#ifndef ntohll
#define ntohll(x)            (__swab64((uint64_t)(x)))
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __FAST_LITTLE_ENDIAN_H__ */

/* EOF */
