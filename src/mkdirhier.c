/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * mkdirhier.c - portable routine to emulate 'mkdir -p'
 *
 * Copyright (c) 2006 Sudhi Herle <sw at herle.net>
 *
 * License: GPLv2
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>

//#include "utils/mkdirhier.h"

static const char*
sep(const char* p)
{
    int c;
    while ((c = *p)) {
        if (c == '/')
            return p;

        p++;
    }

    return 0;
}


int
mkdirhier(const char* path, mode_t mode)
{
    char z[PATH_MAX];
    struct stat st;

    if (0 == stat(path, &st) && S_ISDIR(st.st_mode))
        return 0;

    if (strlen(path) >= PATH_MAX)
        return -ENAMETOOLONG;


    const char *prev;
    const char *next;

    prev = path;
    if (*prev == '/')
        prev++;

    while ((next = sep(prev))) {
        size_t n = next - path;
        memcpy(z, path, n);
        z[n] = 0;

        //printf("path=%.*s  z=%s\n", n, path, z);

        int r = stat(z, &st);
        if (r == 0 && !S_ISDIR(st.st_mode))
            return -ENOTDIR;
        else if (r < 0) {
            if (errno != ENOENT)
                return -errno;

            r = mkdir(z, mode);
            if (r < 0)
                return -errno;
        }

        prev = next+1;
    }

    // Make the last and final part
    if (mkdir(path, mode) < 0)
        return -errno;

    return 0;
}

/* EOF */
