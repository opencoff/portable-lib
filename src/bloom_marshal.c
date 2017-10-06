/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * bloom_marshal.c - Serialize/De-serialize bloom filters.
 *
 * Copyright (c) 2015 Sudhi Herle <sw at herle.net>
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
 * o  See commentary in bloom.c.
 * o  All integers are in little endian order - to make it easy for
 *    the most common arch to just read it off the disk.
 * o  The filter data always starts at a 64-byte boundary.
 * o  For version 1 we always use SHA256 as the checksum.
 *
 *
 * Disk layout of data:
 * --------------------
 *
 * - 64 byte header:
 *     o magic           4
 *     o version         1  -- monotonically increasing
 *     o type            1  -- one of the BLOOM_TYPE_xxx values
 *     o hash func       1  -- one of BLOOM_HASH_xxx values
 *     o checksum algo   1  -- one of BLOOM_CKSUM_xxx values
 *     o total entries   8  -- number of entries in the bloom filter
 *     o error ratio     8  -- the error ration 'e' the filter was created with
 *     o marshalled size 8  -- total number of marshalled bytes
 *     o ZEROES          -  -- padding to 64 bytes
 *
 * - Filter Directory:
 *     o N filters       4  -- number of scalable bloom filters (1 for everyone else)
 *     o scale           4  -- scale factor for scalable blooms; zero otherwise
 *     o 'r'             8  -- tightening ratio for error prob: only for scalable bloom filters
 *     o N entries of file offset of filter-data
 *          - off        8  -- offset where the filter bits actually start (offset 0 is start of file)
 *
 *  - N entries of filter data:
 *     o m      8 -- number of slots
 *     o k      8 -- number of hash functions
 *     o salt   8 -- hash salt
 *     o size   8 -- number of filter entries
 *     o bmsize 8 -- size of the filter bitmap
 *     o bitmap N bytes (bmsize bytes)
 *
 * - Last 'n' bytes of the marshaled data is the checksum over the
 *   _entire_ file (including header, blank spots and everything).
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
#include <math.h>
#include <limits.h>

#include "utils/bloom.h"
#include "utils/utils.h"
#include "fast/encdec.h"
#include "fast/vect.h"

// Need libsodium to be installed.
#include "sodium.h"
#include "bloom_internal.h"


/* Darwin doesn't have fdatasync() prototype */
#ifdef __Darwin__
extern int fdatasync(int);
#endif // __Darwin__


/*
 * 64 bytes of header. See description above.
 */
#define HDRSIZ      64

/*
 * Header of each individual filter:
 */
#define FILT_HDRSIZ 40

#define BLOOM_BLAKE2b_SIZE    crypto_generichash_BYTES
#define BLOOM_SHA256_SIZE     crypto_hash_sha256_BYTES

#define max_CKSUMSZ           (BLOOM_BLAKE2b_SIZE > BLOOM_SHA256_SIZE ? BLOOM_BLAKE2b_SIZE : BLOOM_SHA256_SIZE)

/*
 * Encapculates checksum and verify operation. We support two
 * checksum algorithms for now, the abstraction below allows us to
 * add more in the future.
 */
struct checksummer
{
    void     (*init)(struct checksummer*);
    void     (*update)(struct checksummer*, void* buf, size_t n);
    void     (*final)(struct checksummer*,  void* out);
    size_t   size;
    int      type;

    // Common storage
    union {
        crypto_generichash_state blake2;
        crypto_hash_sha256_state sha256;
    };
};
typedef struct checksummer checksummer;


// Array of offsets
struct offpair
{
    // Offset where we will write the filter header
    uint64_t hdroff;

    // offset where we will write the filter bytes
    uint64_t dataoff;
};
typedef struct offpair offpair;

VECT_TYPEDEF(off_vect, offpair);


/*
 * Common state for marshal/unmarshal.
 */
struct mstate
{
    Bloom *b;

    // Size of the actual data bytes (excluding the trailing
    // checksum).
    uint64_t datasize;

    // Vector of offsets for filter data.
    off_vect    offs;

