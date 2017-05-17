/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * mmap.c - Win32 implementation of Posix mmap(2) system call.
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
 * Creation date: Thu Oct 20 11:05:35 2005
 */

#include <sys/types.h>
#include <stdint.h>
#include <sys/mman.h>
#include <errno.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winbase.h>
#include <io.h>
#include <win32/os.h>


/*
 * Map Posix flags to win32 flags.
 * Note: cfm_flags  => flags for CreateFileMapping()
 *       mvof_flags => flags for MapViewOfFile()
 */
static void
__map_flags(int prot, int flags, DWORD * p_cfm_flags, DWORD * p_mvof_flags)
{
    DWORD cfm_flags  = 0,
          mvof_flags = 0;


    if (prot == PROT_NONE)
    {
        cfm_flags = PAGE_NOACCESS;
        goto out0;
    }

    if (prot & PROT_WRITE)
    {
        mvof_flags |= flags & MAP_PRIVATE ? FILE_MAP_COPY : FILE_MAP_WRITE;
    }
    else
        mvof_flags |= FILE_MAP_READ;

    if (mvof_flags & FILE_MAP_COPY)
        cfm_flags = PAGE_WRITECOPY;
    else if (mvof_flags & FILE_MAP_WRITE)
        cfm_flags = PAGE_READWRITE;
    else
        cfm_flags = PAGE_READONLY;

out0:

    /* XXX Will this work?  */
    if (flags & MAP_ANONYMOUS)
        cfm_flags |= SEC_RESERVE;

    *p_cfm_flags  = cfm_flags;
    *p_mvof_flags = mvof_flags;
    return;
}

static void*
__error(int e)
{
    errno = e;
    return MAP_FAILED;
}


void*
mmap(void* addr, size_t ilen, int prot, int flags, int fd, off_t offset)
{
    off_t len = 0 + ilen;
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    DWORD cfm_flags,
          mvof_flags;

    /* Validation */
    if ((flags & MAP_ANONYMOUS) == MAP_ANONYMOUS)
    {
        if (fd != -1)
            return __error(EINVAL);

        if (len == 0 || offset != 0)
            return __error(EINVAL);
    }
    else
    {
        LARGE_INTEGER li;
        uint64_t    max = (offset + (off_t)len);

        if (!GetFileSizeEx(h, &li))
            return __error(map_win32_error(GetLastError()));

        if (max > (uint64_t)li.QuadPart)
            return __error(EINVAL);
    }

    /* If MAP_FIXED is specified, the user better provide an addr */
    if ((flags & MAP_FIXED) && !addr)
        return __error(EINVAL);

    if (offset < 0 || (offset + (off_t)len) < 0)
        return __error(EINVAL);

    __map_flags(prot, flags, &cfm_flags, &mvof_flags);
    {
        void * retaddr;
        uint64_t maxsize = (offset +  (off_t)len);
        DWORD    size_lo = (DWORD)(maxsize & 0xffffffff);
        DWORD    size_hi = (DWORD)(maxsize >> 32);
        HANDLE   fm_h    = CreateFileMapping(h, 0, cfm_flags, size_hi, size_lo, 0);

        if (!fm_h)
            return __error(map_win32_error(GetLastError()));

        /*
         * XXX Not 64-bit safe!
         * XXX Not safe for files > 4GB!
         */
        retaddr = MapViewOfFileEx(fm_h, mvof_flags, 0, (DWORD)offset, len, addr);
        if (!retaddr)
        {
            DWORD err = GetLastError();
            CloseHandle(fm_h);

            return __error(map_win32_error(err));
        }

        CloseHandle(fm_h);
        return retaddr;
    }
}


/*
 * unmap file mapping at 'addr'.
 * In this win32 implementation, we ignore the 'len' parameter.
 */
int
munmap(void* addr, size_t len)
{
    len = len;

    if (addr == MAP_FAILED)
    {
        errno = EINVAL;
        goto _error;
    }

    if (UnmapViewOfFile(addr))
        return 0;

    errno = map_win32_error(GetLastError());
_error:
    return -1;
}


/*
 * Flush pages to disk.
 */
int
msync(void* addr, size_t len, int flags)
{
    flags = flags;

    if (addr == MAP_FAILED)
    {
        errno = EINVAL;
        goto _error;
    }

    if (FlushViewOfFile(addr, len))
        return 0;

    errno = map_win32_error(GetLastError());
_error:
    return -1;
}

/*
 * Pin pages in RAM; don't allow to be flushed to swap.
 */
int
mlock(void* addr, size_t len)
{
    if (addr == MAP_FAILED)
    {
        errno = EINVAL;
        goto _error;
    }

    if (VirtualLock(addr, len))
        return 0;

    errno = map_win32_error(GetLastError());
_error:
    return -1;
}


/*
 * Unpin pages previously pinned.
 */
int
munlock(void* addr, size_t len)
{
    if (addr == MAP_FAILED)
    {
        errno = EINVAL;
        goto _error;
    }

    if (VirtualUnlock(addr, len))
        return 0;

    errno = map_win32_error(GetLastError());
_error:
    return -1;
}

/* EOF */
