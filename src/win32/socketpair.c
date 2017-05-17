/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * socketpair.c - Socketpair implementation for win32.
 *
 * Copyright (c) 2007 Sudhi Herle <sw at herle.net>
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

/*
 * Even though they have same name as POSIX header files, they come
 * from ../inc/win32
 */
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <windows.h>

#include "syserror.h"

static int
bind_easy(SOCKET s, struct sockaddr_in* addr)
{
    int addr_size = sizeof *addr;

    memset(addr, 0, sizeof addr);
    addr->sin_family      = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr->sin_port        = 0;  /* system assigned port# */

    if (0 != bind(s, (struct sockaddr *)addr, addr_size))
        goto fail;

    /* Get the port# to which we are bound */
    if (0 != getsockname(s, (struct sockaddr *)addr, &addr_size))
        goto fail;

    return 0;

fail:
    return -geterrno();
}



int
socketpair(int dom, int typ, int proto, int sv[])
{
    struct sockaddr_in addr0, addr1;
    int addr_size = sizeof addr0;
    int err = 0;
    SOCKET s0, s1;

    /*
     * For win32, dom=AF_UNIX cannot be implemented. Hence, we
     * always ignore it.
     */
    dom = dom;
    s0  = socket(AF_INET, typ, proto);
    if (INVALID_SOCKET == s0)
    {
        errno = geterrno();
        goto fail0;
    }

    err = bind_easy(s0, &addr0);
    if (0 != err)
        goto fail1;


    if (typ != SOCK_DGRAM)
    {
        if (0 != listen(s0, 5))
        {
            err = geterrno();
            goto fail1;
        }
    }


    /* Second endpoint */
    s1 = socket(AF_INET, typ, 0);
    if (INVALID_SOCKET == s1)
    {
        err = geterrno();
        goto fail1;
    }

    if (typ == SOCK_DGRAM)
    {
        err = bind_easy(s1, &addr1);
        if (0 != err)
            goto fail2;
    }

    /* Connect s1 to s0 */
    if (0 != connect(s1, (struct sockaddr *)&addr0, addr_size))
    {
        err = geterrno();
        goto fail2;
    }


    /*
     * We now need to connect s0 and s1. But, TCP and UDP are
     * slightly different. For UDP, we need to connect() on both
     * sockets. But, for TCP the accept() performs similar function.
     *
     *    - s1 -> s0: common to UDP
     *    - s0 -> s1: do connect() for UDP, accept() for TCP.
     */

    if (typ == SOCK_DGRAM)
    {
        /* Connect s0 to s1 */
        addr_size = sizeof addr0;
        if (0 != connect(s0, (struct sockaddr *)&addr1, addr_size))
        {
            err = geterrno();
            goto fail2;
        }
    }
    else
    {
        SOCKET s2;

        if (INVALID_SOCKET == (s2 = accept(s0, 0, 0)))
        {
            err = geterrno();
            goto fail2;
        }

        /* Close listening socket; we don't need it now. */
        closesocket(s0);

        s0 = s2;
    }

    /* Now, s0 and s1 are connected. */

    sv[0] = (int)s1;
    sv[1] = (int)s0;


    return 0;

fail2:
    closesocket((SOCKET)sv[0]);

fail1:
    closesocket(s0);

fail0:
    errno = err;
    return -1;
}

/* EOF */