    // checksummer instance
    checksummer ck;
};
typedef struct mstate mstate;



/*
 * Implementation of various checksum algorithms:
 *
 * We always use the length of the buffers in calculating the
 * hash. This avoids length extension attacks. It's a good
 * discipline to cultivate when using hash functions.
 */


static void
blake2b_init(checksummer* ck)
{
    crypto_generichash_state* st = &ck->blake2;
    crypto_generichash_init(st, 0, 0, ck->size);
};


static void
blake2b_update(checksummer* ck, void* buf, size_t n)
{
    uint8_t b0[8];
    crypto_generichash_state* st = &ck->blake2;

    enc_BE_u64(b0, n);
    crypto_generichash_update(st, buf, n);
    crypto_generichash_update(st, b0,  8);
}


static void
blake2b_final(checksummer* ck, void* out)
{
    crypto_generichash_final(&ck->blake2, out, ck->size);
    sodium_memzero(&ck->blake2, sizeof ck->blake2);
}

static void
sha256_init(checksummer* ck)
{
    crypto_hash_sha256_state* st = &ck->sha256;
    crypto_hash_sha256_init(st);
};


static void
sha256_update(checksummer* ck, void* buf, size_t n)
{
    uint8_t b0[8];
    crypto_hash_sha256_state* st = &ck->sha256;

    enc_BE_u64(b0, n);
    crypto_hash_sha256_update(st, buf, n);
    crypto_hash_sha256_update(st, b0,  8);
}


static void
sha256_final(checksummer* ck, void* out)
{
    crypto_hash_sha256_final(&ck->sha256, out);
    sodium_memzero(&ck->sha256, sizeof ck->sha256);
}



/*
 * Map of checksum types to the handlers
 * Keep this array in-sync with the #defines above.
 */
#define _x(a, b, c) { .init=a ## _init, .update=a ## _update, .final=a ## _final, .size=b, .type=c}
static const checksummer Checksums[] =  {
      _x(sha256,  BLOOM_SHA256_SIZE,  BLOOM_CKSUM_SHA256)
    , _x(blake2b, BLOOM_BLAKE2b_SIZE, BLOOM_CKSUM_BLAKE2b)
};


static inline void
cksum(checksummer *ck, uint8_t *out, uint8_t *buf, uint64_t sz)
{
    ck->init(ck);
    ck->update(ck, buf, sz);
    ck->final(ck, out);
}


// Write header, filter-offset directory and update checksum.
static uint8_t *
wrhdr(uint8_t *ptr, mstate *m)
{
    Bloom *b   = m->b;
    uint8_t *z = ptr;
    uint8_t *filtoff = z + HDRSIZ;  // start of filter-offset list
    off_vect *v      = &m->offs;

    memset(ptr, 0, HDRSIZ);
    memcpy(ptr, "BLOM", 4);     ptr += 4;

    *ptr = BLOOM_VER0;          ptr += 1;
    *ptr = b->typ;              ptr += 1;
    *ptr = BLOOM_HASH_FASTHALF; ptr += 1;
    *ptr = m->ck.type;          ptr += 1;
    ptr  = enc_LE_u64(ptr, b->n);
    ptr  = enc_LE_float64(ptr, b->e);
    ptr  = enc_LE_u64(ptr, m->datasize);

    assert((ptr-z) <= HDRSIZ);

    ptr = filtoff;
    if (b->typ == BLOOM_TYPE_SCALE) {
        scalable_bloom *sb = b->filter;
        uint32_t i;

        ptr = enc_LE_u32(ptr,     sb->len);
        ptr = enc_LE_u32(ptr,     sb->scale);
        ptr = enc_LE_float64(ptr, sb->r);

        for (i = 0; i < sb->len; i++) {
            offpair *o = &VECT_ELEM(v, i);

            // We only encode the start of the filter header.
            // We can find the filter data by looking at the next
            // 64-byte aligned boundary past the filter header.
            ptr  = enc_LE_u64(ptr, o->hdroff);
        }
    } else {
        offpair *o = &VECT_ELEM(v, 0);

        ptr = enc_LE_u32(ptr, 1);
        ptr += 12;  // 4 + 8 bytes of zeroes

        ptr = enc_LE_u64(ptr, o->hdroff);
    }

    return ptr;
}


