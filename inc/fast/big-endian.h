/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * big-endian.h - Portable byteorder macros for big endian systems.
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

#ifndef __FAST_BIG_ENDIAN_H__
#define __FAST_BIG_ENDIAN_H__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

#define __constant_htonll(x) ((uint64_t)(x))
#define __constant_htonl(x)  ((uint32_t)(x))
#define __constant_ntohll(x) ((uint64_t)(x))
#define __constant_ntohl(x)  ((uint32_t)(x))
#define __constant_htons(x)  ((uint16_t)(x))
#define __constant_ntohs(x)  ((uint16_t)(x))

#define __cpu_to_le32(x)     ((uint32_t)__swab32((x)))
#define __cpu_to_le64(x)     ((uint64_t)__swab64((x)))
#define __cpu_to_be32(x)     ((uint32_t)(x))
#define __cpu_to_be64(x)     ((uint64_t)(x))

#define __le32_to_cpu(x)     (__swab32((uint32_t)(x)))
#define __le64_to_cpu(x)     (__swab64((uint64_t)(x)))
#define __be32_to_cpu(x)     ((uint32_t)(x))
#define __be64_to_cpu(x)     ((uint64_t)(x))


#ifndef htonll
#define htonll(x)   ((uint64_t)(x))
#endif

#ifndef ntohll
#define ntohll(x)   ((uint64_t)(x))
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __FAST_BIG_ENDIAN_H__ */

/* EOF */
