/* vim: expandtab:sw=4:ts=4:smartindent:tw=68:
 *
 * arena.c -- Object lifetime based memory manager.
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
 * Notes
 * =====
 *  An Arena manages memory of objects based on their lifetime. i.e.,
 *  individual objects are not deleted. The storage of all objects
 *  are "deleted" together. Thus, allocations are "fast" and
 *  de-allocations incur no-overhead! This will make for simpler
 *  algorithms.
 *
 *  e.g., symbol-table management: Symbol-tables of a particular
 *  scope can all be deleted in one shot.
 *
 *  It relies internally on malloc() and free() for its operations.
 *
 */

#include <errno.h>
#include <stdlib.h>
#include "utils/arena.h"
#include "utils/utils.h"
#include "fast/list.h"

#define DEFAULT_CHUNK_SIZE  (128 * 1024)

#if 0
#define DIAG(a) printf a
#else
#define DIAG(a)
#endif

struct arena_node
{
    SL_LINK(arena_node) link;

    /* Bytes available in this arena */
    unsigned int total;

    /* Start of available memory in thus chunk. */
    unsigned char * free;

    /* total bytes available */
    unsigned char* end;

    /* free memory starts beyond this block */
};
typedef struct arena_node arena_node;


struct arena
{
    SL_HEAD(node_head, arena_node) head;
    int chunk_size;
};
typedef struct arena arena;





/* Minimum alignment restriction on _this_ platform */
union align
{
    double dv ;
    int     iv;
    long    lv;
    long    *lp;
    char  *cp;
    char * (*cfp)(void);
} ;


#define SYS_ALIGNMENT   (sizeof(union align))



int
arena_new(arena_t* p_arena, int chunk_size)
{
    int retval = -ENOMEM;
    arena * a;

    if (!p_arena) return -EINVAL;

    if (chunk_size <= 0)
        chunk_size = DEFAULT_CHUNK_SIZE;
    
    a = NEWZ(arena);
    if (a) {
        retval = 0;
        SL_INIT(&a->head);
        a->chunk_size = chunk_size;
    }

    *p_arena = a;
    return retval;
}


/* compute available bytes in a node */
static inline int
__avail(arena_node* n)
{
    return n->end - n->free;
}


/*
 * Allocate memory from an arena
 */
void *
arena_alloc (arena_t a, int nbytes)
{
    arena_node* n;
    int chunk;
    void * mem = 0;

    if (!a) return 0;

    /* Account for system alignment */
    nbytes = _ALIGN_UP(nbytes, SYS_ALIGNMENT);

    /*
     * find an arena with the best size.
     */
    DIAG(("arena=%p; alloc-req=%d:\n", p, nbytes));
    SL_FOREACH(n, &a->head, link) {
        if (__avail(n) >= nbytes)
            goto _found;
    }

    /* allocate a new node; but, try to allocate a few more bytes
     * to satisfy atleast 16 such allocations.  */
    chunk = nbytes < a->chunk_size ? a->chunk_size : nbytes * 128;
    n     = (arena_node *) malloc(sizeof(arena_node) + chunk);
    if (!n) goto _end;

    n->total = chunk;
    n->free  = pUCHAR(n) + sizeof *n;
    n->end   = n->free + chunk;
    
    SL_INSERT_HEAD(&a->head, n, link);

_found:

    mem     = _ALIGN_UP(n->free, SYS_ALIGNMENT);
    n->free = pUCHAR(mem) + nbytes;

_end:
    return mem;
}




/* Delete all pools of memory associated with arena `a' */
void
arena_delete (arena_t a)
{
    arena_node* n;

    n = SL_FIRST(&a->head);
    while (n) {
        arena_node* next = SL_NEXT(n, link);

        DEL(n);
        n = next;
    }
    DEL(a);
}

/* EOF */