// Calculate how big the marshal'd data is going to be
static uint64_t
calc_marshal_size(mstate *m)
{
    Bloom *b    = m->b;
    off_vect *v = &m->offs;
    uint64_t sz = HDRSIZ + 16;

    if (b->typ == BLOOM_TYPE_SCALE) {
        scalable_bloom *sb = b->filter;
        uint32_t i;

        sz += (sb->len * 8);    // table of offsets
        sz  = _ALIGN_UP(sz, 64);

        VECT_INIT(v, sb->len);

        for (i = 0; i < sb->len; i++) {
            bloom *f   = &sb->bfa[i];
            offpair *o = &VECT_GET_NEXT(v);

            o->hdroff = sz;
            sz += FILT_HDRSIZ;

            // We will write the actual filter bits at a
            // cached-aligned offset. This will be useful when we
            // read the filter back in mmap'd mode.
            o->dataoff = sz = _ALIGN_UP(sz, 64);

            sz += f->bmsize;
            sz  = _ALIGN_UP(sz, 64);
        }
    } else {
        bloom *f = b->filter;
        offpair *o;

        VECT_INIT(v, 1);

        sz += 8;    // one entry in the table
        o   = &VECT_GET_NEXT(v);

        o->hdroff = sz = _ALIGN_UP(sz, 64);
        sz += FILT_HDRSIZ;

        o->dataoff = sz = _ALIGN_UP(sz, 64);

        sz += f->bmsize;
        sz  = _ALIGN_UP(sz, 64);
    }

    // This is the actual number of marshal'd bytes we will write.
    // The checksum will come just after this.
    m->datasize = sz;

    return sz;
}

// marshal bloom filter 'b' beginning at 'ptr'
// 'ptr' is start of mmap'd base.
static void
wrfilter(uint8_t * start, offpair *o, bloom *b)
{
    uint8_t * h = start + o->hdroff;
    uint8_t * d = start + o->dataoff;
    uint8_t * z = h;

    h = enc_LE_u64(h,  b->m);
    h = enc_LE_u64(h,  b->k);
    h = enc_LE_u64(h,  b->salt);
    h = enc_LE_u64(h,  b->size);
    h = enc_LE_u64(h,  b->bmsize);

    assert((h - z) == FILT_HDRSIZ);

    memcpy(d, b->bitmap, b->bmsize);
}

// Read filter data from offset pair 'o' into bloom 'b'.
static int
rdfilter(uint8_t *start, offpair *o, bloom *b, int dommap)
{
    uint8_t * h = start + o->hdroff;
    uint8_t * d = start + o->dataoff;
    uint8_t * z = h;

    memset(b, 0, sizeof *b);

    b->m      = dec_LE_u64(h); h += 8;
    b->k      = dec_LE_u64(h); h += 8;
    b->salt   = dec_LE_u64(h); h += 8;
    b->size   = dec_LE_u64(h); h += 8;
    b->bmsize = dec_LE_u64(h); h += 8;
    b->e      = make_e(b->k);

    assert((h - z) == FILT_HDRSIZ);

    if (dommap) {
        b->bitmap = d;
        b->flags  = BLOOM_BITMAP_MMAP;
    } else {
        b->bitmap = NEWA(uint8_t, b->bmsize);
        if (!b->bitmap) return -ENOMEM;
        memcpy(b->bitmap, d, b->bmsize);
    }

    return 0;
}


/*
 * Read header and filter-offset directory. Assumes that the file
 * checksum is valid and verified.
 *
 * Return 0 on success, -errno on failure.
 */
