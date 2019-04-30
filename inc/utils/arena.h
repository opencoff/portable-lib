/* 
 * arena.h -- Lifetime based memory manager.
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
 * An Arena manages memory of objects based on their lifetime. i.e.,
 * individual objects are not deleted. The storage of all objects
 * are "deleted" together. Thus, allocations are "fast" and
 * de-allocations incur no-overhead! This will make for simpler
 * algorithms.
 *
 * e.g., symbol-table management: Symbol-tables of a particular
 * scope can all be deleted in one shot.
 *
 * It relies internally on malloc() and free() for its operations.
 */

#ifndef		__ARENA_H__
#define		__ARENA_H__		1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <string.h>
#include "utils/memmgr.h"

/* Opaque data type. */
typedef struct arena * arena_t ;


/* Create a new arena and return it as an output parameter 'ret_ptr'
 *
 * Returns:
 *   On Success: 0
 *   On failure: -EINVAL or -ENOMEM
 */
extern int arena_new(arena_t* ret_ptr, int alloc_chunk_size);


/* Allocate `n' bytes of storage from arena `a'.
 * Returns:
 *   On success: pointer to suitably aligned block of memory
 *               atleast `n' bytes big.
 *   On failure: NULL
 */
extern void * arena_alloc(arena_t a, int n);


/* Delete the entire arena `a' -- thus deallocating all individual
 * requests (via arena_alloc()) in one shot! */
extern void arena_delete(arena_t a);

/* Make an arena memory manager out of the arena `a' into `m'. */
#define arena_memmgr(m,a) memmgr_init(m, (Alloc_f *)arena_alloc, 0, a)


/*
 * Duplicate a string in this arena.
 */
static inline char*
arena_strdup(arena_t a, const char* s)
{
    size_t n = strlen(s) + 1;
    void*  p = arena_alloc(a, n);
    return (char *)memcpy(p, s, n);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* ! __ARENA_H__ */
/* EOF */
