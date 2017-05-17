/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * sys/mman.h - Win32 implementation of Posix mmap(2) system call.
 *
 * Copyright (c) 2005-2008 Sudhi Herle <sw at herle.net>
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
 * Creation date: Thu Oct 20 11:05:35 2005
 */
#ifndef __SYS_MMAN_H__8755114121__
#define __SYS_MMAN_H__8755114121__ 1

#if !defined(_WIN32) && !defined(WIN32)
#error  "** Ach! This is meant for Win32 systems"
#endif

#include <sys/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROT_READ       (0x01)          /* page can be read */
#define PROT_WRITE      (0x02)          /* page can be written */
#define PROT_EXEC       (0x04)          /* page can be executed */
#define PROT_NONE       (0x00)          /* page can not be accessed */

#define MAP_PRIVATE     (0x02)          /* Changes are private */
#define MAP_ANONYMOUS   (0x20)          /* Ignore fd and offset parameters */
#define MAP_FIXED       (0x10)          /* Interpret addr exactly */
#define MAP_FAILED      ((void*)-1)


/** Map a chunk of file or anonymous memory and return a pointer to
 *  it.
 *
 *  @param addr     Recommended address for the mapping. It is
 *                  advised to keep this 0 - unless you know what you are
 *                  doing.
 *  @param len      Number of bytes of mapping desired.
 *  @param prot
 *  @param flags
 *  @param fd
 *  @param offset
 *
 *  @return MAP_FAILED  if the mapping failed; if so, errno is set
 *                      up to return cause of error.
 *  @return valid pointer to mapped segment of file (or anonymous
 *          memory)
 */
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);


/** Unmap a previously mapped segment.
 *
 *  @param addr The base address of mapped region. This must be same
 *              as the value returned by a corresponding mmap(2) call.
 *  @param len  In this implementation, this param is ignored.
 *
 *  @return 0   if no error
 *         -1   if addr is invalid or the system call fails. If so,
 *              errno will be set to appropriate value.
 */
int munmap(void *addr, size_t len);


/** Flush any dirty pages to disk.
 *
 *  @param addr The base address of mapped region. This must be same
 *              as the value returned by a corresponding mmap(2) call.
 *  @param len  The length of the mapped region to flush to disk.
 *              This length will be rounded up to next page boundary.
 *  @param flags In this implementation, this param is ignored.
 *
 *  @return 0   if no error
 *         -1   if addr is invalid or the system call fails. If so,
 *              errno will be set to appropriate value.
 */
int msync(void *addr, size_t len, int flags);


/** Pin one or more pages in RAM (i.e., prevent them from being
 *  swapped to disk)
 */
int mlock(void* addr, size_t len);

/** Unpin one or more pages that are currently pinned in RAM.
 *
 * @param   addr    base address of pages that must be unpinned.
 *                  This base address must be at a page boundary.
 *
 * @param   len     Number of bytes to unpin. This must be a
 *                  multiple of page size.
 */
int munlock(void* addr, size_t len);

#ifdef __cplusplus
}
#endif

#endif  /* !__SYS_MMAN_H__8755114121__ */
