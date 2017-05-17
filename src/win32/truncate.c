/* vim: ts=4:sw=4:expandtab:
 *
 * truncate.c - Win32 implementation of a similar POSIX function.
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

int
truncate (const char * file, unsigned long length)
{
    HANDLE h = CreateFile (file, GENERIC_WRITE,
                            FILE_SHARE_READ,
                            0,
                            OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            0
                );

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

    CloseHandle (h);
    return 0;
}

