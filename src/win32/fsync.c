/* vim: ts=4:sw=4:expandtab:
 *
 * fsync.c - Win32 implementation of fsync(2) and fdatasync(2)
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <errno.h>
#include <io.h>

#include "sys/fcntl.h"

int
fsync(int fd)
{
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    if ( INVALID_HANDLE_VALUE == h )
    {
        errno = EINVAL;
        return -1;
    }

    if (!FlushFileBuffers(h))
    {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int
fdatasync(int fd)
{
    return fsync(fd);
}

