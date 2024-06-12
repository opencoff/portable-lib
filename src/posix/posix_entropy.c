/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * posix_entropy.c - Entropy interface for POSIX systems.
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


#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>


static int _Randfd = -1;
static pthread_once_t _Rand_once = PTHREAD_ONCE_INIT;

static void
__randopen(void)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "can't open /dev/urandom: %s (%d)\n", strerror(errno), errno);
        exit(1);
    }

    _Randfd = fd;
}


int
getentropy(void* buf, size_t n)
{
    uint8_t* b    = (uint8_t*)buf;

    pthread_once(&_Rand_once, __randopen);

    while (n > 0) {
        ssize_t m = (read)(fd, b, n);

        if (m < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        b += m;
        n -= m;
    }

    return 0;
}



/* EOF */
