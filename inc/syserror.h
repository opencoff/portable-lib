/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * syserror.h - Generic access to system errors
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
 * Creation date: Sat Sep 17 13:23:39 2005
 */

#ifndef __SYSERROR_H_1126981419__
#define __SYSERROR_H_1126981419__ 1


// Get system specific error numbers.
// These are platform independent
#ifdef WIN32
#include "win32/os.h"
#else
#include "posix/os.h"
#endif

#ifdef __cplusplus

#include <errno.h>
#include <string>
#include <exception>

namespace putils {

struct syserror
{
    int err;
    std::string errstr;
    syserror(int x, const char * buf): err(x), errstr(buf) {}
    syserror(int x, const std::string& buf): err(x), errstr(buf) {}
};


syserror geterror(const std::string&);
syserror geterror(int err, const std::string&);

// Get last error and return as pair
inline syserror geterror(const char * prefix = 0)
{
    if (!prefix)
        prefix = "";
    return geterror(std::string(prefix));
}

// Convert 'err' to syserror
inline syserror geterror(int err, const char* prefix=0)
{
    if (!prefix)
        prefix = "";
    return geterror(err, std::string(prefix));
}




class sys_exception: public std::exception
{
public:
    sys_exception(const syserror&);
    sys_exception(const syserror&, const char * fmt, ...);
    sys_exception(const char * fmt, ...);
    virtual ~sys_exception() throw();

    virtual const char * what() const throw();

    int errcode() const throw() { return m_err; }


protected:
    sys_exception();
    sys_exception(int err);

protected:
    int         m_err;
    char        m_buf[1024];
};

class syscall_interrupted: public std::exception
{
public:
    syscall_interrupted(const char * fmt, ...);
    virtual ~syscall_interrupted() throw();

    virtual const char * what() const throw();

    int errcode() const throw() { return EINTR; }


protected:
    syscall_interrupted();
    syscall_interrupted(int err);

protected:
    int         m_err;
    char        m_buf[1024];
};


class dll_not_found : public sys_exception
{
public:
    dll_not_found(const char * name);
    virtual ~dll_not_found() throw();
};

class dll_symbol_not_found : public sys_exception
{
public:
    dll_symbol_not_found(const char * dll, const char * sym);
    virtual ~dll_symbol_not_found() throw();
};


/*
 * Exception thrown by objects that manage locks
 */

class lock_owner_died : public sys_exception
{
public:
    lock_owner_died(const std::string& name);
    virtual ~lock_owner_died() throw();
};


}

// For C Linkage 
extern "C" {
#endif  /* __cplusplus */


// Return last errno
extern int  geterrno(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* ! __SYSERROR_H_1126981419__ */

/* EOF */
