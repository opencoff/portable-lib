/* :vi:ts=4:sw=4:
 * 
 * win32.h - Win32 specific definitions
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
 * Creation date: Mon Jun 20 06:59:13 2005
 *
 */

#ifndef __WIN32_H_1119268753_97208__
#define __WIN32_H_1119268753_97208__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define SEP     '\\'
#define SEPSTR  "\\"


// Compatibility stat(2) macros for MSVC badness

#define __s_istype(mode, mask)  (((mode) & S_IFMT) == (mask))

#ifndef S_ISDIR
#define S_ISDIR(mode)    __s_istype((mode), S_IFDIR)
#endif

#ifndef S_ISCHR
#define S_ISCHR(mode)    __s_istype((mode), S_IFCHR)
#endif

#ifndef S_ISBLK
#define S_ISBLK(mode)    __s_istype((mode), S_IFBLK)
#endif

#ifndef S_ISREG
#define S_ISREG(mode)    __s_istype((mode), S_IFREG)
#endif


// Win32 doesn't support symlinks, fifo or sockets on filesystems
#undef S_ISLNK
#undef S_ISFIFO
#undef S_ISSOCK
#undef S_ISBLK

#define S_ISLNK(m)      0
#define S_ISFIFO(m)     0
#define S_ISSOCK(m)     0
#define S_ISBLK(m)      0


/* File mode protection bits */
#undef S_IRUSR
#undef S_IWUSR
#undef S_IXUSR
#undef S_IRWXU

#define S_IRUSR S_IREAD   /* Read by owner.  */
#define S_IWUSR S_IWRITE  /* Write by owner.  */
#define S_IXUSR S_IEXEC   /* Execute by owner.  */

/* Read, write, and execute by owner.  */
#define S_IRWXU (S_IREAD|S_IWRITE|S_IEXEC)

#define S_IRGRP (S_IRUSR >> 3)  /* Read by group.  */
#define S_IWGRP (S_IWUSR >> 3)  /* Write by group.  */
#define S_IXGRP (S_IXUSR >> 3)  /* Execute by group.  */
/* Read, write, and execute by group.  */
#define S_IRWXG (S_IRWXU >> 3)

#define S_IROTH (S_IRGRP >> 3)  /* Read by others.  */
#define S_IWOTH (S_IWGRP >> 3)  /* Write by others.  */
#define S_IXOTH (S_IXGRP >> 3)  /* Execute by others.  */
/* Read, write, and execute by others.  */
#define S_IRWXO (S_IRWXG >> 3)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __WIN32_H_1119268753_97208__ */

/* EOF */