static int
rdhdr(uint8_t *buf, uint64_t sz, mstate *m)
{
    uint64_t avail = sz,
             bn;
    uint8_t *p = buf;
    uint32_t n = 0,
             scale = 0;
    double r  = 0.0,
           be = 0.0;
    int typ;


    if (avail < (HDRSIZ+16))       return -EBADF;
    if (0 != memcmp(p, "BLOM", 4)) return -EBADF;

    p += 4;
    if (*p != BLOOM_VER0)   return -EBADF;

    p += 1;
    switch (*p) {
        case BLOOM_TYPE_SCALE:
        case BLOOM_TYPE_COUNTING:
        case BLOOM_TYPE_QUICK:
            typ = *p;
            break;

        default:
            return -EBADF;
    }

    p += 1;
    if (*p != BLOOM_HASH_FASTHALF) return -EBADF;

    p += 2;
    /*
     * We already have parsed the checksum type.
     */

    bn = dec_LE_u64(p);          p += 8;
    be = dec_LE_float64(p);      p += 8;
    m->datasize = dec_LE_u64(p); p += 8;

    assert((p-buf) <= HDRSIZ);

    if (m->datasize != sz) return -EBADF;

    avail -= HDRSIZ;
    p      = buf + HDRSIZ;

    // Validate offsets
    if (typ == BLOOM_TYPE_SCALE) {
        uint32_t i;

        n      = dec_LE_u32(p);     p += 4;
        scale  = dec_LE_u32(p);     p += 4;
        r      = dec_LE_float64(p); p += 8;

        // Sanity check
        if (n > 1048576)            return -E2BIG;

        VECT_INIT(&m->offs, n);

        avail -= 16;
        if (avail  < (n * 8)) return -EBADF;

        // Decode offsets
        for (i = 0; i < n; i++) {
            offpair *o = &VECT_GET_NEXT(&m->offs);

            o->hdroff  = dec_LE_u64(p);  p += 8;
            o->dataoff = _ALIGN_UP(o->hdroff + FILT_HDRSIZ, 64);

            if (o->hdroff  >= sz) return -EBADF;
            if (o->dataoff >= sz) return -EBADF;
        }
    } else {
        offpair *o;

        n = dec_LE_u32(p);
        if (n != 1)  return -E2BIG;

        p     += 16;    // skip over everything else
        avail -= 16;

        VECT_INIT(&m->offs, 1);

        o = &VECT_GET_NEXT(&m->offs);
        o->hdroff  = dec_LE_u64(p);
        o->dataoff = _ALIGN_UP(o->hdroff + FILT_HDRSIZ, 64);
        if (o->hdroff  >= sz) return -EBADF;
        if (o->dataoff >= sz) return -EBADF;
    }

    // At this point, we have not setup b->e. We will do that when
    // we read in the actual filter data.

    m->b = __alloc_bloom(typ, next_pow2(n));
    if (!m->b) return -ENOMEM;

    m->b->n = bn;
    m->b->e = be;

    if (typ == BLOOM_TYPE_SCALE) {
        scalable_bloom *sb = m->b->filter;

        sb->scale = scale;
        sb->len   = n;  // we have exactly 'n' filter levels
        sb->r     = r;
    }

    return 0;
}

static int
is_valid_cktyp(checksummer *ck, int typ)
{
    switch (typ) {
        case BLOOM_CKSUM_SHA256:
        case BLOOM_CKSUM_BLAKE2b:
            *ck = Checksums[typ];
            return 1;
            break;

        default:
            return 0;
    }
    return 0;
}

/*
 * Marshal bloom filter 'b' to file 'fname'.
 *
 * Writes the data and checksum in mmap'd mode.
 *
 * Returns 0 on success, -errno on failure.
 */
