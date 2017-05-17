/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * netdb.h - compat header file for win32
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
 * Creation date: Sun Sep 11 13:19:53 2005
 */

#ifndef __NETDB_H_1126462793__
#define __NETDB_H_1126462793__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <winsock2.h>

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN     16
#endif


extern int inet_aton(const char * ipaddr, struct in_addr *);
extern char * inet_ntop(int family, const void * addr, char * buf, int bufsize);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __NETDB_H_1126462793__ */

/* EOF */
