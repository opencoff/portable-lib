/* vim: ts=4:sw=4:expandtab:
 *
 * error.c - Unified warning and fatal error message printing
 *           function. Found in GNU-libc implementation.
 *           This is a clean room re-implementation of error(3).
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
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "error.h"

void (*error_print_progname) (void) = 0;
const char * program_name = 0;

#if defined(_WIN32) || defined(WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

static char *
win32_error_string(int err)
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
#endif  /* win32 */

void
error(int status, int errnum, const char * message, ...)
{
    va_list ap;

#ifdef WIN32
    /*
     * Do this before we call any other system function. We don't
     * want the error value to be cleared/reset.
     */
    if (errnum < 0)
    {
        errnum = GetLastError();
        if (errnum == 0)
            errnum = WSAGetLastError();
    }
#endif  /* WIN32 */

    fflush(stdout);
    fflush(stderr);

    if (error_print_progname)
        (*error_print_progname)();
    else if (program_name)
        fprintf(stderr, "%s: ", program_name);

    va_start(ap, message);
    vfprintf(stderr, message, ap);
    va_end(ap);

#ifdef _WIN32

    if (errnum > 0)
    {
        char* errstr = win32_error_string(errnum);
        fprintf(stderr, ": %s [%d]", errstr, errnum);
        LocalFree(errstr);
    }

#else
    if (errnum < 0)
        errnum = -errnum;

    if (errnum > 0)
        fprintf(stderr, ": %s [%d]", strerror(errnum), errnum);
#endif

    fputc('\n', stderr);
    fflush(stderr);

    if (status)
        exit(status);
}

/* EOF */
