/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * pack.c - Portable pack/unpack via template strings.
 *
 * Copyright (c) 2018 Sudhi Herle <sw at herle.net>
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
 * Documentation on the formats
 * ============================
 * * Pack format strings start with an optional endian indicator for
 *   multi-byte integers; the ">" endian indicator indicates big-endian
 *   encoding for multi-byte integers and "<" indicates little-endian
 *   encoding.
 *
 * * Each element that is packed/unpacked follows the same general
 *   scheme: 'nnnC' where 'nnn' is the optional decimal count and
 *   'C' is the format character. The optional count can be as large
 *   as a 32-bit unsigned integer.
 *
 *   The format-character must be one of:
 *       b, B:    byte
 *       s, S:    signed, unsigned 16-bit integer
 *       i, I:    signed, unsigned 32-bit integer
 *       l, L:    signed, unsigned 64-bit integer
 *       f:       32-bit IEEE float
 *       D:       64-bit IEEE float
 *       x:       padding (adds/consumes 0x0 bytes) without
 *                consuming input args.
 *       z, Z:    null-terminated string
 *       *:       Indicates that the next input argument will hold
 *                the count (for variable length arrays).
 *
 *   The optional decimal count indicates that the corresponding
 *   argument is a pointer to an array of the appropriate type.
 *   e.g., "9I" denotes encoding/decoding 9 consecutive unsigned
 *   32-bit integers from the next argument (which _must_ be an
 *   array or pointer).
 *
 *   If the repeat-count cannot be known a-priori but only available
 *   at run-time, Pack() and Unpack() support using '*' as the
 *   "count": this format character consumes one input argument and
 *   that argument must provide the repeat-count. The subsequent
 *   argument treated as a pointer will be required to have these
 *   many entries for encoding/decoding. e.g.,
 *       size_t n;          // number of entries in 'arr'
 *       uint16_t *arr;     // array of 16-bit unsigned ints
 *
 *       // Packs "n" 16-bit quantities found in 'arr'
 *       ssize_t psz = Pack("* I", n, arr);
 *
 *   In the above example, 'n' should contain the number of 16-bit
 *   entries in 'arr'.
 *
 *   The optional count when used with "Z" indicates a fixed length
 *   string. Excess bytes in the input are truncated and short
 *   strings are padded with '\0'.
 *
 *   Unpacking any string (fixed-length or VLA or null-terminated)
 *   will *always* get a trailing '\0'. i.e., it will *always* be a
 *   well-formed C string.
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>

#include "fast/encdec.h"
#include "utils/utils.h"

#include "utils/pack.h"

/*
 * internal struct to move data back/forth between various
 * functions.
 */
struct pdata {
    uint8_t *ptr;
    uint8_t *end;
};
typedef struct pdata pdata;

/*
 * An instance of packer represents the pack/unpack details for
 * exactly one type. A table of these is used to capture all
 * supported types.
 */
struct packer {
    uint8_t   typ;     // one of the types above.
    uint8_t  __pad0;
    uint16_t  size;    // packed/unpacked size
    ssize_t (*pack)(const struct packer*,   pdata*, size_t num, void *arg);
    ssize_t (*unpack)(const struct packer*, pdata*, size_t num, void *arg, int alloc);
};
typedef struct packer packer;

// fwd decl of table of endian specific packer/unpackers
static const packer le_packers[];
static const packer be_packers[];

/*
 * Internal functions to do the actual pack/unpack.
 */
static int unpack(const char *fmt, pdata *pd, va_list ap);
static int pack(const char *fmt, pdata *pd, va_list ap);
static const packer* getpacker(int c, const packer* pp);

/*
 * Pack into buf.
 *
 * Return > 0: Number of packed bytes.
 * Return < 0: -errno.
 */
ssize_t
Pack(uint8_t *buf, size_t bufsize, const char *fmt, ...)
{
    pdata pd = {
        .ptr = buf,
        .end = buf + bufsize,
    };
    va_list ap;
    int r;

    va_start(ap, fmt);
    r = pack(fmt, &pd, ap);
    va_end(ap);

    return r < 0 ? r : (pd.ptr - buf);
}

