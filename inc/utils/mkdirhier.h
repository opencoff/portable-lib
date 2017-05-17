/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * mkdirhier.h - "mkdir -p" in "C"
 *
 * Copyright (c) 2011 Sudhi Herle <sw at herle.net>
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

#ifndef ___MKDIRHIER_H_7048806_1317399773__
#define ___MKDIRHIER_H_7048806_1317399773__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <sys/types.h>
#include <limits.h>

/**
 * make a directory and all its necessary parents.
 *
 * Returns:
 *   0  on success
 *   -1 on error (sets errno)
 *
 * Errno:
 *    - ENAMETOOLONG: path is too long
 *    - NOTDIR: One of the intermediate components is NOT a
 *      directory
 */
extern int mkdirhier(const char* path, mode_t mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___MKDIRHIER_H_7048806_1317399773__ */

/* EOF */
