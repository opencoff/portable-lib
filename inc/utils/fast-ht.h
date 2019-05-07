/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/ht.h - Linear, Open Addressed hash table
 *
 * Copyright (c) 2015 Sudhi Herle <sw at herle.net>
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
 * Notes
 * =====
 * o The hashtable "key" is a "uint64_t"; i.e., an already hashed
 *   quantity. Callers are responsible for using a good hash function.
 * o This key (hashed) is always assumed to be NON-ZERO.
 * o Value is an opaque "void *"; this library does not manage the
 *   memory or the lifetime of this void ptr.
 */

#ifndef ___UTILS_HT_H_668986_1448848235__
#define ___UTILS_HT_H_668986_1448848235__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include "fast/list.h"


/*
 * Hash node.
 *
 * For BAGSZ == 4, these nodes will occupy a full cache-line.
 */
struct hn
{
    uint64_t  h;    // hash
    void *    v;    // value
};
typedef struct hn hn;


#define FASTHT_BAGSZ       3
#define FILLPCT            87


#if    FASTHT_BAGSZ > 8

#warn "FASTHT_BAGSZ is too large; clamping at 8"
#undef FASTHT_BAGSZ

#define FASTHT_BAGSZ    8
#endif

/*
 * Bucket
 */
struct bag
{
    hn         a[FASTHT_BAGSZ];
    SL_ENTRY(bag) link;
    //uint64_t __pad0; // cache-line aligned for BAGSZ==3
};
typedef struct bag bag;

SL_HEAD_TYPEDEF(bag_head, bag);

/*
 * Hash bucket.
 */
struct hb
{
    bag_head  head;
    uint32_t  nodes;    // number of nodes in this list
    uint32_t  bags;     // number of bags
};
typedef struct hb hb;


/*
 * Hash table.
 */
struct ht
{
    uint64_t n;         // number of buckets
    uint64_t rand;      // random seed
    hb      * b;        // array of buckets

    uint64_t nodes;     // number of nodes in the hash table
    uint64_t fill;      // number of buckets occupied.

    uint32_t splits;    // number of times HT is doubled
    uint32_t bagmax;    // max number of bags in a bucket
    uint32_t maxn;      // max number of items in a bucket
};
typedef struct ht ht;


/*
 * Initialize a hash table and return it.
 * 'nlog2' is a size hint; it is log2() of the number of expected
 * nodes.
 */
ht* ht_init(ht*, uint32_t nlog2);


/*
 * Free any associated with the table.
 */
void ht_fini(ht*);


/*
 * Create and initialize a new hash table.
 */
ht* ht_new(uint32_t nlog2);


/*
 * Delete and free memory associated with the hash table.
 */
void ht_del(ht*);



/*
 * Add a new entry to the hash-table only if it doesn't already exist.
 * This is the main interface to adding new elements. The key is
 * 'hv' - an already hashed quantity.
 *
 * Return True if element exists, False otherwise.
 */
int  ht_probe(ht*, uint64_t hv, void*);



/*
 * Find the hash value 'hv' and return the corresponding node.
 * Return true on success, false if not found.
 */
int ht_find(ht*, uint64_t hv, void** p_ret);


/*
 * Remove the hash value 'hv' from the table.
 * Return true on success, false if not found.
 */
int ht_remove(ht*, uint64_t hv, void** p_ret);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_HT_H_668986_1448848235__ */

/* EOF */