/*
 * Unpack from buf.
 *
 * Return > 0: Number of packed bytes.
 * Return < 0: -errno.
 */
ssize_t
Unpack(uint8_t *buf, size_t bufsize, const char *fmt, ...)
{
    pdata pd = {
        .ptr = buf,
        .end = buf + bufsize,
    };
    va_list ap;
    int r;

    va_start(ap, fmt);
    r = unpack(fmt, &pd, ap);
    va_end(ap);

    return r < 0 ? r : (pd.ptr - buf);
}

// extract a length-prefix from 'fmt' starting with the digit in 'c'.
#define __len_prefix(c, fmt) ({\
                                const size_t max = ~((size_t)0) / 10;       \
                                      size_t nn  = c - '0';                 \
                                for (++fmt, c = *fmt; c; ++fmt) {           \
                                    if (!isdigit(c)) break;                 \
                                    const size_t v = c - '0';               \
                                    if (nn > max ||                         \
                                            (nn == max && v > (~((size_t)0) % 10))) {   \
                                        return -E2BIG;                      \
                                    }                                       \
                                    nn *= 10;                               \
                                    nn += v;                                \
                                }                                           \
                                nn;                                         \
                            })


// internal pack function that operates on va_list
static int
pack(const char *fmt, pdata *pd, va_list ap)
{
    const packer *pk = &le_packers[0];
    int c = *fmt;

    switch (c) {
        default:
            break;

        case '<':
            pk = &le_packers[0];
            ++fmt;
            break;

        case '>':
            pk = &be_packers[0];
            ++fmt;
            break;
    }

    uint32_t vnum      = 0;
    int      have_vnum = 0;
    for (; (c = *fmt); ++fmt) {

        if (isspace(c)) continue;

        if (c == '*') {
            const packer *p = getpacker('I', pk);
            assert(p);
            assert(p->size > 1);

            vnum = va_arg(ap, uint32_t);

            /*
             * For VLAs we need to first encode a length followed by the
             * actual entity.
             */

            if ((pd->ptr + p->size) > pd->end)
               return -ENOSPC;

            c = p->pack(p, pd, 1, (void *)(ptrdiff_t)vnum);
            if (c < 0) return c;

            have_vnum = 1;
            continue;
        }

        size_t num = isdigit(c) ? __len_prefix(c, fmt) : 0;

        // Three rules:
        // 1. VLA and length-prefix can't both be used.
        // 2. VLA always overrides any other length-prefix.
        // 3. length-prefix is always 1 except for strings (Z)
        //
        // Corrollary for strings:
        // * VLA _or_ length-prefix for Z implies a _fixed_ length
        //   string(null filled for short strings; clamped for longer
        //   strings).
        // * if neither VLA or length-prefix is present, we
        //   explicitly use strlen()
        // * empty pointers for strings is same as zero-length
        //   string.

        if (num > 0 && have_vnum) return -EINVAL;

        const packer *p = getpacker(c, pk);
        if (!p) return -ENOENT;

        assert(p->pack);

        // We always pull apart a generic pointer; each
        // packer/unpacker knows what to do with it.
        void *arg = 0;

        switch (c) {
            case 'x':
                break;  // do NOT consume an arg!

            case 'z':   // fallthrough
            case 'Z':
                arg = va_arg(ap, void *);
                if (have_vnum) {
                    num = vnum;
                    if (0 == num) goto next;
                } else {
                    // default is to copy everything include trailing '\0'
                    if (num == 0 && arg) num = 1+strlen((const char*)arg);
                }
                break;

            default:
                arg = va_arg(ap, void *);
                if (have_vnum) {
                    num = vnum;
                    if (0 == num) goto next;
                } else {
                    // All other types need _atleast_ one arg.
                    // zero-length strings with no VLA will get an
                    // implicit '\0' (i.e., encoded-length == 1)
                    if (0 == num) num = 1;
                }
        }

        // pad and strings get extra validations. Most other types
        // are easy to validate.
        if (p->size > 0 && ((pd->ptr + (num * p->size)) > pd->end))
           return -ENOSPC;

        c = p->pack(p, pd, num, arg);
        if (c < 0) return c;

next:
        have_vnum = 0;
        vnum = 0;
    }

    return 0;
}


