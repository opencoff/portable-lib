/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * machdep.h - machine specific macro goo.
 *
 * Copyright (c) 2004, 2005, 2006 Sudhi Herle <sw at herle.net>
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
 * Creation date: Sun May 23 14:36:03 2004
 */

#ifndef __UTILS_MACHDEP_H_1156361763__
#define __UTILS_MACHDEP_H_1156361763__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#if defined(__little_endian__) && defined(__big_endian__)
  #error "** Can't define both big _and_ little endian!"
#endif

/* Do autodetect magic iff one of these are NOT defined. */
#if !(defined(__little_endian__) && defined(__big_endian__))


#if defined(__i386__) || defined(__MINGW32__) || defined(_MSC_VER) \
        || defined(WIN32) || defined(_WIN32)

  #define __little_endian__ 1

#elif defined(__mips__) || defined(__mips64) || defined(mips) || defined(__mips)

  #if defined(MIPSEB) || defined(__MIPSEB__)  || defined(_MIPSEB) || defined(__MIPSEB)
    #define __big_endian__    1
  #elif defined(MIPSEL) || defined(__MIPSEL__)  || defined(_MIPSEL) || defined(__MIPSEL)
    #define __little_endian__ 1
  #else
    #error "** Unknown mips endian ordering?!"
  #endif /* mips EB vs. EL */

#elif defined(__arm__)

  #if defined(__ARMEB__)
    #define __big_endian__    1
  #elif defined(__ARMEL__)
    #define __little_endian__ 1
  #else
    #error "** Unknown ARM endian ordering?!"
  #endif /* arm EB vs. EL*/

#endif /* arch-auto-detect */


#if defined(__x86_64__)
  #define __little_endian__ 1
#endif

#if defined(__sparc__)
  #define __big_endian__    1
#endif /* sparc */

#endif /* neither little or big are defined */


#ifdef __little_endian__

  #define __LITTLE_ENDIAN_BITFIELD 1
  #undef  __BIG_ENDIAN_BITFIELD
  #undef  __big_endian__

#elif defined(__big_endian__)

  #define __BIG_ENDIAN_BITFIELD 1
  #undef  __LITTLE_ENDIAN_BITFIELD
  #undef  __little_endian__

#else

  #warning "Unable to auto-detect endianness"
  #error "** Please define __little_endian__ OR __big_endian__ appropriately"

#endif /* little vs. big. */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __UTILS_MACHDEP_H_1156361763__ */

/* EOF */
