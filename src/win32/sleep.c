/* vim: ts=4:sw=4:expandtab:
 *
 * sleep.c - POSIX sleep(3) implementation for win32
 *
 * Copyright (c) 2005, 2006 Sudhi Herle <sw at herle.net>
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
sleep(int s)
{
    DWORD ms = s * 1000;

    Sleep(ms);
    return 0;
}