static int
unpack(const char *fmt, pdata *pd, va_list ap)
{
    const packer *pk = &le_packers[0];
    int c = *fmt;

    switch (c) {
        default:
            break;

        case '<':
            pk = &le_packers[0];
            ++fmt;
            break;

        case '>':
            pk = &be_packers[0];
            ++fmt;
            break;
    }

    uint32_t vnum      = 0;
    int      have_vnum = 0;
    for (; (c = *fmt); ++fmt) {
        if (isspace(c)) continue;

        if (c == '*') {
            const packer *p = getpacker('I', pk);
            assert(p);
            assert(p->size > 1);

            uint32_t *pv = va_arg(ap, uint32_t*);

            if (!pv) return -EINVAL;
            if ((pd->ptr + p->size) > pd->end) return -ENOSPC;


            c = p->unpack(p, pd, 1, pv, 0);
            if (c < 0) return c;

            // We now will decode vnum entities next.
            vnum = *pv;
            have_vnum = 1;
            continue;
        }

        size_t num = isdigit(c) ? __len_prefix(c, fmt) : 0;

        if (num > 0 && have_vnum) return -EINVAL;

        // Everything is pointers when we unpack. So, they better be
        // valid.
        void *arg = 0;
        const packer *p = getpacker(c, pk);
        if (!p) return -ENOENT;

        assert(p->unpack);

        // We have to treat unadorned 'Z' separately: decode till
        // '\0'
        switch (c) {
            case 'x':   // we don't consume an arg!
                break;

            default:
                arg = va_arg(ap, void *);
                if (!arg) return -EINVAL;

                if (have_vnum) {
                    num = vnum;
                    if (num == 0) goto next;
                } else {
                    // For all types except z/Z, we have atleast _one_ element
                    if (c != 'z' && c != 'Z') {
                        if (num == 0) num = 1;
                    }
                }
        }


        // pad and strings get extra validations. Most other types
        // are easy to validate.
        if (p->size > 0 && ((pd->ptr + (num * p->size)) > pd->end))
           return -ENOSPC;

        c = p->unpack(p, pd, num, arg, have_vnum == 1);
        if (c < 0) return c;

next:
        have_vnum = 0;
        vnum = 0;
    }

    // missing type descriptor after '*'
    if (have_vnum) return -EINVAL;

    return 0;
}

// 8 bit packer/unpackers

static ssize_t
b_pack(const packer *p, pdata *pd, size_t num, void *arg)
{
    if (num == 1) {
        ptrdiff_t zz = (ptrdiff_t)arg;
        uint8_t x = zz & 0xff;

        *pd->ptr = x;
        pd->ptr += 1;
        return 1;
    }

    uint8_t *x = arg;

    USEARG(p);
    memcpy(pd->ptr, x, num);
    pd->ptr += num;
    return num;
}

static ssize_t
b_unpack(const packer *p, pdata *pd, size_t num, void *arg, int alloc)
{
    uint8_t *x = arg;

    if (alloc) {
        uint8_t **p_x = arg;

        x = NEWZA(uint8_t, num);
        if (!x) return -ENOMEM;
        *p_x = x;
    }

    USEARG(p);
    memcpy(x, pd->ptr, num);
    pd->ptr += num;
    return num;
}

// 16 bit packer/unpackers

static ssize_t
le_s_pack(const packer *p, pdata *pd, size_t num, void *arg)
{
    if (num == 1) {
        ptrdiff_t zz = (ptrdiff_t)arg;
        uint16_t  x  = zz & 0xffff;

        pd->ptr = enc_LE_u16(pd->ptr, x);
        return p->size;
    }

    uint16_t *x = arg;
    uint8_t *z  = pd->ptr;
    size_t i;

    for (i = 0; i < num; ++i, ++x) {
        z = enc_LE_u16(z, *x);
    }
    pd->ptr = z;
    return num * p->size;
}

