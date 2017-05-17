/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * inetutils.c - support for win32 sockets
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
 * Creation date: Sun Sep 11 12:36:52 2005
 */

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>

/*
 * Even though they have same name as POSIX header files, they come
 * from ../inc/win32
 */
#include <netdb.h>
#include <sys/socket.h>

/*
 * Win32 seems to be missing these two functions that are prevalent
 * in POSIX systems.
 */

int
inet_aton(const char * ip, struct in_addr * a)
{
    unsigned long x = inet_addr(ip);
    if ( x == INADDR_NONE )
        return 0;

    a->s_addr = x;
    return 1;
}


char *
inet_ntop(int family, const void * addr, char * buf, int bufsize)
{
    const unsigned char * p = (const unsigned char *)addr;
    if ( family == AF_INET )
    {
        int len = snprintf(buf, bufsize, "%d.%d.%d.%d",
                                p[0], p[1], p[2], p[3]);
        if (len >= bufsize)
        {
            errno = ENOSPC;
            return 0;
        }

        return buf;
    }

    errno = EAFNOSUPPORT;
    return 0;
}

/* EOF */
