/* :vi:ts=4:sw=4:
 * 
 * mtrand.h - 
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
 *
 * Creation date: Mon Sep 19 08:35:34 2005
 */

#ifndef __MTRAND_H_1127136934_64313__
#define __MTRAND_H_1127136934_64313__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


extern void seedrand(unsigned long);

extern unsigned long genrand(void);

extern double dgenrand(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __MTRAND_H_1127136934_64313__ */

/* EOF */
