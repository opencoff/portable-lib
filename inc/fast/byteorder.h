/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * byteorder.h - Portable byteorder macros.
 *
 * Ideas borrowed from linux kernel.
 *
 * Copyright (c) 2004,2005,2006  Sudhi Herle <sw at herle.net>
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
 * Creation date: May 11 23:49:50 2004
 */

#ifndef __FAST_BYTEORDER_H__
#define __FAST_BYTEORDER_H__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include "fast/swab.h"

#include "utils/machdep.h"

#ifdef __big_endian__
#include "fast/big-endian.h"
#elif defined(__little_endian__)
#include "fast/little-endian.h"
#else
#error "** neither __big_endian__ nor __little_endian__ defined!"
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __FAST_BYTEORDER_H__ */

/* EOF */
