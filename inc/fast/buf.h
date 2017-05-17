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
#include "utils/utils.h"
    

typedef struct fast_buf fast_buf;
struct fast_buf
{
    unsigned char * buf;
    int size;
    int capacity;
};



/*
 * Initialize the fast buffer with an initial size of 'sz'.
 */
#define fast_buf_init(b,sz)   do { \
    fast_buf * b_ = (b); \
    int sz_ = (sz); \
    memset (b_, 0, sizeof(*b_)); \
    b_->capacity = sz_; \
    b_->buf      = NEWA(unsigned char, sz_); \
} while (0)




/*
 * Finalize the fastbuf and free any storage that was
 * allocated.
 */
#define fast_buf_fini(b)      do { \
    DEL ((b)->buf);       \
} while (0)



/*
 * Internal macro to grow the buffer on demand.
 */
#define _fast_buf_expand(b,req) do { \
        fast_buf * b__ = (b); \
        int n__ = b__->capacity;     \
        int want__ = (req) + b__->size; \
        if  ( want__ >= n__ ) {       \
            do { n__ *= 2; } while (n__ < want__); \
            b__->buf = RENEWA(unsigned char, b__->buf, n__);\
            b__->capacity = n__;     \
        }                           \
    } while (0)




/*
 * Push a sequence of bytes to the end of the buffer.
 */
#define fast_buf_push(b,d,l)    do { \
    fast_buf * b_ = (b); \
    int l_ = (l); \
    _fast_buf_expand(b_,l_);      \
    memcpy (b_->buf+b_->size, (d), l_); \
    b_->size += l_; \
} while (0)



/*
 * Ensure that the buffer has atleast 'n' bytes of space.
 */
#define fast_buf_ensure(b,l)    _fast_buf_expand(b,l)



/*
 * Append one byte to the fastbuf.
 */
#define fast_buf_append(b,c)    do { \
    fast_buf * b_ = (b); \
    _fast_buf_expand(b_,1);      \
    b_->buf[b_->size++] = (c);  \
} while (0)



/*
 * Reset a fast buf to initial conditions.
 */
#define fast_buf_reset(b)       do { \
    (b)->size = 0;\
} while (0)



/*
 * Advance a fast buf ptr by 'n' bytes.
 */
#define fast_buf_advance(b,n)   do { \
    (b)->size += (n); \
} while (0)




/*
 * Return a pointer to the beginning of the fast buffer.
 */
#define fast_buf_ptr(b)     (b)->buf


/*
 * Return a pointer to the end of the fast buffer.
 */
#define fast_buf_endptr(b)  ((b)->buf+(b)->size)


/*
 * Return available capacity in the fast buffer.
 */
#define fast_buf_capacity(b)  (b)->capacity
#define fast_buf_avail(b)     (b)->capacity


/*
 * Return the size of the fast buffer.
 */
#define fast_buf_size(b)    (b)->size



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __FAST_BUF_H__ */

/* EOF */
