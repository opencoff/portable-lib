/* vim: ts=4:sw=4:expandtab:tw=72:
 *
 * mempool.h - Fast, scalable fixed-size objects' allocator.
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


/*
 * Introduction to the Fixed Size Allocator:
 *
 *   Any time memory is requested from the heap (free-store), there
 *   is an overhead associated with each request that is satisfied.
 *   Depending on the platform and heap manager, the overhead varies
 *   between 4 and 16 bytes. For small, fixed sized objects, this
 *   overhead can lead to incredible wastage of the heap - not to
 *   mention slowing down your program.
 *
 *   A fixed size allocator is meant to amortize the heap
 *   allocation overhead over several hundred (thousand) small
 *   object allocations. The central idea is very simple:
 *
 *     Allocate contiguous chunks of fixed sized objects from the
 *     OS and carve out one at a time (without using any intermediate
 *     overhead). If the number of contiguous chunks are large
 *     (several thousand), then the 4-16 byte heap overhead is
 *     amortized over those many fixed sizes.
 *
 *   This is a O(1) implementation of such a fixed size allocator.
 *   It takes care of the tedium of bookeeping associated with
 *   successive allocation, free etc.
 *
 * Multi-threaded Issues
 * =====================
 * This allocator is fully-reentrant. i.e., it does NOT maintain any
 * local information. Any thing it needs is passed as one of the
 * arguments to the API functions.
 *
 * However, if a _single_ instance of the allocator is used in a
 * multi-threaded system, the users must use a lock/mutex around
 * calls to the allocator.  e.g.,
 *
 *     typedef mempool<my_class> my_alloc;
 *
 *     // global instance of the allocator for my_class
 *     my_alloc Allocator;
 *
 *     void * alloc()
 *     {
 *         void * x;
 *         lock();
 *            x = Allocator.alloc();
 *         unlock();
 *         return x;
 *     }
 */

#ifndef __MEMPOOL_H_1129300402_79845__
#define __MEMPOOL_H_1129300402_79845__ 1

#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#include "utils/memmgr.h"
#include "fast/list.h"

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mru_node;

DL_HEAD_TYPEDEF(mru_head_type, mru_node);
/*
 * mempool state;
 */
struct mempool
{
    /* Head of MRU block list */
    struct mru_head_type  mru_head;

    /* size of each block in this pool */
    uint64_t block_size;

    /* Maximum number of blocks (if constrained so)  */
    uint64_t max_blocks;

    /* minimum number of units to allocate at a time */
    uint64_t min_units;

    /* list of OS allocated chunks */
    struct memchunk  *chunks;

    /* OS Traits */
    struct memmgr traits;
};
typedef struct mempool mempool;


/** Create a new small obj allocator:
 *      mempool_new() creates & initializes a new mempool instance
 *      mempool_init() initializes a mempool instance
 *
 *  This function creates a new fixed size allocator to
 *  efficiently allocate objects of 'blksize' bytes.
 *
 *
 *  @param st       Allocator state that must be initialized.
 *                  Callers must treat this as an opaque handle
 *                  and not peek/modify the internal state of
 *                  the allocator.
 *  @param traits   OS Traits for allocating memory. If this is NULL,
 *                  then if USE_MALLOC is defined in the corresponding
 *                  .c file, POSIX/Win32 malloc()/free() will be used
 *                  for memory allocation.. If USE_MALLOC is undefined,
 *                  then the caller *must* supply a valid allocator.
 *  @param blksize  Size of each fixed sized block.
 *  @param maxblks  If non-zero, clamps this allocator instance to
 *                  this many fixed size blocks. If this is zero, then
 *                  the allocator will allocate as many blocks of
 *                  memory from the OS as required to satisfy new
 *                  requests.
 *  @param min_alloc_units If non-zero, sets the allocation
 *                         granularity when requests are made from
 *                         the OS. This is specified in terms of
 *                         number of 'blocks' of 'blksize'.
 *                         If this is set to zero, then the library
 *                         will use an internal default.
 *
 *  @return  0      on success
 *  @return -EINVAL if 'st' is NULL or 'traits' is NULL.
 *  @return -ENOMEM if there is no more memory
 */
int mempool_new(struct mempool** p_st,
                const memmgr* traits,
                unsigned int blksize, unsigned int maxblks,
                unsigned int min_alloc_units);

int mempool_init(struct mempool* st,
                const memmgr* traits,
                unsigned int blksize, unsigned int maxblks,
                unsigned int min_alloc_units);


/** Create & initialize a new small obj allocator to work out of a
 * preallocated chunk of raw memory. In this mode, the allocator
 * makes ZERO calls to the underlying OS for additional memory.
 *
 *      mempool_new_from_mem() creates & initializes a new
 *          fixed-size zone.
 *      mempool_init_from_mem() initializes an instance of mempool
 *          to operate out of a fixed size zone.
 *
 *
 *  @param st       Allocator state that must be initialized.
 *                  Callers must treat this as an opaque handle
 *                  and not peek/modify the internal state of
 *                  the allocator.
 *  @param blksize  Size of each fixed sized block.
 *  @param mem      Pointer to memory block which will be carved into
 *                  blksize'd chunks
 *  @param memsize  Size of the memory block
 *
 *  @return  0      on success
 *  @return -EINVAL if 'st' is NULL or 'pool' is NULL.
 *  @return -ENOMEM if the pool is too small
 */