static ssize_t
le_s_unpack(const packer *p, pdata *pd, size_t num, void *arg, int alloc)
{
    uint16_t *x = arg;
    uint8_t *z  = pd->ptr;
    size_t i;

    if (alloc) {
        uint16_t **p_x = arg;

        x = NEWZA(uint16_t, num);
        if (!x) return -ENOMEM;
        *p_x = x;
    }

    for (i = 0; i < num; ++i, ++x) {
        *x = dec_LE_u16(z);
        z += p->size;
    }
    pd->ptr = z;
    return num * p->size;
}

static ssize_t
be_s_pack(const packer *p, pdata *pd, size_t num, void *arg)
{
    if (num == 1) {
        ptrdiff_t zz = (ptrdiff_t)arg;
        uint16_t x = zz & 0xffff;

        pd->ptr = enc_BE_u16(pd->ptr, x);
        return p->size;
    }

    uint16_t *x = arg;
    uint8_t *z  = pd->ptr;
    size_t i;

    for (i = 0; i < num; ++i, ++x) {
        z = enc_BE_u16(z, *x);
    }
    pd->ptr = z;
    return num * p->size;
}

static ssize_t
be_s_unpack(const packer *p, pdata *pd, size_t num, void *arg, int alloc)
{
    uint16_t *x = arg;
    uint8_t *z  = pd->ptr;
    size_t i;

    if (alloc) {
        uint16_t **p_x = arg;

        x = NEWZA(uint16_t, num);
        if (!x) return -ENOMEM;
        *p_x = x;
    }

    for (i = 0; i < num; ++i, ++x) {
        *x = dec_BE_u16(z);
        z += p->size;
    }
    pd->ptr = z;
    return num * p->size;
}



// 32 bit packer/unpackers

static ssize_t
le_i_pack(const packer *p, pdata *pd, size_t num, void *arg)
{
    if (num == 1) {
        ptrdiff_t zz = (ptrdiff_t)arg;
        uint32_t x = zz & 0xffffffff;

        pd->ptr = enc_LE_u32(pd->ptr, x);
        return p->size;
    }

    uint32_t *x = arg;
    uint8_t *z  = pd->ptr;
    size_t i;

    for (i = 0; i < num; ++i, ++x) {
        z = enc_LE_u32(z, *x);
    }
    pd->ptr = z;
    return num * p->size;
}

static ssize_t
le_i_unpack(const packer *p, pdata *pd, size_t num, void *arg, int alloc)
{
    uint32_t *x = arg;
    uint8_t *z  = pd->ptr;
    size_t i;

    if (alloc) {
        uint32_t **p_x = arg;

        x = NEWZA(uint32_t, num);
        if (!x) return -ENOMEM;
        *p_x = x;
    }

    for (i = 0; i < num; ++i, ++x) {
        *x = dec_LE_u32(z);
        z += p->size;
    }
    pd->ptr = z;
    return num * p->size;
}

static ssize_t
be_i_pack(const packer *p, pdata *pd, size_t num, void *arg)
{
    if (num == 1) {
        ptrdiff_t zz = (ptrdiff_t)arg;
        uint32_t x = zz & 0xffffffff;

        pd->ptr = enc_BE_u32(pd->ptr, x);
        return p->size;
    }

    uint32_t *x = arg;
    uint8_t *z  = pd->ptr;
    size_t i;

    for (i = 0; i < num; ++i, ++x) {
        z = enc_BE_u32(z, *x);
    }
    pd->ptr = z;
    return num * p->size;
}

