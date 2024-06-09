/* vim: ts=4:sw=4:expandtab:tw=72:
 *
 * mempool.c - Small, scalable fixed size object allocator.
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
#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "utils/mempool.h"
#include "fast/list.h"

/*
 * IMPLEMENTATION NOTES
 * ====================
 *
 *  Note: When we say 'lower-layer allocator' we refer to a
 *  "primitive" allocator - typically one supplied by an underlying
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
 *  - Keep track of such large allocations in a singly linked list
 *    of "memchunks".  Each such memchunk holds a integral number
 *    of fixed-size objects.
 *
 *  - Newly created chunks are inserted into the head of the link list.
 *    The reasoning is that we only allocate a new chunk when our MRU
 *    list is empty i.e., even the last chunk we allocated is empty.
 *    Thus, it makes good sense to keep the "hot chunk" at the head
 *    of the list to facilitate fast allocations.
 *    head of the list.
 *  
 *  - Once fixed sized objects are returned back to the pool, we
 *    keep the returned objects in a second "MRU" linked list.
 *
 *  - Requests for new fixed sized objects are satisfied in the following
 *    order:
 *      * MRU list
 *      * latest allocated chunk ("hot chunk")
 *      * newly allocated chunk
 *
 *  - if a pool instance is initialized with a fixed amount of
 *    memory, no new requests will be made to the underlying
 *    low-level allocator.
 *
 * Knobs to tune
 * -------------
 *   MEMPOOL_DEBUG  - this macro controls debug features.
 *                    Additional error checking is done on pointers
 *                    that are returned back to the allocator.
 */



/* Controls additional sanity checks. */
#ifdef __MAKE_OPTIMIZE__
    #if __MAKE_OPTIMIZE__ == 1
        #define MEMPOOL_DEBUG  0
    #endif

#else
    #define MEMPOOL_DEBUG 1
#endif



/* useful typedefs */
typedef unsigned int       uint_t;
typedef unsigned char      uint8_t;
#if defined(__PTRDIFF_TYPE__)
typedef unsigned __PTRDIFF_TYPE__ ptrval_t;
#elif defined(__LP64__)
typedef uint64_t  ptrval_t;
#else
typedef uint32_t ptrval_t;
#endif


/* handy macros to cast away our troubles */
#define _PTRVAL(x)  ((ptrval_t)(x))
#define pUCHAR(x)   ((uint8_t *)(x))



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
    uint8_t * free_area;

    /* points to the end of this hunk */
    uint8_t * end;
};
typedef struct memchunk memchunk;


/*
 * Each fixed sized object when it is returned to the pool is linked
 * into a MRU list. This struct provides the minimum infrastructure
 * for such an MRU list.
 *
 * Incidentally, the size of this struct is the minimum size of a
 * fixed-size block.
 */
struct mru_node
{
    DL_ENTRY(mru_node)  link;
};
typedef struct mru_node mru_node;


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




/*
 * Traits for allocating directly from the OS/Libc
 */
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


static inline void
__dummy_free(void* unused, void* a)
{
    (void)unused;
    (void)a;
}


/*
 * Allocate a new chunk from the OS
 */
static int
new_chunk(mempool* a)
{
    uint64_t chunk_size = (uint64_t)a->block_size * (uint64_t)a->min_units;
    uint64_t alloc_size = sizeof(memchunk) + chunk_size + MINALIGNMENT;

    /* Guard against arithmetic overflow */
    assert(chunk_size > a->block_size && chunk_size > a->min_units);
    assert(alloc_size > chunk_size);

    memchunk *ch = (memchunk *) (*a->traits.alloc)(a->traits.context, alloc_size);
    if (!ch) return 0;

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
    uint8_t *ptr  = pUCHAR(ch) + sizeof *ch;    /* start of free area */
    ch->free_area = _Align(uint8_t *, ptr);     /* aligned start of blocks */
    ch->end       = ch->free_area + chunk_size; /* end of chunk memory */

    return 1;
}



