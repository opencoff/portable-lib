/* vim: ts=4:sw=4:expandtab:tw=72:
 * 
 * mempool.c - Small, scalable fixed size object state.
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



#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "utils/mempool.h"
#include "fast/list.h"

/*
 *           IMPLEMENTATION NOTES
 *           ====================
 *
 * Terminology:
 * ------------
 *    BLKSIZE   - size of each fixed sized object
 *    CHUNKSIZE - granularity of allocation made from lower-layer
 *                state (MIN_ALLOC_UNITS)
 *    block     - a fixed sized object sized region of memory (roughly
 *                equal BLKSIZE)
 *    chunk     - region of memory allocated from lower-layer
 *                state; roughly CHUNKSIZE big.
 *
 *  Note: When we say 'lower-layer state' we refer to a
 *  "primitive" state - typically one supplied by an underlying
 *  OS - e.g., malloc/free.
 *
 * General idea:
 * -------------
 *  - Allocations from the lower layer are managed via a "traits"
 *    struct with function pointers for alloc/free etc.
 *
 *  - Make large allocations from the lower-layer. Each allocation can
 *    satifsy multiple BLKSIZE requests.
 *
 *  - Keep track of such large allocations in a singly linked list.
 *    We need the starting pointers of each such chunk when we are
 *    deleting the fixed sized object state.
 *
 *  - Once fixed sized objects are returned back to the state, we
 *    keep the returned objects in a second "MRU" linked list.
 *
 *  - Newly created chunks are inserted into the head of the link list.
 *    The reasoning is that we only allocate a new chunk when our MRU
 *    list is empty i.e., even the last chunk we allocated is empty.
 *    Thus, it makes good sense to keep the "hot chunk" at the head
 *    of the list to facilitate fast allocations.
 *
 *  - Requests for new fixed sized objects are satisfied in the following
 *    order:
 *      o MRU list
 *      o latest allocated chunk ("hot chunk")
 *      o newly allocated chunk
 *
 *  - if an state instance is initialized with a fixed amount of
 *    memory, no new requests will be made to the underlying low-level state.
 *
 * Knobs to tune
 * -------------
 *   MEMPOOL_DEBUG  - this macro controls debug features.
 *                    Additional error checking is done on pointers
 *                    that are returned back to the state.
 *
 *   NDEBUG         - This ANSI/ISO macro is not used in this
 *                    state. This controls how assert()
 *                    behaves. I generally like to leave assert()
 *                    in production code - it is meant to be a
 *                    life-saver, a sort of emergency brake.
 *                    But, if you choose to be reckless, _define_
 *                    this macro at the beginning of this file and
 *                    assert() will be compiled out.
 */



/* Controls additional sanity checks. */
#if 0
#define MEMPOOL_DEBUG  1
#endif



/* useful typedefs */
typedef unsigned int       uint_t;
typedef unsigned char      uchar_t;
#if defined(__PTRDIFF_TYPE__)
typedef unsigned __PTRDIFF_TYPE__ ptrval_t;
#elif defined(__LP64__)
typedef unsigned long long   ptrval_t;
#else
typedef unsigned long ptrval_t;
#endif


/* handy macros to cast away our troubles */
#define _PTRVAL(x)  ((ptrval_t)(x))
#define pUCHAR(x)   ((unsigned char *)(x))



/*
 * Memory allocated from OS is held in one or more instances of this
 * struct.
 */
struct memchunk
{
    /* links all allocated chunks */
    struct memchunk * next;

    /*
     * pointer to free area from whence we can carve out
     * new blocks
     */
    uchar_t * free_area;

    /* points to the end of this hunk */
    uchar_t * end;
};
typedef struct memchunk memchunk;


/*
 * Each fixed sized object when it is returned to the state is linked
 * into a MRU list. This struct provides the minimum infrastructure
 * for such an MRU list.
 *
 * Incidentally, the size of this struct is the minimum size of a
 * fixed-size block.
 */
