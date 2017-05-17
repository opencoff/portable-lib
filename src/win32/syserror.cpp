/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * syserr.cpp - Portable system error description library
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
 * Creation date: Sat Sep 17 13:30:22 2005
 */
#include <string>

// Even though they have same name as POSIX header files, they come
// from ../inc/win32
#include <netdb.h>
#include <sys/socket.h>

#include "syserror.h"

using namespace std;
using namespace putils;


namespace  utils {

static char * error_string(int err);

extern "C" int
geterrno()
{
    int err = GetLastError();

    if ( 0 == err )
        err = WSAGetLastError();

    return err;
}




syserror
geterror(int err, const char* prefix)
{
    char * buf = error_string(err);

    if (!prefix)
        prefix = "";

    syserror p(err, prefix);

    p.errstr += ":";
    p.errstr += buf;

    LocalFree(buf);
    return p;
}

syserror
geterror(const char * prefix)
{
    return geterror(geterrno(), prefix);
}

static char *
error_string(int err)
{
    LPSTR buf;
    DWORD buflen;

    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_IGNORE_INSERTS |
                  FORMAT_MESSAGE_FROM_SYSTEM ;

    //
    // Call FormatMessage() to allow for message 
    // text to be acquired from the system 
    // or from the supplied module handle.
    //

    buflen = FormatMessageA(
                flags,
                NULL, // module to get message from (NULL == system)
                err,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
                (LPSTR) &buf,
                0,
                NULL
            );

    return buf;
}


}
/* EOF */
