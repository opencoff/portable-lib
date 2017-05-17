/* :vim: ts=4:sw=4:expandtab:tw=68:
 * 
 * hashtab.h - Flexible Object oriented, policy based hash table.
 *
 * Copyright (C) 1997-2006 Sudhi Herle <sw at herle.net>
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
 * Creation date: Wed Jul 14 15:22:47 1997
 */

#ifndef __HASH_H_1137905286_65488__
#define __HASH_H_1137905286_65488__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include <stdint.h>
#include "utils/memmgr.h"
#include "utils/lockmgr.h"

    
/*
 * Key comparison function that must return:
 *      > 0 if lhs > rhs
 *      < 0 if lhs < rhs
 *      0   if lhs == rhs
 */
typedef int hash_cmp_f(const void* lhs, const void* rhs);


/*
 * This is how a hash function looks like.
 * User supplied hash functions should just compute a numeric value
 * for the string and NOT do any kind of "modulo arithmetic" to fit
 * the resultant value into a hash bucket!
 */
typedef uint32_t hash_func_f(const void*);


/*
 * Policy/Traits for an instance of the hash table.
 * These traits describe the allocation, de-allocation, hash
 * function etc.
 */
struct hash_table_policy
{
    /*
     * hash func & comparison func
     * Must be non-null
     */
    uint32_t (*hash)(const void*);
    int      (*cmp)(const void* a, const void* b);

    /* Element destructor
     * Can be null.
     */
    void (*dtor)(void*);


    /* Max fill percentage before the hash table is "split" */
    size_t fillmax;


    /*
     * log base 2 of the size of the hash table;
     *
     * The actual size of the hash table is "1 << logsize"; i.e.,
     * it's a power of 2. These are always the best sizes.
     * (cf. Bob Jenkins - www.burtleburtle.net/~bob/)
     */
    size_t logsize;


    /* Memory manager (allocator, etc.) */
    memmgr   mem;

    /* Locking semantics. */
    lockmgr  lock;
};
typedef struct hash_table_policy hash_table_policy;



/* Opaque type representing a hash table */
typedef struct hash_table* hash_table_t;

/*
 * Opaque type representing an iterator to traverse the
 * hash table.
 */
typedef struct hash_table_iter* hash_table_iter_t;



/* Hash table statistics */
struct hash_table_stat
{
    /* Number of hash buckets */
    size_t size;

    /* Number of nodes in the hash table */
    size_t nodes;

    /* Fill level (no. of buckets with nodes) */
    size_t fill;
    
    /* Maximum chain length (in case of collisions) */
    size_t maxchainlen;

    /* Number of operations of a particular kind */
    int splits;
    int inserts;
    int lookups;
    int failed_lookups;
    int replaces;
    int deletes;
};
typedef struct hash_table_stat hash_table_stat;



/*
 * Create a new hash table. The new hash table will use memory
 * manager `a' to get/free its internal storage. If `size' is
 * non-zero, it will be used as the initial size for the hash-table.
 * If `hash' is non-zero, it will be used as the hash function
 * (instead of the default). If `dtor' is non-zero, then it will be
 * called whenever a pair is deleted from the hash table. If
 * `fillmax' is non-zero, it will be used as the cutoff value for
 * the hash table "load". When this threshold is exceeded, the hash
 * table size is doubled.
 * Returns:
 *   On success:
 *          0
 *   On failure: < 0
 *      -EINVAL     if any input parameter was invalid
 *      -ENOMEM     if no memory available.
 */
extern int hash_table_new(hash_table_t * ret, const hash_table_policy *);




/*
 * Delete a hash table created with hash_table_new().
 */
extern void hash_table_delete(hash_table_t);



/*
 * Insert a new item `pair' into the hash table. If the key of the
 * item being inserted already exists in the hash table, this
 * function returns an error.
 * Returns:
 *   On success: 0  -- successful insertion.
 *   On failure: < 0:
 *      -EEXIST     if item exists
 *      -ENOMEM     if memory exhausted
 *   
 */
extern int hash_table_insert(hash_table_t, const void * pair);




/*
 * Insert a new item 'pair' into the hash table. If the item exists,
 * return a pointer to the old item. Else, insert the item and
 * return a pointer to it. In other words, ensure that this item is
 * in the hash table.
 * Returns:
 *   On success: pointer to pair in hash table
 *   On failure: 0.
 */
extern void * hash_table_probe(hash_table_t, const void * pair);



/*
 * Find the item whose key matches 'key'.
 * Even though a keyval_pair is passed, this function only looks at
 * 'key' and 'keylen'. The user supplied hash function can choose to
 * look at other fields of keyval_pair if it desires.
 *
 * Returns:
 *   On success: True. It also sets *ret to the datum if ret is
 *                  non-null.
 *   On failure: False (0).
 */
extern int hash_table_lookup(hash_table_t, const void * key, void** p_ret);





/*
 * Insert a new item `pair' into the hash table. If the key of the
 * item being inserted already exists in the hash table, this
 * function replaces the existing key with the new value.
 *
 * If the pair replaces an existing pair, the destructor for the
 * original pair is called.
 * Returns:
 *   On success: 0  -- plain insertion.
 *   On failure: < 0
 */