int mempool_new_from_mem(struct mempool** st,
                         unsigned int blksize, void* mem, unsigned int memsize);

int mempool_init_from_mem(struct mempool* st,
                         unsigned int blksize, void* mem, unsigned int memsize);


/** Delete/Finalize a fixed size allocator.
 *      mempool_delete() cleans up the pool _and_ deletes the state memory
 *      mempool_fini() cleans up the pool
 *
 *  This function deletes a fixed size allocator created by the
 *  previous function.
 *
 *  @param a Handle to the allocator
 *
 *  @see  mempool_new
 */
void mempool_delete(struct mempool* a);
void mempool_fini(struct mempool*);



/** Allocate one fixed size block from the allocator.
 *
 *  @param a Handle to the allocator
 *
 *  @return If successful, pointer to a suitably aligned
 *          fixed size memory location.
 *
 *  @return 0 if no memory is available or if the allocator was
 *            clamped.
 */
void * mempool_alloc(struct mempool*);



/** Return a fixed size block back to the allocator.
 *
 *  @param a    Handle to the allocator
 *  @param ptr  Pointer to fixed size that was previously
 *              allocated by calling mempool_alloc().
 */
void mempool_free(struct mempool* a, void * ptr);


/** Return the block size (size of each small obj) maintained by
 *  this instance of the allocator.
 *
 * @note The allocator may round-up the size of the fixed size to
 *       meet alignment and other implementation constraints. Thus,
 *       the return value of this function may not be the same as
 *       the value given to mempool_new().
 *
 * @param a Handle to the allocator
 *
 * @return Actual block size (size of each small obj) used by the allocator
 */
unsigned int mempool_block_size(struct mempool* a);


/** Return the number of fixed sized blocks available in the allocator.
 *
 * @note This function is useful for those situations where the
 *       allocator is bounded (with max number of blocks).
 *
 * @param a Handle to the allocator
 *
 * @return > 0  Max available blocks
 * @return == 0 If an infinite number of blocks are available
 */
unsigned int mempool_total_blocks(struct mempool* a);


/** Turn the mempool into a memgr interface.
 *  Basically, given a lower level of allocator, stack the mempool
 *  interface on top of it.
 *
 *  Passing 0 to the underlying allocator (traits) will enable
 *  mempool to use malloc()/free().
 */
struct memmgr* mempool_make_memmgr(struct memmgr* out, struct mempool* a);


#ifdef __cplusplus
} /* end of "C" linkage */



#include <new>

namespace putils {

// Mempool creates a fixed-size allocator to hold objects of type 'T'.
template <typename T> class Mempool
{
public:
    Mempool(int max = 0, int minunits=0, const memmgr* tr=0)
    {
        int e = mempool_init(&m_pool, tr, sizeof(T), max, minunits);
        if (e < 0) throw std::bad_alloc();
    }

    Mempool(void* mem, unsigned int memsize)
    {
        int e = mempool_init_from_mem(&m_pool, sizeof(T), mem, memsize);
        if (e < 0) throw std::bad_alloc();
    }

    virtual ~Mempool()
    {
        mempool_fini(&m_pool);
    }

    // These ctors and operators are problematic for memory allocators.
    // It's not clear what should be the semantics when we do:
    //   Mempool<obj> a;
    //   Mempool<obj> b {a};
    //
    // Should we have two copies of the underlying pool? Create a
    // new pool from the size/limits of 'a'?

    Mempool(const Mempool&) = delete;
    Mempool(Mempool&&) = delete;

    Mempool& operator=(const Mempool&) = delete;
    Mempool& operator=(Mempool&&) = delete;

    // Allocate one fixed size block and construct using the right args
    template <class... Args> T* Alloc(Args&&... a)
    {
        void *r = mempool_alloc(&m_pool);
        if (!r) throw std::bad_alloc();

        return new(r) T(std::forward<Args>(a)...);
    }

    // Deallocate a fixed size block
    void Free(T * ptr)
    {
        // Call explicitly
        ptr->~T();
        mempool_free(&m_pool, ptr);
    }

    // Return the actual block size used by this allocator.
    // Note: The actual block size may be larger than the size
    // specified when the allocator was created.
    unsigned int Blocksize() { return mempool_block_size(&m_pool); }

    // Return the max blocks available with this allocator.
    // 0 => infinite number of blocks
    unsigned int Totalblocks() { return mempool_total_blocks(&m_pool);}

private:
    mempool m_pool;
};



} /* namespace putils */

#endif /* __cplusplus */

#endif /* ! __MEMPOOL_H_1129300402_79845__ */

/* EOF */
