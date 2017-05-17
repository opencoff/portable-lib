/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * socket.h - Win32 compatibility header file
 *
 * (c) 2005 Sudhi Herle <sw at herle.net>
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
 *
 * Creation date: Sun Sep 11 12:58:43 2005
 */

#ifndef __SOCKET_H_1126461523__
#define __SOCKET_H_1126461523__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <winsock2.h>
#include <errno.h>

#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif

#ifndef SOL_TCP
#define SOL_TCP   IPPROTO_TCP
#endif


#ifndef SHUT_RD
#define SHUT_RD     SD_RECEIVE
#endif


#ifndef SHUT_WR
#define SHUT_WR     SD_SEND
#endif


#ifndef SHUT_RDWR
#define SHUT_RDWR    SD_BOTH
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT    MSG_PARTIAL
#endif



// Win32 doesn't provide this. Very strange.
struct iovec
{
    void *iov_base;   /* Starting address */
    size_t iov_len;   /* Number of bytes */
};


/*
 * Compatibility errno definitions
 */
#define EAFNOSUPPORT    WSAEAFNOSUPPORT
#define ENETUNREACH     WSAENETUNREACH


// This is missing on Win32 systems
typedef int socklen_t;


/**
 * socketpair(2) implementation for win32.
 *
 * The socketpair() call creates an unnamed pair of connected sockets in
 * the specified domain 'dom', of the specified 'typ', and using the
 * optionally specified protocol.  The descriptors used in referencing
 * the new sockets are returned in fd[0] and fd[1]. The two sockets are
 * indistinguishable.
 *
 * @param dom     Must be AF_UNIX or AF_LOCAL. On Win32, this param
 *                is ignored.
 * @param typ     Must be SOCK_STREAM or SOCK_DGRAM
 * @param proto   Can be 0 or IPPROTO_UDP, IPPROTO_TCP
 *
 * @result        fd[2] - the return fds for the connected sockets.
 *
 * @return    0 if no error
 *           -1 if some error happened, errno is set appropriately.
 */
extern int socketpair(int dom, int typ, int proto, int fd[2]);


/**
 * Helper routine to initialize WinSock library.
 *
 * @return   0   if no error
 *          -err on error
 */
int __initialize_winsock(void);



/**
 * Helper routine to finalize WinSock library.
 *
 * @return   0   if no error
 *          -err on error
 */
void __finalize_winsock(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __SOCKET_H_1126461523__ */

/* EOF */