struct mru_node
{
    DL_ENTRY(mru_node)  link;
    //struct mru_node * next;
};
typedef struct mru_node mru_node;

DL_HEAD_TYPEDEF(mru_head_type, mru_node);

/*
 * State of the fixed-size state. Must be treated as opaque by
 * callers.
 */
struct mempool
{
    /* Head of MRU block list */
    mru_head_type         mru_head;

    /* size of each block in this state */
    uint64_t block_size;

    /* Maximum number of blocks (if constrained so)  */
    uint64_t max_blocks;

    /* minimum number of units to allocate at a time */
    uint64_t min_units;

    /* list of OS allocated chunks */
    struct memchunk    * chunks;

    /* OS Traits */
    struct memmgr traits;
};
typedef struct mempool state;




/*
 * This allocator and the system allocator must have identical
 * behavior when used to store various types of objects. Thus, the
 * alignment of the allocated blocks becomes very important. i.e.,
 * we need the allocated blocks to start at perfect "system
 * alignment" boundaries.
 *
 * Since alignment of pointers is intensely platform dependent, we
 * use a union containing all the types that are possible on this
 * system. And, we use the size of this union to determine the
 * system-dependent alignment necessary.
 */
union min_alignment
{
    unsigned int iv;
    void * pv;
    int  * pi;
    unsigned long * pl;
    double dv;
    unsigned long lv;
    uint64_t v2;
    unsigned long long v;
};
typedef union min_alignment min_alignment;

#define MINALIGNMENT    (sizeof(min_alignment))

/*
 * Align 'p' to the minimum required alignment and return it as an
 * object of type 'typ'
 */
#define _Align(typ, p) (typ)(_Isaligned_p(p) ? _PTRVAL(p) : ((_PTRVAL(p)+MINALIGNMENT-1) & ~(MINALIGNMENT-1)))
#define _Isaligned_p(p)  ((_PTRVAL(p) & (MINALIGNMENT-1)) == _PTRVAL(p))

/*
 * The smallest block size should allow insertion into the MRU list.
 * Thus, the minimum block size is sizeof(mru_block).
 */
#define MIN_OBJ_SIZE    (sizeof(mru_node))


/*
 * Minimum number of blocks to allocate from the OS in one shot
 */
#ifndef MEMPOOL_MIN_ALLOC_UNITS
#define MEMPOOL_MIN_ALLOC_UNITS     4096
#endif





static void*
__os_alloc(void* unused, size_t n)
{
    (void)unused;
    return calloc(1, n);
}

void
__os_free(void* unused, void* a)
{
    (void)unused;
    free(a);
}



/*
 * Helper functions to make life easier.
 */
static void*
__alloc(state* a, size_t n)
{
    const memmgr* tr = &a->traits;

    return tr->alloc(tr->context, n);
}


static void
__dummy_free(void* unused, void* a)
{
    (void)unused;
    (void)a;
}


/*
 * Allocate a new chunk from the OS
 */
static int
new_chunk(state * a)
{
    uint64_t chunk_size = (uint64_t)a->block_size * (uint64_t)a->min_units;
    uint64_t alloc_size = sizeof(memchunk) + chunk_size + MINALIGNMENT;
    memchunk   * ch;
    uchar_t * ptr;

    /* Guard against arithmetic overflow */
    assert(chunk_size > a->block_size && chunk_size > a->min_units);
    assert(alloc_size > chunk_size);

    ch = (memchunk *)__alloc(a, alloc_size);
    if (!ch)
        return 0;

    /* add this to list of chunks we already have */
    ch->next  = a->chunks;
    a->chunks = ch;


    /*
     * Setup the pointers within this chunk so that all alignment
     * constraints are met; thus making it easy for allocating
     * blocks when required.
     *
     * When we are done, all memory between ch->free_area and
     * ch->end will be an exact multiple of a->block_size and
     * properly aligned.
     */

    ptr           = pUCHAR(ch) + sizeof *ch;    /* start of free area */
    ch->free_area = _Align(uchar_t *, ptr);     /* aligned start of blocks */
    ch->end       = ch->free_area + chunk_size; /* end of chunk memory */

    return 1;
}



