/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/ulid.h - Lexicographically sortable Unique Ids.
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
 */

#ifndef ___UTILS_ULID_H__cKkRChOvt3FAqxV8___
#define ___UTILS_ULID_H__cKkRChOvt3FAqxV8___ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <sys/types.h>

/*
 * Generate a random UUID and put it into 'bsiz' bytes sized 'buf'.
 *
 * Returns on failure:
 *  -EINVAL     if bsiz is 0 or buf is null
 *  -ENOSPC     if bsiz < 16
 *
 * Returns on success:
 *  Number of bytes of UUID (16).
 */
extern int ulid_generate(uint8_t *buf, size_t bsiz);


/*
 * Parse a string representation of UUID in 'str' and write it to
 * 'buf'; 'bsiz' is the capacity of 'buf'.
 *
 * Returns on failure:
 *  -ENOSPC     if bsiz  < 16
 *  -EINVAL     if str != 36 or any of the coded chars are invalid
 *
 * Returns on success:
 *  Number of bytes written to 'buf' (16).
 */
extern int ulid_fromstring(uint8_t *buf, size_t bsiz, const char *str);

/*
 * Convert a 16-byte UUID in 'buf' to string 'str'; 'str' is at most
 * 'bsiz' bytes big.
 *
 * Returns on failure:
 *  -ENOSPC     if bsiz  < 37
 *
 * Returns on success:
 *  Number of bytes written to 'buf'
 */
extern int ulid_tostring(char *str, size_t bsiz, const uint8_t *buf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_ULID_H__cKkRChOvt3FAqxV8___ */

/* EOF */
