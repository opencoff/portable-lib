/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/oht.h - Linear, Open Addressed hash table
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
 *
 * o Callers are responsible for using an appropriate hash function
 * o The hash function should provide sufficient "mixing" of the
 *   input bits.
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
 * Hash node. Sized to occupy a full cache-line.
 */
struct ohn
{
    uint64_t  h;    // hash
    void *    v;    // value
};
typedef struct ohn ohn;


#define HB       8
#define FILLPCT  75

/*
 * Bucket
 */

struct ohb
{
    ohn      a[HB];     // array of HB nodes
    uint32_t  n;        // number of nodes in this array
};
typedef struct ohb ohb;


/*
 * Hash table.
 */
struct oht
{
    uint64_t n;         // number of buckets
    uint64_t novf;      // number of overflow nodes
    uint64_t rand;      // random seed
    ohb     * b;        // array of buckets
    ohn     * ovf;      // Overflow nodes (linearly probed)

    uint64_t nodes;     // number of nodes in the hash table
    uint64_t fill;      // number of buckets occupied.

    uint64_t povf;      // # of overflow probes
    uint64_t fovf;      // # of overflow finds

    uint32_t splits;    // number of times HT is doubled
    uint32_t maxn;      // max number of items in a bucket
};
typedef struct oht oht;


/*
 * Initialize a hash table and return it.
 * 'nlog2' is a size hint; it is log2() of the number of expected
 * nodes.
 */
oht* oht_init(oht*, uint32_t nlog2);


/*
 * Free any associated with the table.
 */
void oht_fini(oht*);


/*
 * Create and initialize a new hash table.
 */
oht* oht_new(uint32_t nlog2);


/*
 * Delete and free memory associated with the hash table.
 */
void oht_del(oht*);



/*
 * Add a new node only if it doesn't already exist.
 * This is the main interface to adding new elements.
 *
 * Return True if element exists, False otherwise.
 */
int  oht_probe(oht*, uint64_t hv, void*);



/*
 * Find the hash value 'hv' and return the corresponding node.
 * Return true on success, false if not found.
 */
int oht_find(oht*, uint64_t hv, void** p_ret);


/*
 * Remove the hash value 'hv' from the table.
 * Return true on success, false if not found.
 */
int oht_remove(oht*, uint64_t hv, void** p_ret);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_HT_H_668986_1448848235__ */

/* EOF */
