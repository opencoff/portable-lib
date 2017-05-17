    /* vim: expandtab:tw=68:ts=4:sw=4:
     *
     * t_cresolve.c - simple test harness to test the name resolver
     *                functionality.
     *
     * Copyright (C) 2005, Sudhi Herle <sw@herle.net>
     *
     * Licensing Terms: (See LICENSE.txt for details). In short:
     *    1. Free for use in non-commercial and open-source projects.
     *    2. If you wish to use this software in commercial projects,
     *       please contact me for licensing terms.
     *    3. By using this software in commercial OR non-commercial
     *       projects, you agree to send me any and all changes, bug-fixes
     *       or enhancements you make to this software.
     * 
     * This software does not come with any express or implied
     * warranty; it is provided "as is". No claim  is made to its
     * suitability for any purpose.
     *
     * Creation date: Sun Sep 11 11:33:33 2005
     */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "error.h"
#include "utils/resolve.h"
#include "utils/utils.h"







/* 
 * Usage: $0 [name] [name ..]
 *
 * Name can be an interface name or hostname. If no arguments are
 * passed, it enumerates all interfaces and all addresses bound to
 * each interface.
 */
int
main(int argc, char** argv)
{
    int i;

    program_name = argv[0];

    if (argc < 2)
    {
        if_address_vect av;
        if_address* ifa;
        int r;

        VECT_INIT(&av, 16);

        r = get_all_if_address(&av, F_LINK|F_INET|F_INET6);
        if (r < 0)
            error(1, -r, "Error obtaining interfaces");

        VECT_FOR_EACH(&av, ifa)
        {
            if_addr* z;
            char buf[64];

            printf("%s:\n", ifa->if_name);
            VECT_FOR_EACH(&ifa->if_addr, z)
            {
                printf("   %s\n", sockaddr_to_string(buf, sizeof buf, (struct sockaddr*)&z->sa));
            }
        }
        exit(0);
    }



    for (i = 1; i < argc; ++i)
    {
        if_addr_vect av;
        if_addr * z;
        char* host = argv[i];
        int r;
        char buf[64];

        VECT_INIT(&av, 16);

        r = resolve_host_or_ifname(host, &av, 0);
        if (r != 0)
        {
            error(0, -r, "Unable to resolve %s", host);
            continue;
        }

        printf("%s:\n", host);
        VECT_FOR_EACH(&av, z)
        {
            printf("  %s\n", sockaddr_to_string(buf, sizeof buf, (struct sockaddr*)&z->sa));
        }

        VECT_FINI(&av);
    }

    return 0;
}
// EOF
