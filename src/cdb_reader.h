/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * cdb_reader.h - CDB Reader Interface
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
 */

#ifndef ___CDB_READER_H_8420881_1470077389__
#define ___CDB_READER_H_8420881_1470077389__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <inttypes.h>

#ifndef __BYTE_ORDER__
#error "Don't know the byte order!"
#endif /* !__BYTE_ORDER__ */

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __little_endian
#else
#undef __little_endian
#endif /* __BYTE_ORDER__ */


/*
 * The first level directory index.
 */
struct idx {
    uint32_t off;
    uint32_t len;
};
typedef struct idx idx;


/*
 * Abstrction of a DJB CDB/
 *
 * This struct is optimized for little-endian systems; we don't need
 * to store the entire table.
 */
struct CDB {
    uint8_t *mmap;
    uint64_t size;

#ifdef __little_endian
    idx     *index;   // we just point to the structure
#else
    idx     index[256];
#endif /* LITTLE_ENDIAN */

    int      fd;
};
typedef struct CDB CDB;



/*
 * Initialize the 'db' for read operations from file 'fname'
 * Return:
 *  0      on success
 *  -errno on failre.
 */
extern int cdb_read_init(CDB* db, const char* fname);



/**
 * Find key 'key' in the DB. If found, set 'p_ret' to the
 * corresponding value.
 *
 * Returns:
 *      >= 0     key is found; # of bytes of value
 *      -ENOENT  key is not found
 */
extern ssize_t cdb_find(CDB* db, void** p_ret, const void* key, size_t klen);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___CDB_READER_H_8420881_1470077389__ */

/* EOF */