/*
 * Try to allocate from the most recent allocated chunk and return a
 * pointer to a block.  Return 0 if not possible.
 */
static void *
alloc_from_chunk(state * a)
{
    memchunk * ch = a->chunks; /* latest chunk */
    void  * p  = 0;

    if ( ch->free_area < ch->end )
    {
        p              = ch->free_area;
        ch->free_area += a->block_size;
    }

    return p;
}




/*
 * Debug scaffolding - verify that a pointer that is freed actually
 * came from this state.
 */
#ifdef MEMPOOL_DEBUG

static int
_valid_blk_p(state *a, uchar_t * ptr)
{
    memchunk * ch = a->chunks;

    while (ch)
    {
        uchar_t * chunk_start = pUCHAR(ch) + sizeof *ch;

        if (chunk_start <= ptr && ptr < ch->end)
            return 1;

        ch = ch->next;
    }

    return 0;
}

/* Clear any memory that is returned back to the state */
#define clear_memory(a, p)  memset(p, 0x77, a->block_size)
#define fill_memory(a, p)   (p) ? memset((p), 0xaa, a->block_size) : 0

#else

#define _valid_blk_p(a,b)   1
#define clear_memory(a,p)   do { } while (0)
#define fill_memory(a,p)    p


#endif /* MEMPOOL_DEBUG */


/*
 * -- External functions --
 */




/*
 * Instantiate a new fixed sized obj state.
 */
int
mempool_new(state ** p_a, const memmgr* tr, uint_t block_size,
                  uint_t max, uint_t min_alloc_units)
{
    state* a;

    if ( !p_a )
        return -EINVAL;

    if (!tr) {
        static const memmgr os_traits = {
            .alloc   = __os_alloc,
            .free    = __os_free,
            .context = 0
        };
        tr = &os_traits;
    }

    if  (!(tr->alloc && tr->free))
        return -EINVAL;

    a = (state *)tr->alloc(tr->context, sizeof *a);
    if (!a)
        return -ENOMEM;

    memset(a, 0, sizeof *a);


    /*
     * Make sure block_size has minimum qualifications.
     */
    if (block_size < MIN_OBJ_SIZE)
        block_size = MIN_OBJ_SIZE;


    block_size = _Align(uint_t, block_size);


    /*
     * If max is specified, we clamp min_alloc_units to that value.
     * Otherwise, we use a default (if required).
     */
    if (max)                    min_alloc_units = max;
    else if (!min_alloc_units)  min_alloc_units = MEMPOOL_MIN_ALLOC_UNITS;

    a->block_size = block_size;
    a->max_blocks = max;
    a->min_units  = min_alloc_units;
    a->traits     = *tr;


    if (!new_chunk(a))
        return -ENOMEM;

    *p_a = a;
    return 0;
}




