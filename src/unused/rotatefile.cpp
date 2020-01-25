/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * rotatefile.cpp - Simple utility to rotate a log file
 *
 * Copyright (c) 2004-2007 Sudhi Herle <sw at herle.net>
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

#include <string>
#include <exception>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>

#include "utils/rotatefile.h"

using namespace std;
using namespace putils;


namespace putils {

// return true if 'filename' exists
static int
__exists(const string& filename)
{
    struct stat st;

    int r = ::stat(filename.c_str(), &st);
    if (r < 0) {
        if (errno == ENOENT)
            return 0;

        return -errno;
    }

    return S_ISREG(st.st_mode);
}


// Delete all old files between 'start' and 'end'
static void
delete_old(const string& fn, int start, int end)
{
    for (; start < end; ++start) {
        char x[PATH_MAX];

        ::snprintf(x, sizeof x, "%s.%d", fn.c_str(), start);
        ::unlink(x);
    }
}


/**
 * Unconditionally rotate a file and keep the last 'nsaved' copies.
 * Flags is currently unused. It can be used to communicate things
 * like "compress" etc.
 */
int
rotate_filename(const string& filename, int nsaved, unsigned int)
{
    // Delete older files upto a max of 100 extra files
    delete_old(filename, nsaved, nsaved+500);

    while (--nsaved > 0) {
        char s1[16];    // suffix #1
        char s2[16];    // suffix #2

        snprintf(s1, sizeof s1, ".%d", nsaved-1);
        snprintf(s2, sizeof s2, ".%d", nsaved);

        string f1 = filename + s1;
        string f2 = filename + s2;

        //printf("%s -> %s\n", f1.c_str(), f2.c_str());
        int r = __exists(f1);
        if (r < 0) return r;

        if (r > 0) {
            if (::rename(f1.c_str(), f2.c_str()) < 0) return -errno;
        }
    }

    int r = __exists(filename);
    if (r < 0) return r;

    if (r > 0) {
        string f = filename + ".0";
        if (::rename(filename.c_str(), f.c_str()) < 0) return -errno;
    }

    return 0;
}


/**
 * Rotate a file if it exceeds size_mb MBytes and keep the last 'nsaved' copies.
 * Flags is currently unused. It can be used to communicate things
 * like "compress" etc.
 */
int
rotate_filename_by_size(const string& filename, int nsaved,
                        unsigned long size_mb, unsigned int flags)
{
    struct stat st;

    int r = ::stat(filename.c_str(), &st);
    if (r < 0) {
        if (errno == ENOENT)
            return 0;

        return -errno;
    }
    if (!S_ISREG(st.st_mode)) return -EINVAL;

    unsigned long actual_size = st.st_size / 1048576;
    if (actual_size >= size_mb)
        return rotate_filename(filename, nsaved, flags);

    return 0;
}

} // namespace putils


/* EOF */
