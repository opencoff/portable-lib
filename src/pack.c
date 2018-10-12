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
 * Notes
 * =====
 * * The template string for pack/unpack model similar ones in
 *   Python's struct module.
 * * While encoding or decoding, no distinction is made of signed
 *   vs. unsigned quantities.
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
    char   typ;     // one of the types above.
    size_t size;    // packed/unpacked size
    ssize_t (*pack)(const struct packer*,   pdata*, size_t num, void *arg);
    ssize_t (*unpack)(const struct packer*, pdata*, size_t num, void *arg, int alloc);
};
typedef struct packer packer;

// fwd decl of table of endian specific packer/unpackers
const packer le_packers[];
const packer be_packers[];

/*
 * Internal functions to do the actual pack/unpack.
 */
static int unpack(const char *fmt, pdata *pd, va_list ap);
static int pack(const char *fmt, pdata *pd, va_list ap);

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

    if (r < 0) return r;

    return pd.ptr - buf;
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

    if (r < 0) return r;

    return pd.ptr - buf;
}

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


// internal pack function that operates on va_list
static int
pack(const char *fmt, pdata *pd, va_list ap)
{
    const packer *pk = &le_packers[0];
    int c = *fmt;

    switch (c) {
        case '<':
            pk = &le_packers[0];
            ++fmt;
            break;

        case '>':
            pk = &be_packers[0];
            ++fmt;
            break;

        default:
            break;
    }

    uint32_t vnum = 0;
    int      have_vnum = 0;
    for (; (c = *fmt); ++fmt) {
        size_t num = 0;

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

        if (isdigit(c)) {
            const size_t max = ~((size_t)0) / 10;
            num = c - '0';
            for (++fmt, c = *fmt; c; ++fmt) {
                if (!isdigit(c)) break;

                size_t v = c - '0';
                if (num > max || 
                        (num == max && v > (~((size_t)0) % 10))) {
                    return -E2BIG;
                }
                num *= 10;
                num += v;
            }
        }

        const packer *p = getpacker(c, pk);
        if (!p) return -ENOENT;

        // Two rules:
        // * length prefix is always 1 except for strings (Z)
        // * VLA always overrides any other length prefix.

        // We always pull apart a generic pointer; each
        // packer/unpacker knows what to do with it.
        void *arg = va_arg(ap, void *);

        switch (c) {
            case 'z':   // fallthrough
            case 'Z':
                // we let the string packer call strlen() and
                // calculate the correct number of bytes.
                // If num > 0, it serves as an upper bound.
                // In particular, we need a way to encode
                // zero-length strings (with a solo 0).

                // Gah. Without this, we have no way to
                // distinguish a clamping value vs. actual
                // value.
                num = have_vnum ? vnum : strlen((const char*)arg);
                break;

            default:
                if (have_vnum) {
                    num = vnum;
                    if (0 == num) continue;
                } else {

                    // All other types need _atleast_ one arg.
                    if (0 == num) num = 1;
                }
        }

        // pad and strings get extra validations. Most other types
        // are easy to validate.
        if (p->size > 0 && ((pd->ptr + (num * p->size)) > pd->end))
           return -ENOSPC;

        c = p->pack(p, pd, num, arg);
        if (c < 0) return c;

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
        case '<':
            pk = &le_packers[0];
            ++fmt;
            break;

        case '>':
            pk = &be_packers[0];
            ++fmt;
            break;

        default:
            break;
    }

    uint32_t vnum = 0;
    int      have_vnum = 0;
    for (; (c = *fmt); ++fmt) {
        size_t num = 0;

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

        if (isdigit(c)) {
            const size_t max = ~((size_t)0) / 10;
            num = c - '0';
            for (++fmt, c = *fmt; c; ++fmt) {
                if (!isdigit(c)) break;

                size_t v = c - '0';
                if (num > max || 
                        (num == max && v > (~((size_t)0) % 10))) {
                    return -E2BIG;
                }
                num *= 10;
                num += v;
            }
        }

        // XXX What takes precedence if vnum > 0 and num > 0?
        //     e.g., "* 8I"

        const packer *p = getpacker(c, pk);
        if (!p) return -ENOENT;

        // string encodings are special. We must be able to detect
        // empty strings.
        switch (c) {
            case 'z':   // fallthrough
            case 'Z':
                // we let the string packer call strlen() and
                // calculate the correct number of bytes.
                // If num > 0, it serves as an upper bound.
                // In particular, we need a way to decode
                // zero-length strings (with a solo 0).
                if (have_vnum) {
                    num = vnum;
                }
                break;

            default:
                if (have_vnum) {
                    num = vnum;
                    if (0 == num) continue;
                } else {

                    // All other types need _atleast_ one arg.
                    if (0 == num) num = 1;
                }
        }

        // pad and strings get extra validations. Most other types
        // are easy to validate.
        if (p->size > 0 && ((pd->ptr + (num * p->size)) > pd->end))
           return -ENOSPC;

        // We always pull apart a generic pointer; each
        // packer/unpacker knows what to do with it.
        void *arg = va_arg(ap, void *);

        // Everything is pointers when we unpack. So, they better be
        // valid.
        if (!arg) return -EINVAL;

        c = p->unpack(p, pd, num, arg, have_vnum == 1);
        if (c < 0) return c;

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
 * We are always passed at least the length of the string.
 */
static ssize_t
z_pack(const packer *p, pdata *pd, size_t n, void *arg)
{
    const char *s = arg;

    USEARG(p);
    if (!s) return -EINVAL;

    if ((pd->ptr + n+1) > pd->end) return -ENOSPC;

    // If caller told us to only pack() a fixed number of chars,
    // then we must explicitly null-terminate. To avoid special
    // casing this, we *always* null terminate.
    if (n > 0) memcpy(pd->ptr, s, n);
    pd->ptr[n] = 0;
    pd->ptr += n+1;
    return n+1;
}

/*
 * For unpacking strings, we *always* allocate memory.
 * Note: strings are encoded with their trailing 0.
 */
static ssize_t
z_unpack(const packer *p, pdata *pd, size_t num, void *arg, int alloc)
{
    char **ps  = arg;
    uint8_t *s = pd->ptr;
    size_t n;

    USEARG(p);
    USEARG(alloc);

    // we can't use strlen() because the input may not have a \0 in
    // it. So, we have to find the end without over-running the input
    // buffer.
    for (; *s; ++s) {
        if (s == pd->end) return -EINVAL;
    }

    n = (s - pd->ptr);

    char *z = NEWZA(char, n+1);
    if (!z) return -ENOMEM;

    if (n > 0) memcpy(z, pd->ptr, n);
    z[n] = 0;

    // If we have a clamping value, we ignore it.
    // z[num] = 0;
    USEARG(num);

    // we start the *next* processing after the trailing \0
    pd->ptr += n+1;
    *ps = z;

    return n+1;
}


/*
 * Definition of le and be packers/unpackers below.
 *
 * while packing/unpacking - we don't distinguish the signed vs
 * unsigned variants. To us they are n-bit encoded entities.
 */
const packer le_packers[] = {
      {'b', 1, b_pack,    b_unpack}
    , {'B', 1, b_pack,    b_unpack}

    , {'s', 2, le_s_pack, le_s_unpack}
    , {'S', 2, le_s_pack, le_s_unpack}

    , {'i', 4, le_i_pack, le_i_unpack}
    , {'I', 4, le_i_pack, le_i_unpack}

    , {'l', 8, le_l_pack, le_l_unpack}
    , {'L', 8, le_l_pack, le_l_unpack}

    , {'f', 4, le_i_pack, le_i_unpack}
    , {'D', 8, le_l_pack, le_l_unpack}

    , {'x', 0, x_pack,    x_unpack}
    , {'z', 1, z_pack,    z_unpack}
    , {'Z', 1, z_pack,    z_unpack}

    , {0, 0, 0, 0}  // always in the end
};

const packer be_packers[] = {
      {'b', 1, b_pack,    b_unpack}
    , {'B', 1, b_pack,    b_unpack}

    , {'s', 2, be_s_pack, be_s_unpack}
    , {'S', 2, be_s_pack, be_s_unpack}

    , {'i', 4, be_i_pack, be_i_unpack}
    , {'I', 4, be_i_pack, be_i_unpack}

    , {'l', 8, be_l_pack, be_l_unpack}
    , {'L', 8, be_l_pack, be_l_unpack}

    , {'f', 4, be_i_pack, be_i_unpack}
    , {'D', 8, be_l_pack, be_l_unpack}

    , {'x', 0, x_pack,    x_unpack}
    , {'z', 1, z_pack,    z_unpack}
    , {'Z', 1, z_pack,    z_unpack}

    , {0, 0, 0, 0}  // always in the end
};

/* EOF */
