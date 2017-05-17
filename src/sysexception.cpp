/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * sysexception.cpp
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
 * Creation date: Sat Sep 17 14:37:19 2005
 */

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include <algorithm>
#include "syserror.h"

using namespace std;
using namespace putils;


namespace putils {

sys_exception::sys_exception(const syserror& e)
    : m_err(e.err)
{
    memset(m_buf, 0, sizeof(m_buf));

    size_t avail = min(sizeof(m_buf)-1, e.errstr.length());
    memcpy(m_buf, e.errstr.c_str(), avail);
}


sys_exception::sys_exception(const syserror& e, const char * fmt, ...)
    : m_err(e.err)
{
    const string::size_type max = sizeof(m_buf)-2;
    const string& err = e.errstr;
    char buf[sizeof(m_buf)];

    memset(m_buf, 0, sizeof(m_buf));

    va_list ap;
    va_start(ap, fmt);
    size_t n = vsnprintf(buf, max, fmt, ap);
    va_end(ap);

    assert(n <= max);

    memcpy(m_buf, buf, n);
    m_buf[n++] = ':';

    size_t avail = max - n;
    if ( avail > 0 )
    {
        avail = min(avail, err.length());
        memcpy(m_buf+n, err.c_str(), avail);
    }
}



sys_exception::sys_exception(const char * fmt, ...)
    : m_err(0)
{
    memset(m_buf, 0, sizeof(m_buf));

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(m_buf, sizeof(m_buf), fmt, ap);
    va_end(ap);
}


sys_exception::~sys_exception() throw()
{
}


const char *
sys_exception::what() const throw()
{
    return m_buf;
}


sys_exception::sys_exception()
    : m_err(0)
{
    memset(m_buf, 0, sizeof(m_buf));
}
      
sys_exception::sys_exception(int err)
    : m_err(err)
{
    memset(m_buf, 0, sizeof(m_buf));
}



// -- Syscall Interrupted --


syscall_interrupted::syscall_interrupted(const char * fmt, ...)
    : m_err(0)
{
    memset(m_buf, 0, sizeof(m_buf));

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(m_buf, sizeof m_buf, fmt, ap);
    va_end(ap);
}


syscall_interrupted::~syscall_interrupted() throw()
{
}


const char *
syscall_interrupted::what() const throw()
{
    return m_buf;
}


syscall_interrupted::syscall_interrupted()
    : m_err(0)
{
    memset(m_buf, 0, sizeof(m_buf));
}
      



// -- DLL not found --

dll_not_found::dll_not_found(const char * name)
    : sys_exception(geterror(), "Can't load DLL %s", name)
{
}

dll_not_found::~dll_not_found() throw()
{
}

dll_symbol_not_found::dll_symbol_not_found(const char * dll, const char * sym)
    : sys_exception(geterror(), "Can't find '%s()' in DLL %s", sym, dll)
{
}

dll_symbol_not_found::~dll_symbol_not_found() throw()
{
}


lock_owner_died::lock_owner_died(const string& name)
    : sys_exception("Thread owning locked '%s' died", name.c_str())
{
}

lock_owner_died::~lock_owner_died() throw()
{
}


}

/* EOF */
