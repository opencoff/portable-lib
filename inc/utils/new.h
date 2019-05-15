/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/new.h - new/delete macros for C
 *
 * Copyright (c) 2019 Sudhi Herle <sw at herle.net>
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

#ifndef ___UTILS_NEW_H__9srMJVqD0uvg1HlG___
#define ___UTILS_NEW_H__9srMJVqD0uvg1HlG___ 1

#include <stdlib.h>
#include <string.h>

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Handy macro to allocate an instance of a particular type */
#define NEW(ty_)            (ty_ *)malloc(sizeof(ty_))

/* Macro to allocate an array of 'ty_' type elements. */
#define NEWA(ty_,n)         (ty_ *)malloc(sizeof (ty_) * (n))

/* Macro to allocate a zero initialized item of type 'ty_' */
#define NEWZ(ty_)           (ty_ *)calloc(1, sizeof (ty_))

/* Macro to allocate a zero initialized array of type 'ty_' items */
#define NEWZA(ty_,n)        (ty_ *)calloc((n), sizeof (ty_))

/* Macro to 'realloc' an array 'ptr' of items of type 'ty_' */
#define RENEWA(ty_,ptr,n)   (ty_ *)realloc(ptr, (n) * sizeof(ty_))

/* Crazy macro to pretend we use an arg */
#define USEARG(x)          (void)(x)

/* Corresponding macro to "free" an item obtained via NEWx macros.
 * */
#define DEL(ptr)             do { \
                                if (ptr) free(ptr); \
                                ptr = 0; \
                            } while (0)

/* Return the size of a initialized array of any type */
#define ARRAY_SIZE(a)       (sizeof(a) / sizeof(a[0]))


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_NEW_H__9srMJVqD0uvg1HlG___ */

/* EOF */
