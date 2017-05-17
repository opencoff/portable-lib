/* :vi:ts=4:sw=4:
 *
 * memmgr.h - simple memory manager interface in "C"
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
 *
 * Memory manager interface to suit a variety of allocators and
 * de-allocators.
 *
 * Usage:
 *   1) Initialize a variable of type memmgr with the correct
 *      alloc and free functions.
 *   2) Call the macros memmgr_alloc() and memmgr_free() to call
 *      the desired alloc/free functions.
 *
 */

#ifndef __MEMMGR_H__
#define __MEMMGR_H__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>

typedef void * Alloc_f (void *, size_t size);
typedef void   Free_f (void *, void *);

struct memmgr
{
    void* (*alloc)(void* ctx, size_t n);
    void  (*free)(void* ctx, void* ptr);
    void * context;
};
typedef struct memmgr memmgr;

extern memmgr * memmgr_init(memmgr *, Alloc_f *, Free_f *, void * context);
extern memmgr * memmgr_init_default(memmgr *);
extern memmgr * memmgr_init_xmalloc(memmgr *);


#define memmgr_alloc(m,s)   (*(m)->alloc)((m)->context, s)
#define memmgr_free(m,p)    (*(m)->free)((m)->context, p)

#define memmgr_valid_p(m)   ((m)->alloc)



/* CTOR: Make a memory manager using malloc/free */
#define malloc_memmgr(m)    memmgr_init_default(m)

/*
 * This memory allocator cannot fail. i.e., it will exit with error
 * message on allocation failure.
 */
#define xmalloc_memmgr(m)   memmgr_init_xmalloc(m)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __MEMMGR_H__ */

/* EOF */