static ssize_t
be_i_unpack(const packer *p, pdata *pd, size_t num, void *arg, int alloc)
{
    uint32_t *x = arg;
    uint8_t *z  = pd->ptr;
    size_t i;

    if (alloc) {
        uint32_t **p_x = arg;

        x = NEWZA(uint32_t, num);
        *p_x = x;
    }

    for (i = 0; i < num; ++i, ++x) {
        *x = dec_BE_u32(z);
        z += p->size;
    }
    pd->ptr = z;
    return num * p->size;
}


// 64-bit packer/unpackers

static ssize_t
le_l_pack(const packer *p, pdata *pd, size_t num, void *arg)
{
    if (num == 1) {
        uint64_t x = (uint64_t)arg;

        pd->ptr = enc_LE_u64(pd->ptr, x);
        return p->size;
    }

    uint64_t *x = arg;
    uint8_t *z  = pd->ptr;
    size_t i;

    for (i = 0; i < num; ++i, ++x) {
        z = enc_LE_u64(z, *x);
    }
    pd->ptr = z;
    return num * p->size;
}

static ssize_t
le_l_unpack(const packer *p, pdata *pd, size_t num, void *arg, int alloc)
{
    uint64_t *x = arg;
    uint8_t *z  = pd->ptr;
    size_t i;

    if (alloc) {
        uint64_t **p_x = arg;

        x = NEWZA(uint64_t, num);
        if (!x) return -ENOMEM;
        *p_x = x;
    }

    for (i = 0; i < num; ++i, ++x) {
        *x = dec_LE_u64(z);
        z += p->size;
    }
    pd->ptr = z;
    return num * p->size;
}

static ssize_t
be_l_pack(const packer *p, pdata *pd, size_t num, void *arg)
{
    if (num == 1) {
        uint64_t x = (uint64_t)arg;

        pd->ptr = enc_BE_u64(pd->ptr, x);
        return p->size;
    }

    uint64_t *x = arg;
    uint8_t *z  = pd->ptr;
    size_t i;

    for (i = 0; i < num; ++i, ++x) {
        z = enc_BE_u64(z, *x);
    }
    pd->ptr = z;
    return num * p->size;
}

static ssize_t
be_l_unpack(const packer *p, pdata *pd, size_t num, void *arg, int alloc)
{
    uint64_t *x = arg;
    uint8_t *z  = pd->ptr;
    size_t i;

    if (alloc) {
        uint64_t **p_x = arg;

        x = NEWZA(uint64_t, num);
        if (!x) return -ENOMEM;
        *p_x = x;
    }

    for (i = 0; i < num; ++i, ++x) {
        *x = dec_BE_u64(z);
        z += p->size;
    }
    pd->ptr = z;
    return num * p->size;
}

// x and z  packer/unpackers

static ssize_t
x_pack(const packer *p, pdata *pd, size_t num, void *arg)
{
    USEARG(p);
    USEARG(arg);

    if ((pd->ptr + num) > pd->end) return -EINVAL;

    memset(pd->ptr, 0, num);
    pd->ptr += num;
    return num;
}

static ssize_t
x_unpack(const packer *p, pdata *pd, size_t num, void *arg, int alloc)
{
    USEARG(p);
    USEARG(arg);
    USEARG(alloc);

    if ((pd->ptr + num) > pd->end) return -EINVAL;

    pd->ptr += num;
    return num;
}

/*
 * Pack strings; we pack two types of strings:
 *
 * 1) Strings with trailing '\0' (unadorned 'Z' fmt)
 * 2) Exact length strings with padding/truncation as needed ('*' or
 *    "nnZ" fmt)
 */
static ssize_t
z_pack(const packer *p, pdata *pd, size_t n, void *arg)
{
    uint8_t   *st = pd->ptr;
    const char *s = arg;

    USEARG(p);

    // we are _never_ sent zero sized requests
    assert(n > 0);

    if ((pd->ptr + n) > pd->end) return -ENOSPC;

    if (s) {
        size_t m = strlen(s);
        size_t z = m < n ? m : n;

        memcpy(pd->ptr, s, z);
        n       -= z;
        pd->ptr += z;
    }

    switch (n) {
        case 0:
            break;

        case 1: // common case
            *pd->ptr = 0;
            break;

        default:
            memset(pd->ptr, 0, n);
            break;
    }

    pd->ptr += n;
    return pd->ptr - st;
}