/*
 * Try to allocate from the most recent allocated chunk and return a
 * pointer to a block.  Return 0 if not possible.
 */
static void *
alloc_from_chunk(mempool* a)
{
    memchunk *ch = a->chunks; /* latest chunk */
    void  *p     = 0;

    if (ch->free_area < ch->end) {
        p  = ch->free_area;
        ch->free_area += a->block_size;
    }

    return p;
}




/*
 * Debug scaffolding - verify that a pointer that is freed actually
 * came from this state.
 */
#if MEMPOOL_DEBUG > 0

static int
_valid_blk_p(mempool* a, uint8_t * ptr)
{
    memchunk * ch = a->chunks;

    while (ch) {
        uint8_t *chunk_start = pUCHAR(ch) + sizeof *ch;

        if (chunk_start <= ptr && ptr < ch->end)
            return 1;

        ch = ch->next;
    }

    return 0;
}

/* Clear any memory that is returned back to the mempool */
#define clear_memory(a, p)  memset(p, 0x77, a->block_size)
#define fill_memory(a, p)   (p) ? memset((p), 0xaa, a->block_size) : 0

#else

#define _valid_blk_p(a,b)   1
#define clear_memory(a,p)   do { } while (0)
#define fill_memory(a,p)    p


#endif /* MEMPOOL_DEBUG */


static int
__real_init(mempool *a, const memmgr *tr, uint_t block_size, uint_t max, uint_t min_alloc_units)
{
    memset(a, 0, sizeof *a);

    /*
     * Make sure block_size has minimum qualifications.
     */
    if (block_size < MIN_OBJ_SIZE) block_size = MIN_OBJ_SIZE;


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


    if (!new_chunk(a)) return -ENOMEM;

    return 0;
}


/*
 * Initialize mempool
 */
int
mempool_init(mempool* a, const memmgr* tr, uint_t block_size,
                  uint_t max, uint_t min_alloc_units)
{
    if ( !a ) return -EINVAL;
    if (!tr) {
        static const memmgr os_traits = {
            .alloc   = __os_alloc,
            .free    = __os_free,
            .context = 0
        };
        tr = &os_traits;
    } else {
        if  (!(tr->alloc && tr->free)) return -EINVAL;
    }

    return __real_init(a, tr, block_size, max, min_alloc_units);
}


/*
 * Create & initialize a new fixed sized obj pool.
 */
int
mempool_new(mempool** p_a, const memmgr* tr, uint_t block_size,
                  uint_t max, uint_t min_alloc_units)
{
    mempool* a = 0;

    if ( !p_a ) return -EINVAL;

    if (!tr) {
        static const memmgr os_traits = {
            .alloc   = __os_alloc,
            .free    = __os_free,
            .context = 0
        };
        tr = &os_traits;
    } else  {
        if  (!(tr->alloc && tr->free)) return -EINVAL;
    }

    a = (mempool* )tr->alloc(tr->context, sizeof *a);
    if (!a) return -ENOMEM;

    *p_a = a;
    return __real_init(a, tr, block_size, max, min_alloc_units);
}


/*
 * Iniitialize a new mempool to work out of a fixed-size zone.
 */
