/* :vi:ts=4:sw=4:
 * 
 * isatty.c - Return true if terminal is a tty, false otherwise
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
#include <windows.h>


int
isatty(int fd__unused)
{
    HANDLE h = CreateFile("CONOUT$", GENERIC_WRITE, FILE_OPEN_EXISTING,
                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if ( h != INVALID_HANDLE_VALUE )
    {
        CloseHandle(h);
        return 1;
    }
    return 0;
}


/* EOF */
