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

const char * program_name = 0;

#if defined(_WIN32) || defined(WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

#define realerr(e) ({ \
                        if (e < 0) { \
                            if (0 == (e = GetLastError())) \
                                e = WSAGetLastError(); \
                        } \
                e;\
        })

static char *
geterrstr(int err)
{
    LPSTR buf;
    DWORD buflen;

    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_IGNORE_INSERTS |
                  FORMAT_MESSAGE_FROM_SYSTEM ;

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
#else
#define realerr(e)      ((e) < 0 ? -(e) : (e))
#define geterrstr(e)    strerror(e)
#endif // _WIN32


// print error message, retrieve string corresponding to 'err' if
// necessary. Abort program if 'status' > 0.
void
error(int status, int err, const char *fmt, ...)
{
    err = realerr(err);
    const char * errstr = geterrstr(err);

    fflush(stdout);
    fflush(stderr);
    if (program_name) fprintf(stderr, "%s: ", program_name);

    char buf[2048];
    int max = (sizeof buf) - 1; // for \0

    va_list ap;
    va_start(ap, fmt);

    int n = vsnprintf(buf, max, fmt, ap);
    if (n >= max || n < 0) {
        // output truncated. Ensure we have a trailing zero!
        buf[max] = 0;
        n = max;
    }
    va_end(ap);

    // erase the trailing \n; we'll add it back later.
    if (buf[n-1] == '\n') buf[--n] = 0;

    fputs(buf, stderr);
    if (err > 0) fprintf(stderr, ": %s [%d]", errstr, err);
    fputc('\n', stderr);
    fflush(stderr);

    if (status) exit(status);
}


// print to stream 'fp' but make sure the string is LF terminated.
static void
vprint(FILE *fp, const char *fmt, va_list ap)
{
    char buf[2048];
    size_t max = (sizeof buf) - 2; // for \n and \0

    int n = vsnprintf(buf, max, fmt, ap);
    if (n < 0 || ((size_t)n) >= max) {
        // output truncated. Ensure we have a trailing zero!
        buf[max] = 0;
        n = max;
    }

    if (buf[n-1] != '\n') {
        buf[n]   = '\n';
        buf[n+1] = '\0';
    }
    fputs(buf, fp);
    fflush(fp);
}

// show a warning message
void
warn(const char *fmt, ...)
{
    fflush(stdout);
    fflush(stderr);
    if (program_name) fprintf(stderr, "%s: ", program_name);

    va_list ap;
    va_start(ap, fmt);
    vprint(stderr, fmt, ap);
    va_end(ap);
}

// die horribly
void
die(const char *fmt, ...)
{
    fflush(stdout);
    fflush(stderr);
    if (program_name) fprintf(stderr, "%s: ", program_name);

    va_list ap;
    va_start(ap, fmt);
    vprint(stderr, fmt, ap);
    va_end(ap);

    exit(1);
}

/* EOF */
