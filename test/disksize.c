/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * disksize.c - Get physical disksize
 *
 * Copyright (c) 2015 Sudhi Herle <sw at herle.net>
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

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>

#include "error.h"

#ifdef __APPLE__
#include <sys/disk.h>
#elif defined(__linux__)
#include <linux/fs.h>
#endif

uint64_t
get_disksize(int fd, const char* fname)
{
    struct stat st;

    if (fstat(fd, &st) < 0)
        error(1, errno, "Can't query %s", fname);

    if (S_ISREG(st.st_mode)) return st.st_size;

    if (!S_ISBLK(st.st_mode))
        error(1, 0, "%s is not a file or a block device?", fname);

#ifdef __linux__

uint64_t oct = 0;

    //if(ioctl(fd,BLKGETSIZE,&blksize) < 0) error(1, errno, "Can't get blocksize for %s", fname);
    if(ioctl(fd,BLKGETSIZE64,&oct) < 0)   error(1, errno, "Can't get blocksize64 for %s", fname);

        return oct;
#elif defined(__APPLE__)
    uint64_t nblks;
    uint32_t blksize;

    if(ioctl(fd, DKIOCGETBLOCKSIZE,  &blksize) < 0)  error(1, errno, "Can't get blocksize for %s", fname);
    if(ioctl(fd, DKIOCGETBLOCKCOUNT, &nblks) < 0)    error(1, errno, "Can't get block count for %s", fname);

    return blksize * nblks;
#else
#error "I don't know how to get PHYS DISK size on this platform"
#endif

}


/* EOF */