extern int hash_table_replace(hash_table_t, const void * pair);



/*
 * Remove an item from the hash table whose key matches 'key'.
 * Even though a keyval_pair is passed, this function only looks at
 * 'key' and 'keylen'. The user supplied hash function can choose to
 * look at other fields of keyval_pair if it desires. The return
 * parameter 'p_val' if non-null, holds the data that is removed.
 *
 * Returns:
 *   On success: True
 *   On failure: False (0)
 */
extern int hash_table_remove(hash_table_t, const void * key, void** p_ret);




/*
 * Conditionally remove one or more items from the hash table. The
 * supplied predicate 'pred' is called for every hash table item. An
 * item in the hash table is removed if the predicate returns
 *
 * 'cookie' is an opaque caller supplied parameter passed to the
 * predicate.
 *
 * NOTE: This function traverses _every_ element in the hash table.
 *
 * Returns:
 *  On success: >= 0
 *      Number of items deleted
 *      
 *  On failure: < 0
 *      -EINVAL     if the predicate is NULL.
 */
extern int hash_table_remove_if(hash_table_t,
             int (*pred) (void * cookie, const void * p),
             void * cookie);





/*
 * Function applicator. Call the supplied function `apply' for
 * _every_ item in the hash table.
 *
 * NOTE: DO NOT TRY TO INSERT/DELETE ITEMS FROM THE HASH TABLE IN
 * THE APPLICATOR 'apply'. You will deadlock and/or corrupt the
 * table.
 */
extern void hash_table_apply(hash_table_t,
        void (*apply)(void * cookie, const void *), void * cookie);



/*
 * Return the statistics for the hash table.
 */
extern hash_table_stat * hash_table_stats(hash_table_t, hash_table_stat *);



/*
 *      --- Iterator interface to hash table ---
 *
 * Usage:
 *
 * hash_table_iter_t iter = hash_table_iter_new(tab);
 * keyval_pair * pair;
 *
 *
 * for (hash_table_iter_begin(iter); (pair = hash_table_iter_item(iter);
 *                  hash_table_iter_next(iter) )
 * {
 *      ... Process `pair' here ...
 * }
 *
 * hash_table_iter_delete(iter);
 *
 * NOTE:
 *      o  Failure to delete the iterator after its use is a serious
 *         memory leak.
 *
 *      o  Changing the contents of the hash table while traversing
 *         it via the iterator leads to UNDEFINED behavior.
 *         (i.e., if your machine reboots because of it, don't blame
 *          me.)
 */


/*
 * Initialize the iterator `iter' to work on hash table `ht'. If
 * 'sorted' is true, then traverse the elements in sorted order.
 * Returns:
 *   On success: 0 and sets p_ret to the newly created iterator.
 *   On failure:
 *      -EINVAL if parameters are invalid
 */
extern int hash_table_iter_new(hash_table_iter_t* p_ret,
                               hash_table_t t, int sorted);



/*
 * Initialize and return an iterator to iterate through the
 * hash-table buckets matching key 'k'.
 * Even though a keyval_pair is passed, this function only looks at
 * 'key' and 'keylen'. The user supplied hash function can choose to
 * look at other fields of keyval_pair if it desires.
 *
 * Returns:
 *   On success: 0 and sets p_ret to the newly created iterator
 *   On failure:
 *      -EINVAL if table or p_ret is NULL
 *      -ENOMEM if unable to allocate any memory.
 */
extern int hash_table_bucket_iter_new(hash_table_iter_t* p_ret,
                                      hash_table_t, const void * k);



/*
 * The inverse of the hash_table_iter_new() functions.
 */
extern void hash_table_iter_delete(hash_table_iter_t);




/*
 * Start the iterator to point to the first bucket with valid data.
 * Returns:
 *   On success: 0
 *   On failure: -1
 */
extern int hash_table_iter_first(hash_table_iter_t);



/*
 * Move the iterator to the next item in the hash table.
 * Returns:
 *   On success: 0
 *   On EOF: -EOF
 *   On failure: -errno
 */
extern int hash_table_iter_next(hash_table_iter_t);



/*
 * Return the item pointed to by the iterator.
 * Returns:
 *   On success: Pointer to the pair
 *   On EOF:     0
 *   On failure: -1
 */
extern void * hash_table_iter_item(hash_table_iter_t);




/*
 * Standard hash functions:
 *
 * NOTE: These assume the keys are NULL terminated 'const char*'
 */





/* Macro overrides for simple functions */

/* Functional style case for double */
#define __d(n)              ((double)(n))

/* Fill ratio: percentage of buckets that are actually occupied */
#define hash_table_fill_percent(st)     ((100 * __d((st)->fill)) / __d((st)->size))

/* Expected nodes/bucket */
#define hash_table_density(st)          (__d((st)->nodes) / __d((st)->size))

/* Actual nodes/bucket. */
#define hash_table_actual_density(st)   ((st)->fill ?  (__d((st)->nodes) / __d((st)->fill)) : 0)

/* Average chainlen is the integer portion of the actual density. */
#define hash_table_avg_chainlen(st)     ((int)hash_table_actual_density(st))



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __HASH_H_1137905286_65488__ */

/* EOF */
