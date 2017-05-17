/* vim: tw=4:sw=4:expandtab:
 *
 * dirent.h - POSIX directory management for Win32
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

#ifndef __WIN32__DIRENT_H__
#define __WIN32__DIRENT_H__

#include <sys/types.h>	/* ino_t definition */

#define	rewinddir(dirp)	seekdir(dirp, 0L)

#define	MAXNAMLEN	1023

#ifdef __cplusplus
extern "C" {
#endif


struct dirent
{
    ino_t d_ino;			/* a bit of a farce */
    int d_reclen;			/* more farce */
    int d_namlen;			/* length of d_name */
    char d_name[MAXNAMLEN+1];
};


typedef struct _dirdesc DIR;

extern DIR *opendir(const char *);
extern void closedir(DIR *);
extern struct dirent *readdir(DIR *);

extern void seekdir(DIR *, long);
extern long telldir(DIR *);

#ifdef __cplusplus
}
#endif

#endif  /* __WIN32__DIRENT_H__ */
