/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * sysrand.h - Portable interface to system random number generator
 *
 * Copyright (c) 2005 Sudhi Herle <sw at herle.net>
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

#ifndef __UTILS_SYSRAND_H_1127158677__
#define __UTILS_SYSRAND_H_1127158677__ 1

#ifdef __cplusplus
extern "C" {
#endif

/** Return a buffer full of random bytes */
extern void * sys_entropy(void * buf, size_t nbytes);


#ifdef __cplusplus
}
#endif


#endif /* ! __UTILS_SYSRAND_H_1127158677__ */

/* EOF */
