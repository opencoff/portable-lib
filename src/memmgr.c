/* :vi:ts=4:sw=4:
 *
 * memmgr.c - memory manager interface
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

#include "utils/utils.h"
#include "utils/memmgr.h"
#include "error.h"
#include <assert.h>
#include <stdlib.h>
#include <errno.h>


void
__dummy_memmgr_free(void * context, void * ptr)
{
    USEARG(context);
    USEARG(ptr);
    return;
}

memmgr *
memmgr_init(memmgr * m, Alloc_f * alloc, Free_f * freefp, void * context)
{
    if ( m) {
        assert(alloc);

        if (!freefp)
            freefp = __dummy_memmgr_free;

        m->alloc   = alloc;
        m->free    = freefp;
        m->context = context;
    }
    return m;
}


/*
 * Standard malloc/free interface
 */
static void *
__malloc_memmgr_alloc(void * _context, size_t size)
{
    USEARG(_context);
    return malloc(size);
}


static void
__malloc_memmgr_free(void * _context, void * ptr)
{
    USEARG(_context);
    if (ptr)
        free(ptr);
}


/* ctor to make a default memory manager. */
memmgr*
memmgr_init_default(memmgr* m)
{
    if (m) {
        m->alloc   = __malloc_memmgr_alloc;
        m->free    = __malloc_memmgr_free;
        m->context = 0;
    }

    return m;
}


/*
 * Unfailable allocator
 */

static void*
__xmalloc_alloc(void* ctx, size_t sz)
{
    USEARG(ctx);
    void* p = malloc(sz);
    if (!p)
        error(1, ENOMEM, "Out of memory while allocating %zu", sz);

    return p;
}


memmgr*
memmgr_init_xmalloc(memmgr* m)
{
    if (m) {
        m->alloc   = __xmalloc_alloc;
        m->free    = __malloc_memmgr_free;
        m->context = 0;
    }
    return m;
}

/* EOF */
