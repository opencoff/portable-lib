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
 * Sensible default for modern 64-bit processors.
 *
 * No compiler provides us with a pragma or CPP macro for the cache
 * line size. Boo.
 *
 * If your CPU has a different cache line size, define this macro on
 * the command line (-DCACHELINE_SIZE=xxx).
 */
#ifndef CACHELINE_SIZE
#define CACHELINE_SIZE      64
#endif


#ifdef __GNUC__
#define __CACHELINE_ALIGNED __attribute__((aligned(CACHELINE_SIZE)))
#elif defined(_MSC_VER)
#define __CACHELINE_ALIGNED __declspec(align(CACHELINE_SIZE))
#else
#error "Can't define __CACHELINE_ALIGNED for your compiler/machine"
#endif /* __CACHELINE_ALIGNED */


#define FASTHT_BAGSZ       ((int)(CACHELINE_SIZE/sizeof(uint64_t)))
#define FILLPCT            85

/*
 * Hash Bucket: holds keys and values as separate arrays.
 * The keys and values are carefully placed to occupy cachelines.
 */
struct bag
{
    SL_ENTRY(bag) link;

    uint64_t __pad0[3];

    // Next two arrays take up two entire cachelines.

    uint64_t   hk[FASTHT_BAGSZ] __CACHELINE_ALIGNED;

    // start of second  cache line
    void*      hv[FASTHT_BAGSZ] __CACHELINE_ALIGNED;
};
typedef struct bag bag;

SL_HEAD_TYPEDEF(bag_head, bag);

/*
 * Hash bucket.
 */
struct hb
{
    bag_head  head;     // start of bags
    uint32_t  nodes;    // number of nodes in this list
    uint32_t  bags;     // number of bags
};
typedef struct hb hb;


/*
 * Hash table.
 */
struct ht
{
    hb      *b;         // array of buckets
    uint64_t n;         // number of buckets
    uint64_t salt;      // random seed
    uint64_t fill;      // number of buckets occupied.

    uint64_t maxfill;   // maximum allowed load factor % before splitting table
    uint64_t nodes;     // number of nodes in the hash table
    uint32_t splits;    // number of times HT is doubled
    uint32_t bagmax;    // max number of bags in a bucket
    uint32_t maxn;      // max number of items in a bucket
    uint32_t pad0;      // alignment
};
typedef struct ht ht;


/*
 * Initialize a hash table and return it.
 * 'size' is a size hint for initial number of buckets. The table
 * will grow dynamically if the fill percent is larger than
 * 'maxfill'.
 */
ht* ht_init(ht*, uint32_t size, uint32_t maxfill);


/*
 * Free any associated with the table.
 */
void ht_fini(ht*);


/*
 * Create and Initialize a hash table and return it.
 * 'size' is a size hint for initial number of buckets. The table
 * will grow dynamically if the fill percent is larger than
 * 'maxfill'.
 */
ht* ht_new(uint32_t size, uint32_t maxfill);


/*
 * Delete and free memory associated with the hash table.
 */
void ht_del(ht*);



/*
 * Add a new entry to the hash-table only if it doesn't already exist.
 * This is the main interface to adding new elements. The key is
 * 'hv' - an already hashed quantity.
 *
 * Return ptr to existing "val" if it exists, 0 if newly inserted.
 */
void* ht_probe(ht*, uint64_t hv, void *val);


/*
 * Replace an existing entry with a new "val".
 * Return true if replaced, false if 'hv' is not in the hash table.
 */
int ht_replace(ht*, uint64_t hv, void *val);


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



/*
 * Dump the hash table via caller provided output function.
 * The 'start' string is printed at the beginning of the dump.
 */
void ht_dump(ht*, const char *start, void (*dump)(const char*, size_t));


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_HT_H_668986_1448848235__ */

/* EOF */