int
mempool_init_from_mem(mempool* a, uint_t block_size,
                           void* pool, uint_t poolsz)
{
    memchunk *ch = 0;
    uint8_t* ptr = 0,
           * end = 0;
    unsigned int nblocks = 0;
    uint64_t poolsize    = poolsz;

    if (!(a && pool && poolsize)) return -EINVAL;

    memset(a, 0, sizeof *a);

    if (block_size < MIN_OBJ_SIZE) block_size = MIN_OBJ_SIZE;

    block_size = _Align(uint_t, block_size);


    /*
     * Make sure pool is aligned to begin with. Most of the time, it
     * will be.
     */
    ptr       = _Align(uint8_t *, pool);
    poolsize -= (ptr - pUCHAR(pool));

    /*
     * the pool must be big enough to hold a chunk and at least one
     * aligned block.
     */
    if (poolsize < (block_size + MINALIGNMENT + sizeof *ch)) return -ENOMEM;

    ch = (memchunk*)ptr;
    poolsize -= sizeof *ch;
    ptr      += sizeof *ch;     // start of allocation area

    /*
     * Setup the pointers within this chunk so that all alignment
     * constraints are met; thus making it easy for allocating
     * blocks when required.
     *
     * When we are done, all memory between ch->free_area and
     * ch->end will be an exact multiple of a->block_size;
     */
    end            = ptr + poolsize;          // end of allocation area
    ch->free_area  = _Align(uint8_t *, ptr);  // aligned start of "n" blocks

    nblocks        = (end - ch->free_area) / block_size;
    ch->end        = ch->free_area + (nblocks * block_size);

    assert(ch->end <= end);

    ch->next       = 0;
    a->chunks      = ch;
    a->max_blocks  = a->min_units = nblocks;
    a->block_size  = block_size;
    a->traits.free = __dummy_free; // to make mempool_delete() easier

    return 0;
}


/*
 * Create & iniitialize a new mempool to work out of a fixed-size zone.
 */
int
mempool_new_from_mem(mempool** p_a, uint_t block_size,
                           void* pool, uint_t poolsize)
{
    mempool* a   = 0;
    uint8_t* ptr = 0;

    if (!(p_a && pool && poolsize)) return -EINVAL;

    /*
     * Make sure pool is aligned to begin with. Most of the time, it
     * will be.
     */
    ptr       = _Align(uint8_t *, pool);
    poolsize -= ptr - pUCHAR(pool);
    a         = (mempool* )ptr;
    ptr      += sizeof *a;
    poolsize -= sizeof *a;

    *p_a = a;
    return mempool_init_from_mem(a, block_size, ptr, poolsize);
}


/*
 * Un-initialize a mempool.
 */
void
mempool_fini(mempool *a)
{
    if (!a) return;

    const memmgr* tr = &a->traits;
    memchunk * ch    = a->chunks;

    while (ch) {
        memchunk * next = ch->next;

        (*tr->free)(tr->context, ch);
        ch = next;
    }
    memset(a, 0, sizeof *a);
}


/*
 * Delete an state and release all memory back to the OS.
 */
void
mempool_delete(mempool* a)
{
    if (a) {
        const memmgr tr = a->traits;

        mempool_fini(a);
        (*tr.free)(tr.context, a);
    }
}



/*
 * Allocate a block from the state.
 */
void *
mempool_alloc(mempool* a)
{
    mru_node * blk = 0;
    void * ptr     = 0;

    assert(a);

    blk = DL_REMOVE_TAIL(&a->mru_head, link);
    if (blk) {
        ptr = blk;
        goto _end;
    }


    /*
     * MRU list is empty. We now try the most recent memchunk.
     * If this state is clamped for max number of units, we
     * won't try allocating more chunks.
     */

    ptr = alloc_from_chunk(a);
    if (ptr || a->max_blocks) goto _end;


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
mempool_free(mempool* a, void * ptr)
{
    mru_node * blk = (mru_node *)ptr;

    assert(a);
    assert(_valid_blk_p(a, pUCHAR(ptr)));
    clear_memory(a, ptr);

    DL_INSERT_HEAD(&a->mru_head, blk, link);
}



/*
 * Return the block size of this mempool.
 */
uint_t
mempool_block_size(mempool* a)
{
    assert(a);

    return a->block_size;
}

/*
 * Return the max number of blocks for this mempool.
 * 0 => unlimited.
 */
uint_t
mempool_total_blocks(mempool* a)
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
    return mempool_alloc((mempool* )x);
}

static void
_stacked_free(void* x, void* ptr)
{
    return mempool_free((mempool* )x, ptr);
}



struct memmgr*
mempool_make_memmgr(struct memmgr* out, struct mempool *a)
{
    out->alloc   = _stacked_alloc;
    out->free    = _stacked_free;
    out->context = a;
    return out;
}

/* EOF */
