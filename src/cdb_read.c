/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * cdb_read.c - CDB Reader Interface
 *
 * Copyright (c) 2016 Sudhi Herle <sw at herle.net>
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
 * Notes
 * =====
 * o  This uses a memory map'd interface for READONLY access to the
 *    database.
 * o  All read offsets are checked for sanity.
 */
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "cdb_reader.h"

#define _u32(x)     ((uint32_t)(x))
#define pU32(x)     ((uint32_t*)(x))

#ifdef __little_endian

#define __read32(d, off) ({ \
        assert(off < (d)->size); \
        (*pU32((d)->mmap+off));  \
        })
#else

static inline uint32_t
dec_LE_u32(const uint8_t * p) {
    return         p[0]
           | (_u32(p[1]) << 8)
           | (_u32(p[2]) << 16)
           | (_u32(p[3]) << 24);
}

#define __read32(d, off) ({ \
        assert(off < (d)->size); \
        dec_LE_u32((d)->mmap+off);     \
        })

#endif /* __little_endian */

extern uint64_t fasthash64(const void*, size_t, uint64_t);

static inline uint32_t
cdb_hash(const void* k, size_t klen)
{
    uint64_t h = fasthash64(k, klen, 0x2de9ce7b97d9569f);
    return (uint32_t)(h - (h >> 32));
}


// Initialize 'db' for reading from 'fname'
// Return -errno on failure; 0 on success
int
cdb_read_init(CDB* db, const char* filename)
{
    int r  = 0;
    int fd = open(filename, O_RDONLY);
    if (fd < 0) return -errno;

    struct stat st;

    if (fstat(fd, &st) < 0) { r = -errno;  goto fail; }
    if (st.st_size < 2048)  { r = -EINVAL; goto fail; }

    void* ptr = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)  { r = -errno;  goto fail; }

    /*
     * Now populate the cdb instance with data
     *
     * First 256 * 8 bytes are the index.
     * Each entry is: offset, length pair (4 bytes each)
     *
     * The entries are in little-endian format.
     */

    memset(db, 0, sizeof *db);
    db->fd    = fd;
    db->mmap  = (uint8_t*)ptr;
    db->size  = st.st_size;

#ifdef __little_endian
    db->index = (idx *)ptr;
#else
    int i;
    uint8_t * p = db->mmap;
    for (i = 0; i < 256; i++) {
        idx *ii = &db->index[i];
        ii->off = dec_LE_u32(p); p += 4;
        ii->len = dec_LE_u32(p); p += 4;
    }
#endif /* __little_endian */

    return 0;

fail:
    close(fd);
    return r;
}




/**
 * Find key 'key' in the DB. If found, set 'p_ret' to the
 * corresponding value.
 *
 * Returns:
 *      >= 0     key is found; # of bytes of value
 *      -ENOENT  key is not found
 */
ssize_t
cdb_find(CDB* db, void** p_ret, const void* key, size_t klen)
{
    uint32_t h = cdb_hash(key, klen);
    idx *ii    = &db->index[h & 0xff];

    if (ii->len == 0) { return -ENOENT; }

    uint32_t start = (h >> 8) % ii->len;
    uint32_t slot  = start;

     do {
        uint32_t slotoff  = ii->off + (8 * slot);
        uint32_t slothash = __read32(db, slotoff);
        uint32_t off      = __read32(db, slotoff+4);

        if (slothash == 0) break;

        // Now, check to see if the key matches
        if (slothash == h) {
            uint32_t dklen = __read32(db, off);
            uint32_t dvlen = __read32(db, off+4);

            if (dklen != klen) return -ENOENT;

            // Sanity check
            assert((off+8+dklen+dvlen) <= db->size);
        
            // key + value is at off+8
            if (0 != memcmp(key, db->mmap+off+8, klen)) return -ENOENT;

            // Value is at off+8+dklen
            *p_ret = db->mmap + off + 8 + dklen;
            return dvlen;
        }

        slot = (slot + 1) % ii->len;
    } while (slot != start);

    return -ENOENT;
}

// EOF