int
mempool_new_from_mem(state ** p_a, uint_t block_size,
                           void* pool, uint_t poolsize)
{
    state* a;
    memchunk * ch;
    uchar_t* ptr,
           * end;
    unsigned int nblocks;

    if (!(p_a && pool && poolsize))
        return -EINVAL;


    /*
     * the pool must be big enough to hold a chunk and at least one
     * aligned block.
     */
    if (block_size < MIN_OBJ_SIZE)
        block_size = MIN_OBJ_SIZE;


    /*
     * And make it conform to minimum alignment requirements as well
     */
    block_size = _Align(uint_t, block_size);

    /*
     * Make sure pool is aligned to begin with. Most of the time, it
     * will be.
     */
    ptr       = _Align(uchar_t *, pool);
    a         = (state *)ptr;
    ptr      += sizeof *a;
    poolsize -= sizeof *a;
    ptr       = _Align(uchar_t*, ptr);
    poolsize -= (ptr - pUCHAR(pool));
    pool      = ptr;

    if (poolsize < (block_size + MINALIGNMENT + sizeof *ch))
        return -ENOMEM;

    memset(a, 0, sizeof *a);

    ch = (memchunk*)pool;


    /*
     * Setup the pointers within this chunk so that all alignment
     * constraints are met; thus making it easy for allocating
     * blocks when required.
     *
     * When we are done, all memory between ch->free_area and
     * ch->end will be an exact multiple of a->block_size;
     */


    ptr            = pUCHAR(ch) + sizeof *ch;    /* start of free area */
    end            = pUCHAR(ch) + poolsize;      /* end of the pool */
    ch->free_area  = _Align(uchar_t *, ptr);     /* aligned start of blocks */
    nblocks        = (end - ch->free_area) / block_size;
    ch->end        = ch->free_area + (nblocks * block_size);

    ch->next       = 0;
    a->chunks      = ch;
    a->max_blocks  = a->min_units = nblocks;
    a->block_size  = block_size;
    a->traits.free = __dummy_free;

    *p_a = a;
    return 0;
}



/*
 * Delete an state and release all memory back to the OS.
 */
void
mempool_delete(state* a)
{
    if (!a)
        goto end;

    const memmgr* tr = &a->traits;
    memchunk * ch    = a->chunks;

    while (ch) {
        memchunk * next = ch->next;

        (*tr->free)(tr->context, ch);
        ch = next;
    }

    (*tr->free)(tr->context, a);

end:
    return;
}



/*
 * Allocate a block from the state.
 */
void *
mempool_alloc(state* a)
{
    mru_node * blk = 0;
    void * ptr = 0;

    assert(a);

    blk = DL_REMOVE_TAIL(&a->mru_head, link);
    if (blk) {
        ptr         = blk;
        goto _end;
    }


    /*
     * MRU list is empty. We now try the most recent hunk.
     * If this state is clamped for max number of units, we
     * won't try allocating more chunks.
     */

    ptr = alloc_from_chunk(a);
    if (ptr || a->max_blocks)
        goto _end;


    /*
     * Last try: allocate a new chunk and fetch a block from it.
     */
    if (new_chunk(a))
        ptr = alloc_from_chunk(a);

_end:
    //printf("state-%p: alloc() => %p\n", a, ptr);
    return ptr ? fill_memory(a, ptr) : 0;
}



/*
 * Return a fixed sized obj back to the state.
 */
void
mempool_free(state* a, void * ptr)
{
    mru_node * blk = (mru_node *)ptr;

    assert(a);
    assert(_valid_blk_p(a, pUCHAR(ptr)));

    clear_memory(a, ptr);

    DL_INSERT_HEAD(&a->mru_head, blk, link);
}



/*
 * Return the block size of this state.
 */
uint_t
mempool_block_size(state* a)
{
    assert(a);

    return a->block_size;
}

uint_t
mempool_total_blocks(state* a)
{
    assert(a);

    return a->max_blocks;
}


/*
 * Stack one memory manager on top of an existing one.
 */


static void*
_stacked_alloc(void* x, size_t n)
{
    (void)n;
    return mempool_alloc((state *)x);
}

static void
_stacked_free(void* x, void* ptr)
{
    return mempool_free((state *)x, ptr);
}



int
mempool_new_memmgr(struct memmgr* out, const memmgr* in,
                    uint_t blksize, uint_t maxblks, uint_t minau)
{
    state* a = 0;
    int r = mempool_new(&a, in, blksize, maxblks, minau);
    if (r != 0)
        return r;


    out->alloc   = _stacked_alloc;
    out->free    = _stacked_free;
    out->context = a;
    return 0;
}

/* EOF */