int
Bloom_marshal(Bloom *b, const char *fname)
{
    mstate m;
    char file[PATH_MAX];
    int fd, r = 0;
    uint64_t sz;

    memset(&m, 0, sizeof m);
    m.b  = b;
    m.ck = Checksums[BLOOM_CKSUM_DEFAULT];
    sz   = m.ck.size + calc_marshal_size(&m);

    snprintf(file, sizeof file, "%s.tmp.XXXXXX", fname);

    fd = mkostemp(file, 0);
    if (fd < 0) return -errno;

    if (ftruncate(fd, sz) < 0) {
        r = -errno;
        goto fail;
    }

    void *mptr = mmap(0, sz, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (mptr == ((void *)-1)) {
        r = -errno;
        goto fail;
    }

    uint8_t *start = mptr;
    wrhdr(start, &m);

    /*
     * Now write actual filter bytes
     */
    if (b->typ == BLOOM_TYPE_SCALE) {
        scalable_bloom *sb = b->filter;
        offpair *o;
        uint32_t i;

        VECT_FOR_EACHi(&m.offs, i, o) {
            bloom *f = &sb->bfa[i];

            wrfilter(start, o, f);
        }
    } else {
        bloom   *f = b->filter;
        offpair *o = &VECT_ELEM(&m.offs, 0);

        wrfilter(start, o, f);
    }

    /*
     * Calculate checksum from start m.datasize
     */
    cksum(&m.ck, start+m.datasize, start, m.datasize);
    VECT_FINI(&m.offs);
    munmap(mptr, sz);
    fsync(fd);
    fdatasync(fd);
    close(fd);
    if (rename(file, fname) < 0) {
        r = -errno;
        unlink(file);
        return r;
    }

    return 0;

fail:
    close(fd);
    unlink(file);
    return -r;
}



/*
 * Unmarshal bloom filter from file 'fname' into a newly allocated
 * filter '*p_b'.
 *
 * Reads the data and checksum in mmap'd mode. If flags is
 * BLOOM_MMAP, then the filter is used in-situ.
 *
 * Returns 0 on success, -errno on failure.
 */
int
Bloom_unmarshal(Bloom **p_b, const char *fname, uint32_t flags)
{
    mstate m;
    uint8_t ckcalc[max_CKSUMSZ];
    const int do_mmap = flags & BLOOM_BITMAP_MMAP;
    int fd, r;

    memset(&m, 0, sizeof m);

    fd = open(fname, O_RDONLY);
    if (fd < 0) return -errno;

    struct stat st;
    if (fstat(fd, &st) < 0) goto fail;

    // 16: Filter directory info
    if (st.st_size < (HDRSIZ+16+max_CKSUMSZ)) {
        errno = EILSEQ;
        goto fail;
    }

    void    *mptr  = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    uint8_t *start = mptr;
    if (mptr == ((void *)-1)) goto fail;

    /*
     * Byte #8 is the checksum type. We read this without verifying.
     * Chicken and egg problem.
     */
    if (!is_valid_cktyp(&m.ck, start[7])) {
        errno = EILSEQ;
        goto fail0;
    }

    // This is the size of the data blob (headers + filters)
    size_t  dsize  = st.st_size - m.ck.size;

    // Calculate checksum before we read any other data.
    // Checksum is in the last "n" bytes.
    cksum(&m.ck, ckcalc, mptr, dsize);
    if (0 != sodium_memcmp(ckcalc, start+dsize, m.ck.size)) {
        errno = EILSEQ;
        goto fail0;
    }

    r = rdhdr(start, dsize, &m);
    if (r < 0) {
        errno = -r;
        goto fail0;
    }

    /*
     * Now we have everything we need to build out the filters.
     * rdhdr() has already allocated the filter memory. We need to
     * decode it.
     */
    if (m.b->typ == BLOOM_TYPE_SCALE) {
        scalable_bloom *sb = m.b->filter;
        offpair *o;
        uint32_t i;

        VECT_FOR_EACHi(&m.offs, i, o) {
            bloom *f = &sb->bfa[i];

            r = rdfilter(start, o, f, do_mmap);
            if (r < 0) {
                errno = -r;
                goto fail0;
            }
        }
    } else {
        bloom   *f = m.b->filter;
        offpair *o = &VECT_ELEM(&m.offs, 0);

        rdfilter(start, o, f, do_mmap);
    }


    assert(p_b);
    *p_b = m.b;

    if (!do_mmap) munmap(mptr, st.st_size);


    /*
     * POSIX allows us to close() and still keep the memory mapping
     * as long as the process is alive.
     */
    close(fd);
    return 0;

fail0:
    munmap(mptr, st.st_size);

fail:
    r = -errno;
    close(fd);
    return r;
}


