/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * sysrand.cpp - Posix implementation of system random number
 * generator interface.
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
 * Creation date: Mon Sep 19 14:37:57 2005
 */

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include "syserror.h"
#include "utils/sysrand.h"
#include "utils/random.h"

namespace putils {


sys_rand::sys_rand()
    : randgen("sysrand")
{
    int f = ::open("/dev/urandom", O_RDONLY);
    if ( f < 0 )
        throw sys_exception(geterror(), "Can't find /dev/urandom");

    fd = f;
}

sys_rand::~sys_rand()
{
    ::close(fd);
}


unsigned long
sys_rand::generate_long()
{
    union
    {
        unsigned long v;
        unsigned char buf[sizeof(unsigned long)];
    } u;

    generate_bits(u.buf, (sizeof u.buf));

    return u.v;
}


void *
sys_rand::generate_bits(void* buf, size_t nbytes)
{
    int f = fd;

    unsigned char * ptr = (unsigned char *)buf;
    while (nbytes > 0)
    {
        int n = ::read(f, ptr, nbytes);

        if ( n < 0 )
        {
            if ( errno == EINTR )
                continue;

            throw sys_exception(geterror(), "Unable to read %d bytes from /dev/urandom",
                    nbytes);
        }

        ptr    += n;
        nbytes -= n;
    }

    return buf;
}

} // namespace putils

/*
 * Return random bytes
 */
extern "C" void *
sys_randbytes(void * buf, int nbytes)
{
    static putils::sys_rand r;

    return r.generate_bits(buf, nbytes);
}


/*
 * Return random u32
 */
extern "C" unsigned long
sys_randlong()
{
    static putils::sys_rand r;

    return r.generate_long();
}

