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
 * Creation date: Sat Sep 17 13:50:06 2005
 */
#include <string.h>
#include <errno.h>

#include "syserror.h"

using namespace std;
using namespace putils;


namespace  putils {

extern "C" int
geterrno()
{
    return errno;
}



syserror
geterror(int err, const string& prefix)
{
    //int err       = errno;
    char buf[256] = { 0 };

    strerror_r(err, buf, sizeof buf);
    
    syserror p(err, prefix);

    if (prefix.length() > 0)
        p.errstr += ": ";
    p.errstr.append(buf);
    return p;
}



syserror
geterror(const string& prefix)
{
    int e = errno;
    return geterror(e, prefix);
}


}

/* EOF */
