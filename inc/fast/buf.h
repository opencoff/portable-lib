/* :vi:ts=4:sw=4:
 *
 * buf.h    - a growable fast buffer.
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

#ifndef __FAST_BUF_H__
#define __FAST_BUF_H__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h>
#include <stdint.h>
#include "utils/new.h"


struct fast_buf
{
    uint8_t *buf;
    size_t size;
    size_t cap;
};
typedef struct fast_buf fast_buf;


/*
 * Initialize the fast buffer with an initial size of 'sz'.
 */
static inline fast_buf*
fast_buf_init(fast_buf *b, size_t sz)
{
    if (sz == 0) sz = 128;

    b->buf  = NEWA(uint8_t, sz);
    b->size = 0;
    b->cap  = sz;
    return b;
}



/*
 * Finalize the fastbuf and free any storage that was
 * allocated.
 */
static inline void
fast_buf_fini(fast_buf *b)
{
    if (b->buf) {
        DEL(b->buf);
        b->buf  = 0;
        b->cap  = 0;
        b->size = 0;
    }
}

/*
 * Ensure that fast_buf has capacity of at least 'n' bytes
 */
static inline void
fast_buf_reserve(fast_buf *b, size_t want)
{
    if (want >= b->cap) {
        do { b->cap *= 2; } while (b->cap < want);

        b->buf = RENEW(uint8_t, b->buf, b->cap);
    }
}


/*
 * Grow the fast buf by 'n' bytes.
 */
static inline void
fast_buf_grow(fast_buf *b, size_t n)
{
    fast_buf_reserve(b, b->size + n);
}


/*
 * Append 'n' bytes from 'buf' to fast_buf 'b'
 */
static inline fast_buf*
fast_buf_append_buf(fast_buf *b, uint8_t *buf, size_t n)
{
    fast_buf_grow(b, n);
    memcpy(b->buf+b->size, buf, n);
    b->size += n;
    return b;
}


/*
 * Append 1 bytes to fast_buf 'b'
 */
static inline fast_buf*
fast_buf_append_byte(fast_buf *b, uint8_t c)
{
    fast_buf_grow(b, 1);
    b->buf[b->size++] = c;
    return b;
}


/*
 * Reset a fast buf to initial conditions.
 */
static inline fast_buf*
fast_buf_reset(fast_buf *b)
{
    b->size = 0;
    return b;
}



/*
 * Advance a fast buf ptr by 'n' bytes.
 */
static inline fast_buf*
fast_buf_advance(fast_buf *b, size_t n)
{
    b->size += n;
    return b;
}


/*
 * Return pointer to start of buffer
 */
static inline uint8_t*
fast_buf_ptr(fast_buf *b)
{
    return b->buf;
}

/*
 * Return size (length) of the buffer
 */
static inline size_t
fast_buf_len(fast_buf *b)
{
    return b->size;
}


/*
 * Return the capacity of the buffer
 */
static inline size_t
fast_buf_cap(fast_buf *b)
{
    return b->cap;
}




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __FAST_BUF_H__ */

/* EOF */
