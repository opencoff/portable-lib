/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * fcntl.h - Compat definitions for Win32
 *
 * Copyright (c) 2008 Sudhi Herle <sw at herle.net>
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

#ifndef __SYS_FCNTL_H_1221513547__
#define __SYS_FCNTL_H_1221513547__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Like sync, but flush contents of 'fd' to disk */
extern int fsync(int fd);

/* Sync data only */
extern int fdatasync(int fd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __SYS_FCNTL_H_1221513547__ */

/* EOF */