/*
 * For unpacking strings, we *always* allocate memory.
 * And 'alloc' indicates that a VLA was present in the format
 * string.
 */
static ssize_t
z_unpack(const packer *p, pdata *pd, size_t num, void *arg, int alloc)
{
    char **ps  = arg;
    uint8_t *s = pd->ptr;

    USEARG(p);
    USEARG(alloc);

    // strings are encoded in one of two ways:
    //   a) with a trailing '\0'
    //   b) exact length (truncated or padded as needed)

    if (num == 0) {
        // We encode the trailing '\0' for 'Z'
        // And it's an error to _not_ have a trailing '\0'
        while (*s++) {
            if (s == pd->end) return -EINVAL;
        }
    } else {
        // Exact length for either "*" or "nnZ" formats
        const size_t max = pd->end - s;
        if (num > max) return -ENOSPC;
        s += num;
    }

    size_t n = s - pd->ptr;
    char  *z = NEWZA(char, n+1);
    if (!z) return -ENOMEM;

    if (n > 0) memcpy(z, pd->ptr, n);

       z[n]  = 0;
    pd->ptr += n;
        *ps  = z;

    return n;
}


/*
 * Definition of le and be packers/unpackers below.
 *
 * while packing/unpacking - we don't distinguish the signed vs
 * unsigned variants. To us they are n-bit encoded entities.
 */


#if 1

// fast lookup table at the expense of memory!
#define _x(a, sz, pp, uu)   [a] = {a, 0, sz, pp, uu}

static const packer*
getpacker(int c, const packer* pp)
{
    const packer *p = &pp[c];

    return p->typ == c ? p : 0;
}

#else

// Slow - but compact format lookup table.

#define _x(a, sz, pp, uu)   {a, 0, sz, pp, uu}

// Get pack/unpack actor for format char 'c'.
static const packer*
getpacker(int c, const packer* pp)
{
    const packer *p = pp;

    for (; p->typ > 0; ++p) {
        if (c == p->typ) return p;
    }

    return 0;
}
#endif

static const packer le_packers[] = {

    _x('b', 1, b_pack, b_unpack),
    _x('B', 1, b_pack, b_unpack),
    _x('x', 0, x_pack, x_unpack),
    _x('z', 1, z_pack, z_unpack),
    _x('Z', 1, z_pack, z_unpack),

    _x('s', 2, le_s_pack, le_s_unpack),
    _x('S', 2, le_s_pack, le_s_unpack),

    _x('i', 4, le_i_pack, le_i_unpack),
    _x('I', 4, le_i_pack, le_i_unpack),

    _x('l', 8, le_l_pack, le_l_unpack),
    _x('L', 8, le_l_pack, le_l_unpack),

    _x('f', 4, le_i_pack, le_i_unpack),
    _x('D', 8, le_l_pack, le_l_unpack),

    _x(0, 0, 0, 0)  // always in the end

};

static const packer be_packers[] = {

    _x('b', 1, b_pack, b_unpack),
    _x('B', 1, b_pack, b_unpack),
    _x('x', 0, x_pack, x_unpack),
    _x('z', 1, z_pack, z_unpack),
    _x('Z', 1, z_pack, z_unpack),

    _x('s', 2, be_s_pack, be_s_unpack),
    _x('S', 2, be_s_pack, be_s_unpack),

    _x('i', 4, be_i_pack, be_i_unpack),
    _x('I', 4, be_i_pack, be_i_unpack),

    _x('l', 8, be_l_pack, be_l_unpack),
    _x('L', 8, be_l_pack, be_l_unpack),

    _x('f', 4, be_i_pack, be_i_unpack),
    _x('D', 8, be_l_pack, be_l_unpack),

    _x(0, 0, 0, 0)  // always in the end
};


/* EOF */
