/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * rotatefile.c - Simple utility to rotate a log file
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
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>

#include "utils/rotatefile.h"

static int __exists(const char *filename);
static void delete_old(const char *fn, int start, int end);

/**
 * Unconditionally rotate a file and keep the last 'nsaved' copies.
 * Flags is currently unused. It can be used to communicate things
 * like "compress" etc.
 */
int
rotate_filename(const char *filename, int nsaved, unsigned int flags)
{
    char f1[PATH_MAX];
    char f2[PATH_MAX];
    int r;

    (void)flags;

    // Delete older files upto a max of 100 extra files
    delete_old(filename, nsaved, nsaved+100);

    while (--nsaved > 0) {
        snprintf(f1, sizeof f1, "%s.%d", filename, nsaved-1);
        snprintf(f2, sizeof f2, "%s.%d", filename, nsaved);

        //printf("%s -> %s\n", f1, f2);
        r = __exists(f1);
        if (r < 0) return r;

        if (r > 0) {
            if (rename(f1, f2) < 0) return -errno;
        }
    }

    r = __exists(filename);
    if (r < 0) return r;

    if (r > 0) {
        snprintf(f1, sizeof f1, "%s.0", filename);
        if (rename(filename, f1) < 0) return -errno;
    }

    return 0;
}


/**
 * Rotate a file if it exceeds size_mb MBytes and keep the last 'nsaved' copies.
 * Flags is currently unused. It can be used to communicate things
 * like "compress" etc.
 */
int
rotate_filename_by_size(const char *filename, int nsaved,
                        uint64_t size_mb, unsigned int flags)
{
    struct stat st;

    int r = stat(filename, &st);
    if (r < 0) {
        if (errno == ENOENT) return 0;

        return -errno;
    }
    if (!S_ISREG(st.st_mode)) return -EINVAL;

    uint64_t actual_size = st.st_size / 1048576;
    if (actual_size > size_mb)
        return rotate_filename(filename, nsaved, flags);

    return 0;
}

// return true if 'filename' exists
static int
__exists(const char *filename)
{
    struct stat st;

    int r = stat(filename, &st);
    if (r < 0) {
        if (errno == ENOENT)
            return 0;

        return -errno;
    }

    return S_ISREG(st.st_mode) ? 1 : -EINVAL;
}


// delete all old files between 'start' and 'end'
static void
delete_old(const char *fn, int start, int end)
{
    char x[PATH_MAX];

    for (; start < end; ++start) {
        snprintf(x, sizeof x, "%s.%d", fn, start);
        unlink(x);
    }
}
