/* vim: ts=4:sw=4:expandtab:
 *
 * ftruncate.c - Win32 implementation of a similar POSIX function.
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
 * Creation date: Jan 22, 2006
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <errno.h>

int
ftruncate(int fd, unsigned long length)
{
    HANDLE h = (HANDLE)_get_osfhandle(fd);

    if ( INVALID_HANDLE_VALUE == h )
    {
        errno = ENOENT;
        return -1;
    }

    if ( SetFilePointer (h, length, 0, FILE_BEGIN) == 0xffffffff )
    {
        errno = EINVAL;
        return -1;
    }

    if ( ! SetEndOfFile (h) )
    {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

