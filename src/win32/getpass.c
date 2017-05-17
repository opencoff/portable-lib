/* vim: ts=4:sw=4:expandtab:
 *
 * getpass.c - Win32 implementation of a similarly named POSIX
 *             function.
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

#include <stdio.h>
#include <windows.h>

char *
getpass (char * passwd, int len, const char *prompt)
{
    size_t i;

    fputs (prompt, stderr);
    fflush (stderr);
    for (i = 0; i < len - 1; ++i)
    {
        passwd[i] = _getch ();
        if (passwd[i] == '\r')
            break;
    }
    passwd[i] = '\0';
    fputs ("\n", stderr);
    return passwd;
}

/* EOF */
