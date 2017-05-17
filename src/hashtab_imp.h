/* vim: expandtab:tw=68:ts=4:sw=4:
 * 
 * hashtab_imp.h -  Internal guts of the hash table module.
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

#ifndef __HASH2_IMP_H__
#define __HASH2_IMP_H__

#include "utils/hashtab.h"
#include "fast/list.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Each item in a collision chain is called a hash-node.
 *
 * The hash_node_head points to the beginning of such nodes.
 */
struct hash_node
{
    SL_LINK(hash_node) link;

    uint32_t hash;

    void*   data;
};
typedef struct hash_node hash_node;


/*
 * Head of the collision chain.
 */
SL_HEAD_TYPEDEF(hash_node_head, hash_node);


struct hash_bucket
{
    /* head of linked list of nodes. */
    hash_node_head head;


    /* Number of nodes in this bucket. */
    size_t count;

    /* Per bucket locking. */
    lockmgr lock;
};
typedef struct hash_bucket  hash_bucket;



struct hash_table
{
    /* Hash buckets. */
    hash_bucket *  buckets;

    /* table size; always a power of two. */
    size_t  size;

    /* max fill percentage - after which the table is split */
    size_t fillmax;

    /*
     * Randomizer for the hashfunc. Set when the table is created.
     * For a given table, this is constant until rehashed.
     */
    uint32_t random;

    /* functors */
    uint32_t (*hash)(const void*);
    int       (*cmp)(const void*, const void*);
    void     (*dtor)(void *);


    /* Memory allocator and lock manager */
    memmgr   mem;
    lockmgr  lock;

    /*
     * Statistics about the hash table. 
     * The information above (size,nodes,fill) are duplicates of
     * items in this struct to provide some speed benefits.
     */
    hash_table_stat stats;
};
typedef struct hash_table hash_table;



/*
 * Iterator specifc data: vanilla iterator to traverse the hash
 * table in no particular order.
 */
struct table_iter
{
    /* Current bucket being visited. */
    int bucket;
};
typedef struct table_iter table_iter;


/*
 * Indirect node to facilitate use of qsort() for ordering the array
 * of keys.
 */
struct indir_node
{
    hash_node* node;
    int (*cmp)(const void* lhs, const void* rhs);
};
typedef struct indir_node indir_node;

/*
 * Data specific to sorted iterator.
 */
struct sorted_iter
{

    size_t index,
           max;
    indir_node * nodes;
};
typedef struct sorted_iter sorted_iter;


/*
 * Data specific to a bucket iterator.
 */
struct bucket_iter
{
    hash_bucket *bucket;
};
typedef struct bucket_iter bucket_iter;




typedef struct hash_table_iter hash_table_iter;


/*
 * Each of the iter ops functions return 0 on success, < 0 on
 * failure.
 */
typedef int iter_init_f(hash_table_iter *, const void * param);
typedef int iter_begin_f(hash_table_iter *);
typedef int iter_next_f(hash_table_iter *);
typedef int iter_end_f(hash_table_iter *);
typedef void * iter_item_f(hash_table_iter *);

struct hash_table_iter_op
{
    iter_init_f     * init;
    iter_begin_f    * begin;
    iter_next_f     * next;
    iter_end_f      * end;
    iter_item_f     * item;
};
typedef struct hash_table_iter_op hash_table_iter_op;
#define _OPINIT(i,b,n,e,t)  { \
    i, b, n, e, t \
}


struct hash_table_iter
{
    hash_table  * table;

    /*
     * Operators that provide the real functionality of the
     * iterators.
     */
    const hash_table_iter_op * op;

    hash_node * cur;

    union
    {
        table_iter   table;
        sorted_iter  sorted;
        bucket_iter  bucket;
    } un;
};



#define tNEW(typ,tab)       (typ*)memmgr_alloc(&(tab)->mem, sizeof(typ))
#define tNEWA(typ,tab,n)    (typ*)memmgr_alloc(&(tab)->mem, (n)*sizeof(typ))
#define tFREE(tab,p)        memmgr_free(&(tab)->mem, (p))

#ifdef __cplusplus
}
#endif

#endif /* __HASH2_IMP_H__ */
