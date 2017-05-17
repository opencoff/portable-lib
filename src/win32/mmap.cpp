/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * mmap.cpp - Win32 MMAP API
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
 * Creation date: Sat Sep 17 12:56:54 2005
 */
#include <windows.h>
#include <winbase.h>

#include <stdint.h>
#include "utils/utils.h"
#include "utils/mmap.h"

using namespace std;
using namespace putils;

namespace putils {


#define _lo32(x)    (((x) << 32) >> 32)
#define _hi32(x)    ((x) >> 32)

mmap_file::mmap_file(const string& filename, unsigned int flags)
        : m_fd(0),
          m_flags(flags),
          m_filesize(_ULLCONST(0)),
          m_filename(filename)
{

    // Shared mappings don't need a file to be open
    if ( flags & MMAP_SHM )
    {
        m_fd = _ULONG(INVALID_HANDLE_VALUE);
        return;
    }

    DWORD mode        = GENERIC_READ;
    DWORD disposition = OPEN_EXISTING;

    if (flags & MMAP_RDWR)
        mode  |= GENERIC_WRITE;

    if (flags & MMAP_CREATE)
        disposition = OPEN_ALWAYS; // XXX CREATE_NEW?

    HANDLE fd = CreateFile(
                    fn(),                   // filename
                    mode,                   // access mode
                    FILE_SHARE_READ,        // share mode
                    0,                      // sec descr
                    disposition,            // file disposition
                    FILE_ATTRIBUTE_NORMAL,  // attr
                    0                       // template
                );

    if ( INVALID_HANDLE_VALUE == fd )
        throw sys_exception(geterror(), "Can't open '%s'", fn());

    LARGE_INTEGER li;
    if (!GetFileSizeEx(fd, &li))
        throw sys_exception(geterror(), "Can't get file size for '%s'", fn());

    m_filesize = li.QuadPart;
    if ( !m_filesize )
        throw sys_exception("Can't mmap zero-length file '%s'", fn());

    m_fd = _ULONG(fd);
}



mmap_file::~mmap_file()
{
    CloseHandle(HANDLE(m_fd));

    all_mappings::iterator i  = m_mappings.begin(),
                          end = m_mappings.end();
    while (i != end)
    {
        const mapping& m = *i;
        UnmapViewOfFile(m.ptr);
        ++i;
    }
}


void *
mmap_file::mmap(unsigned long long off, unsigned long size)
{
    void * ptr = find(off, size);

    if ( ptr )
        return ptr;

    if ( off >= m_filesize )
        throw sys_exception("mmap offset '%ull' for '%s' too large",
                             off, fn());


    // Align the offset to the pagesize
    off = align_down(off, pagesize());

    const bool write_op = !!(m_flags & MMAP_RDWR);
    DWORD cfm_flags = write_op ? PAGE_READWRITE : PAGE_READONLY;
    DWORD mvf_flags = write_op ? FILE_MAP_WRITE : FILE_MAP_READ;

    if ( m_flags & MMAP_PRIVATE )
    {
        cfm_flags  = PAGE_WRITECOPY;
        mvf_flags |= FILE_MAP_COPY;
    }

    LARGE_INTEGER li;
    unsigned long long end = off + size;

    if ( end > m_filesize )
        size = m_filesize - off;    // only what is available

    li.QuadPart = m_filesize;


    HANDLE mh = CreateFileMapping(
                    HANDLE(m_fd),   // file handle
                    0,              // sec descr
                    cfm_flags,      // flags
                    li.HighPart,    // max mapsize high word
                    li.LowPart,     // max mapsize low word
                    fn()            // filename
                );

    // XXX CreateFile() returns INVALID_HANDLE_VALUE but,
    // CreateFileMapping() returns 0. WTF?!
    if ( !mh )
        throw sys_exception(geterror(), "Can't mmap '%s'", fn());


    ptr = MapViewOfFile(
                mh,         // mapping handle
                mvf_flags,  // flags
                _hi32(off), // offset high word
                _lo32(off), // offset low word
                size        // mapping size
         );
    if ( !ptr )
        throw sys_exception(geterror(), "Can't mmap '%s'", fn());

    mapping m(ptr, size, off);

    m_mappings.push_back(m);

    CloseHandle(mh);

    return ptr;
}


void
mmap_file::unmap(void * ptr)
{
    pair<void *, unsigned long> p = maybe_erase(ptr);
    if ( p.first )
        UnmapViewOfFile(p.first);
}


// lock and unlock memory
void
mmap_file::mlock(void * ptr, unsigned long n)
{
    const unsigned long pgsize = pagesize();

    ptr = align_down(ptr, pgsize);
    n   = align_up(n, pgsize);

    if (!VirtualLock(ptr, n))
        throw sys_exception(geterror(), "Can't lock '%lu' bytes at '%p'",
                            n, ptr, fn());
}


void
mmap_file::munlock(void * ptr, unsigned long n)
{
    const unsigned long pgsize = pagesize();

    ptr = align_down(ptr, pgsize);
    n   = align_up(n, pgsize);

    if (!VirtualUnlock(ptr, n))
        throw sys_exception(geterror(), "Can't unlock '%lu' bytes at '%p'",
                            n, ptr, fn());
}



// Ensure that file 'f' is exactly 'size' bytes long; create if
// necessary
void
mmap_file::ensure_file_size(const string& f, unsigned long long size)
{
    DWORD mode = GENERIC_READ | GENERIC_WRITE;
    HANDLE fd = CreateFile(
                    f.c_str(),              // filename
                    mode,                   // access mode
                    FILE_SHARE_READ,        // share mode
                    0,                      // sec descr
                    OPEN_ALWAYS,            // file disposition
                    FILE_ATTRIBUTE_NORMAL,  // attr
                    0                       // template
                );

    if ( INVALID_HANDLE_VALUE == fd )
        throw sys_exception(geterror(), "Can't open '%s'", f.c_str());

    LARGE_INTEGER li;
    if (!GetFileSizeEx(fd, &li))
        throw sys_exception(geterror(), "Can't get file size for '%s'", f.c_str());

    unsigned long long filesz = li.QuadPart;
    if ( filesz != size )
    {
        LONG  hi = _hi32(size);
        DWORD p = SetFilePointer(fd, _lo32(size), &hi, FILE_BEGIN);

        if ( p == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR )
            throw sys_exception(geterror(), "Can't set file of '%s' to %ull",
                                f.c_str(), size);
        if ( !SetEndOfFile(fd) )
            throw sys_exception(geterror(), "Can't set file of '%s' to %ull",
                                f.c_str(), size);
    }

    CloseHandle(fd);
}


// Return's system's VM page size
unsigned long
mmap_file::pagesize()
{
    SYSTEM_INFO si;

    GetSystemInfo(&si); // this func can't fail (returns void)

    return (unsigned long)(si.dwPageSize);
}


}

/* EOF */
