/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * xorfilter_marshal.c - Marshal/Unmarshal of Xorfilters
 *
 * An independent implementation of:
 *  "Xor Filters: Faster and Smaller Than Bloom and Cuckoo Filters"
 *  https://arxiv.org/abs/1912.08258
 *
 * Copyright (c) 2019 Sudhi Herle <sw at herle.net>
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
 * o  See notes in xorfilter.c
 * o  All encoded integers are in Little Endian order
 * o  Filter data starts on a page boundary (so we can mmap it if
 *    needed)
 * o  All information is checksummed using SHA256
 * o  Disk Layout:
 *     - 64 byte header:
 *        * magic         4 bytes [XORF]
 *        * version       1 byte
 *        * isxor16       1 byte
 *        * reserved      2 bytes
 *        * seed          8 bytes
 *        * size          4 bytes
 *        * n elems       4 bytes
 *     - padding to 4k boundary
 *     - actual filter bytes
 *     - 32 byte SHA256 sum
 */
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>

#include "utils/utils.h"
#include "fast/encdec.h"
#include "utils/xorfilter.h"
#include "xorfilt_internal.h"

// Need libsodium to be installed.
#include "sodium.h"

/* Darwin doesn't have fdatasync() prototype */
#ifdef __Darwin__
extern int fdatasync(int);
#endif // __Darwin__


#define SHASIZE  crypto_hash_sha256_BYTES

static inline uint64_t
pagesz()
{
    return sysconf(_SC_PAGESIZE);
}

/*
 * Write header to 'p' from Xorfilter 'x' and advance p to next
 * writable boundary.
 */
static uint8_t*
wrhdr(uint8_t *p, Xorfilter *x)
{
    uint8_t *st  = p;

    memcpy(p, "XORF", 4);  p += 4;

    // version #
    *p = 1;                p += 1;

    // Filter type
    *p = x->is_16 ? 1 : 0;  p += 1;
    p += 2; // padding

    enc_LE_u64(p, x->seed); p += 8;
    enc_LE_u32(p, x->size); p += 4;
    enc_LE_u32(p, x->n);    p += 4;

    uint64_t pad = pagesz() - (p - st);
    return p + pad;
}

/*
 * Read header at 'p' into Xorfilter 'x' and advance p to next
 * readable boundary.
 */
static uint8_t*
rdhdr(uint8_t *p, Xorfilter *x)
{
    uint8_t *st = p;

    memset(x, 0, sizeof *x);

    if (memcmp(p, "XORF", 4) != 0) return 0;

    p += 4;
    if (*p != 1) return 0;  // we only support version #1

    p += 1;
    x->is_16 = *p;           p += 1;
    p += 2; // padding

    x->seed = dec_LE_u64(p); p += 8;
    x->size = dec_LE_u64(p); p += 4;
    x->n    = dec_LE_u64(p); p += 4;

    uint64_t pad = pagesz() - (p - st);
    return p + pad;
}


// Marshal Xorfilter 'x' to file 'fname'
int
Xorfilter_marshal(Xorfilter *x, const char *fname)
{
    char file[PATH_MAX];
    int fd,
        r = 0;
    uint64_t xsz    = xorfilter_size(x);
    uint64_t filesz = xsz;   // total file size

    filesz += pagesz(); // Header is within the first page
    filesz += SHASIZE;  // trailer has SHA256

    snprintf(file, sizeof file, "%s.tmp.XXXXXX", fname);
    fd = mkostemp(file, 0);
    if (fd < 0) return -errno;

    if (ftruncate(fd, filesz) < 0) {
        r = errno;
        goto fail;
    }

    void *mptr = mmap(0, filesz, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (mptr == ((void *)-1)) {
        r = -errno;
        goto fail;
    }

    uint8_t *st = mptr;

    st = wrhdr(st, x);
    memcpy(st, x->ptr, xsz);   st += xsz;

    uint8_t sbuf[8];
    crypto_hash_sha256_state h;

    enc_LE_u64(sbuf, filesz);
    crypto_hash_sha256_init(&h);
    crypto_hash_sha256_update(&h, sbuf, 8);
    crypto_hash_sha256_update(&h, mptr, filesz - SHASIZE);
    crypto_hash_sha256_final(&h, st);

    munmap(mptr, filesz);
    fsync(fd);
    fdatasync(fd);
    close(fd);

    if (rename(file, fname) < 0) {
        r = errno;
        unlink(file);
        return -r;
    }
    return 0;

fail:
    close(fd);
    unlink(file);
    return -r;
}


// Unmarshal Xorfilter 'x' to file 'fname'
int
Xorfilter_unmarshal(Xorfilter **p_x, const char *fname, uint32_t flags)
{
    const int do_mmap = (flags & XORFILTER_MMAP);
    int r  = 0;
    int fd = open(fname, O_RDONLY);
    if (fd < 0) return -errno;

    struct stat st;
    if (fstat(fd, &st) < 0)  {
        r = errno;
        goto fail2;
    }

    if (st.st_size < (off_t)(SHASIZE + pagesz())) {
        r = EILSEQ;
        goto fail2;
    }

    void *mptr = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mptr == ((void *)-1)) {
        r = errno;
        goto fail2;
    }

    uint8_t *p = mptr;
    uint8_t sbuf[8];
    uint8_t sha[SHASIZE];
    crypto_hash_sha256_state h;

    enc_LE_u64(sbuf, st.st_size);
    crypto_hash_sha256_init(&h);
    crypto_hash_sha256_update(&h, sbuf, 8);
    crypto_hash_sha256_update(&h, mptr, st.st_size - SHASIZE);
    crypto_hash_sha256_final(&h, sha);

    if (sodium_memcmp(sha, p + st.st_size - SHASIZE, SHASIZE) != 0) {
        r = EILSEQ;
        goto fail1;
    }

    Xorfilter zx;
    p = rdhdr(p, &zx);
    if (!p) {
        r = EINVAL;
        goto fail1;
    }

    // Sanity check.
    if ((st.st_size - SHASIZE) < (off_t)xorfilter_size(&zx)) {
        r = E2BIG;
        goto fail1;
    }

    if (zx.size != xorfilter_calc_size(zx.n)) {
        r = EINVAL;
        goto fail1;
    }

    if (do_mmap) {
        zx.ptr     = p;
        zx.is_mmap = 1;
    } else {
        uint64_t sz = xorfilter_size(&zx);
        zx.ptr = NEWZA(uint8_t, sz);
        assert(zx.ptr);

        memcpy(zx.ptr, p, sz);
        munmap(mptr, st.st_size);
    }

    Xorfilter *x = NEWZ(Xorfilter);
    assert(x);
    assert(p_x);

    *x   = zx;
    *p_x = x;

    close(fd);
    return 0;

fail1:
    munmap(mptr, st.st_size);

fail2:
    close(fd);
    return -r;
}

/* EOF */
