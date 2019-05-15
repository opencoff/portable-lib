/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * mmap.cpp - Portable memory mapped file implementation.
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
 * Creation date: Sat Sep 17 17:14:51 2005
 */
#include "utils/mmap.h"
#include "utils/utils.h"

#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

using namespace std;
using namespace putils;

namespace putils {

mmap_file::mmap_file(const string& fname, unsigned int flags)
        : m_fd(0),
          m_flags(flags),
          m_filesize(0),
          m_filename(fname)
{
    unsigned int mode = O_RDONLY;

    if ( flags & MMAP_RDWR )
        mode = O_RDWR;


    int fd  = ::open(fn(), mode);
    if (fd < 0)
        throw sys_exception(geterror(), "Can't open '%s'", fn());

    struct stat st;
    if ( ::fstat(fd, &st) < 0 )
        throw sys_exception(geterror(), "Can't query '%s'", fn());


    m_filesize = st.st_size;
    m_fd       = fd;

    // If a file size is zero, it is OK. When we mmap() below, we
    // extend the file as needed.
}


mmap_file::~mmap_file()
{
    ::close(int(m_fd));

    all_mappings::iterator i  = m_mappings.begin(),
                          end = m_mappings.end();
    while (i != end)
    {
        const mapping& m = *i;
        ::munmap(m.ptr, m.size);
        ++i;
    }
}


void *
mmap_file::mmap(off_t off, size_t size)
{
    void * ptr = find(off, size);

    if (ptr) return ptr;

    off_t newsz = off + size;
    if (newsz > m_filesize) {
        if (! (m_flags & MMAP_CREATE))
            throw sys_exception("mmap offset '%ull' for '%s' out of bounds",
                                 off, fn());

        if (::ftruncate(int(m_fd), newsz) < 0)
            throw sys_exception(geterror(), "Can't truncate '%s' to %ull bytes",
                                 fn(), newsz);

        m_filesize = newsz;
    }

    int prot = PROT_READ;
    int mode = MAP_SHARED;

    if ( m_flags & MMAP_RDWR )
        prot  |= PROT_WRITE;

    if ( m_flags & MMAP_SHARED )
        mode |= MAP_SHARED;

    ptr = ::mmap(0, size, prot, mode, int(m_fd), off_t(off));
    if ( (void *)-1 == ptr )
        throw sys_exception(geterror(), "%s: Can't mmap %llu bytes at %llu ", fn(), size, off);

    mapping m(ptr, size, off);

    m_mappings.push_back(m);

    return ptr;
}




void
mmap_file::unmap(void * ptr)
{
    pair<void *, size_t> p = maybe_erase(ptr);
    if ( p.first )
        ::munmap(p.first, p.second);
}


// lock and unlock memory
void
mmap_file::mlock(void * ptr, size_t n)
{
    const size_t pgsize = pagesize();

    ptr = align_down(ptr, pgsize);
    n   = align_up(n, pgsize);

    if ( ::mlock(ptr, n) < 0 )
        throw sys_exception(geterror(), "%s: Can't lock '%lu' bytes at '%p'",
                            fn(), n, ptr);
}


void
mmap_file::munlock(void * ptr, size_t n)
{
    const size_t pgsize = pagesize();

    ptr = align_down(ptr, pgsize);
    n   = align_up(n, pgsize);

    if ( ::munlock(ptr, n) < 0 )
        throw sys_exception(geterror(), "%s: Can't unlock '%lu' bytes at '%p'",
                            fn(), n, ptr);
}

#if 0

// Ensure that file 'f' is exactly 'size' bytes long; create if
// necessary
void
mmap_file::ensure_file_size(const string& f, off_t size)
{
    const char * fn = f.c_str();

    int fd = ::open(f.c_str(), O_RDWR|O_CREAT, 0640);

    if ( fd < 0 )
        throw sys_exception(geterror(), "Can't open '%s'", fn);

    struct stat st;
    if ( ::fstat(fd, &st) < 0 )
        throw sys_exception(geterror(), "Can't query '%s'", fn);

    if ((off_t)(st.st_size) != size) {
        if ( ::ftruncate(fd, size) < 0)
            throw sys_exception(geterror(), "Can't truncate '%s' to %ull bytes",
                                 fn, size);
    }

    ::close(fd);
}
#endif

// Return the system's VM page size
// XXX This code is ugly to maintain portability
size_t
mmap_file::pagesize()
{
    long v;

#if defined(_SC_PAGESIZE) || defined(_SC_PAGE_SIZE)

    #ifdef _SC_PAGESIZE
        v = sysconf(_SC_PAGESIZE);
    #else
        v = sysconf(_SC_PAGE_SIZE);
    #endif /* PAGESIZE vs. PAGE_SIZE */

#else
    // We assume that the OS supports the getpagesize() call
    v = getpagesize();

#endif /* _SC_PAGESIZE || _SC_PAGE_SIZE */
    return v;
}


}

/* EOF */
