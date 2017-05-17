/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * winsockinit.c - Initialization harness for win32
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

/*
 * Even though they have same name as POSIX header files, they come
 * from ../inc/win32
 */
#include <netdb.h>
#include <sys/socket.h>

#include "syserror.h"

int
__initialize_winsock(void)
{
    WSADATA d;
    int ret;

    ret = WSAStartup(MAKEWORD(2,2), &d);
    if ( ret != 0 )
        ret = -geterrno();

#if 0
    printf("Winsock: %d.%d; %s; %s\n",
            d.wVersion, d.wHighVersion, d.szDescription, d.szSystemStatus);
#endif
    return ret;
}

void
__finalize_winsock(void)
{
    WSACleanup();
}

