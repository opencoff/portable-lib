/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * mmap.h - OS Independent memory mapped files.
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
 */

#ifndef __UTILS_MMAP_H_1126972734__
#define __UTILS_MMAP_H_1126972734__ 1

#include <string>
#include <list>
#include <utility>

#include <sys/types.h>
#include "syserror.h"

namespace putils {



class mmap_file
{
public:

// These are flags for the CTOR. Note:
//
//   o MMAP_PRIVATE and MMAP_SHARED are exclusive
//   o MMAP_RDONLY  and MMAP_RDWR   are ecclusive
#define MMAP_CREATE       1
#define MMAP_RDONLY      (0 << 1)   // default is read-only
#define MMAP_RDWR        (1 << 1)

#define MMAP_PRIVATE     (0 << 2)   // default is private
#define MMAP_SHARED      (1 << 2)


    mmap_file(const std::string& filename, unsigned int flags = 0);
    virtual ~mmap_file();

    // map 'mapsize' bytes at 'mapoff'
    void * mmap(off_t mapoff, size_t mapsize);

    // mmap entire file
    void * mmap() { return mmap(0, size_t(m_filesize)); }

    // Unmap the mapping 'ptr'
    void  unmap(void * ptr);


    // lock 'n' bytes of memory at address 'p'. This prevents these
    // pages from being paged out.
    //
    // Preconditions:
    //  1. 'p' must be page aligned. If not, this function will
    //     round _DOWN_ the address to the nearest page boundary.
    //  2. 'n' must be a multiple of system's page size.
    //     If not, it will round _UP_ to the next multiple of page
    //     size.
    void  mlock(void * p, size_t n);


    // unlock 'n' bytes of memory at address 'p'. This is the
    // opposite of the previous function.
    //
    // Preconditions:
    //  1. 'p' must be page aligned. If not, this function will
    //     round _DOWN_ the address to the nearest page boundary.
    //  2. 'n' must be a multiple of system's page size.
    //     If not, it will round _UP_ to the next multiple of page
    //     size.
    void  munlock(void * p, size_t n);


    off_t filesize() const              { return m_filesize; }
    const std::string& filename() const { return m_filename; }

#if 0
    // Ensure that file 'f' is exactly 'size' bytes big, create if
    // required.
    static void ensure_file_size(const std::string&, off_t size);
#endif


    // Return the system's page-size
    static size_t pagesize();

protected:
    mmap_file();
    const char * fn() const { return m_filename.c_str(); };

    // Find off+size pair in m_mappings
    void * find(off_t off, size_t size)
    {
        all_mappings::iterator i  = m_mappings.begin(),
                              end = m_mappings.end();
        while (i != end)
        {
            const mapping& m = *i;

            if ( m.off == off && m.size == size )
                return m.ptr;
            ++i;
        }

        return 0;
    };


    // Erase node containing 'ptr'
    std::pair<void *, size_t> maybe_erase(void * ptr)
    {
        all_mappings::iterator i  = m_mappings.begin(),
                              end = m_mappings.end();
        while (i != end)
        {
            const mapping& m = *i;

            if ( ptr == m.ptr )
            {
                std::pair<void *, size_t> p(ptr, m.size);

                m_mappings.erase(i);
                return p;
            }
            ++i;
        }

        return std::make_pair((void *)0, 0);
    };

protected:
    struct mapping
    {
        void * ptr;
        size_t size;
        off_t  off;

        mapping(void * p, size_t sz, off_t offset):
            ptr(p), size(sz), off(offset) { }
    };

    typedef std::list<mapping> all_mappings;

    unsigned long  m_fd;
    unsigned int   m_flags;
    off_t          m_filesize;

    std::string m_filename;

    all_mappings m_mappings;

    // XXX Do we keep a list of all locked pages?
};

}

#endif /* ! __UTILS_MMAP_H_1126972734__ */

/* EOF */
