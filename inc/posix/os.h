/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * posix/os.h - OS specific stuff for POSIX like OSes
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
 * Creation date: Mon Sep 12 11:27:00 2005
 */

#ifndef __POSIX_OS_H_1126542420__
#define __POSIX_OS_H_1126542420__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <errno.h>

/* Error codes */
enum
{
      ERR_PERM = EPERM
    , ERR_NOENT = ENOENT
    , ERR_SRCH = ESRCH
    , ERR_INTR = EINTR
    , ERR_IO = EIO
    , ERR_NXIO = ENXIO
    , ERR_2BIG = E2BIG
    , ERR_NOEXEC = ENOEXEC
    , ERR_BADF = EBADF
    , ERR_CHILD = ECHILD
    , ERR_AGAIN = EAGAIN
    , ERR_NOMEM = ENOMEM
    , ERR_ACCES = EACCES
    , ERR_FAULT = EFAULT
    , ERR_NOTBLK = ENOTBLK
    , ERR_BUSY = EBUSY
    , ERR_EXIST = EEXIST
    , ERR_XDEV = EXDEV
    , ERR_NODEV = ENODEV
    , ERR_NOTDIR = ENOTDIR
    , ERR_ISDIR = EISDIR
    , ERR_INVAL = EINVAL
    , ERR_NFILE = ENFILE
    , ERR_MFILE = EMFILE
    , ERR_NOTTY = ENOTTY
    , ERR_TXTBSY = ETXTBSY
    , ERR_FBIG = EFBIG
    , ERR_NOSPC = ENOSPC
    , ERR_SPIPE = ESPIPE
    , ERR_ROFS = EROFS
    , ERR_MLINK = EMLINK
    , ERR_PIPE = EPIPE
    , ERR_DOM = EDOM
    , ERR_RANGE = ERANGE

    , ERR_WOULDBLOCK = EWOULDBLOCK

    , ERR_DEADLK = EDEADLK
    , ERR_NAMETOOLONG = ENAMETOOLONG
    , ERR_NOLCK = ENOLCK
    , ERR_NOSYS = ENOSYS
    , ERR_NOTEMPTY = ENOTEMPTY
    , ERR_LOOP = ELOOP
    , ERR_NOMSG = ENOMSG
    , ERR_IDRM = EIDRM

#ifdef EDEADLOCK
    , ERR_DEADLOCK = EDEADLOCK
#endif

#if 0
    , ERR_NOSTR = ENOSTR
    , ERR_NODATA = ENODATA
    , ERR_TIME = ETIME
    , ERR_NOSR = ENOSR
    , ERR_REMOTE = EREMOTE
    , ERR_NOLINK = ENOLINK
    , ERR_PROTO = EPROTO
    , ERR_MULTIHOP = EMULTIHOP
    , ERR_BADMSG = EBADMSG
#endif
    , ERR_OVERFLOW = EOVERFLOW
    , ERR_ILSEQ = EILSEQ
    , ERR_USERS = EUSERS
    , ERR_NOTSOCK = ENOTSOCK
    , ERR_DESTADDRREQ = EDESTADDRREQ
    , ERR_MSGSIZE = EMSGSIZE
    , ERR_PROTOTYPE = EPROTOTYPE
    , ERR_NOPROTOOPT = ENOPROTOOPT
    , ERR_PROTONOSUPPORT = EPROTONOSUPPORT
    , ERR_SOCKTNOSUPPORT = ESOCKTNOSUPPORT
    , ERR_OPNOTSUPP = EOPNOTSUPP
    , ERR_PFNOSUPPORT = EPFNOSUPPORT
    , ERR_AFNOSUPPORT = EAFNOSUPPORT
    , ERR_ADDRINUSE = EADDRINUSE
    , ERR_ADDRNOTAVAIL = EADDRNOTAVAIL
    , ERR_NETDOWN = ENETDOWN
    , ERR_NETUNREACH = ENETUNREACH
    , ERR_NETRESET = ENETRESET
    , ERR_CONNABORTED = ECONNABORTED
    , ERR_CONNRESET = ECONNRESET
    , ERR_NOBUFS = ENOBUFS
    , ERR_ISCONN = EISCONN
    , ERR_NOTCONN = ENOTCONN
    , ERR_SHUTDOWN = ESHUTDOWN
    , ERR_TOOMANYREFS = ETOOMANYREFS
    , ERR_TIMEDOUT = ETIMEDOUT
    , ERR_CONNREFUSED = ECONNREFUSED
    , ERR_HOSTDOWN = EHOSTDOWN
    , ERR_HOSTUNREACH = EHOSTUNREACH
    , ERR_ALREADY = EALREADY
    , ERR_INPROGRESS = EINPROGRESS
    , ERR_STALE = ESTALE
    , ERR_DQUOT = EDQUOT

};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __POSIX_OS_H_1126542420__ */

/* EOF */
