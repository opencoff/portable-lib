/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/bloom.h - Partitioned, counting and scalable bloom filters.
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
 *
 * Notation:
 * =========
 *   e  = false positive error rate that is desired (input parameter)
 *   n  = number of elements predicted by this bloom filter (input
 *        parameter)
 *
 *   m  = total number of 'slots' in this bloom filter. When
 *        partitioned, each partition will have m/k bits.
 *        IMPORTANT: In 'struct bloom', we use 'm' for the size of
 *        the partition - since that is the most frequently used
 *        value.
 *
 *   p  = fill ratio; we have hard coded this to be 1/2 (50%) in all
 *        the math that follows.
 *
 *   k  = number of hash functions and also the number of partitions
 *        of the slots. Each partition holds ceil(m/k) slots.
 *
 *   k = -log(e) / log(2)
 *   m = n * (-log(e) / (log(2) ** 2))
 *
 * Scalable Bloom Filters
 * ======================
 * The implementation of the scalable bloom filter closely follows
 * the ideas laid out in [3]. Continuing with their notation:
 *
 *   r  = error tighetening ratio for scalable bloom filter growth
 *   s  = filter size scale factor
 *
 * For each successive filter we create, the number of items in the
 * partition grows by a factor of 's'. Continuing with the notation
 * of 'm' as the number of elements per slice, for the i_th new filter
 * the number of elements per slice is:
 *
 *    m[i] = m * (s ** i)
 *
 *  Where 'm' is the same as m[0] -- the initial value for the first
 *  filter.
 *
 *  With s = 2, the above simplifies to:
 *
 *    m[i] = m << i
 *
 * Per [3]:
 *   o  0.5 <= r <= 0.9.  We choose 0.9 as the value for our
 *      purposes.
 *   o  s = 2. This gives us exponential growth on the number of
 *      elements we can hold. What this means is that 
 *
 * Notes:
 * ======
 *   o This implementation borrows ideas from [4] to create a
 *     partitioned bloom filter.
 *
 *   o This implementation uses optimizations in Mitzenmacher &
 *     Kirsch [1].
 *     We use a seeded 64-bit hash function: fasthash64 due to Zilong Tan.
 *     The seed is a random 64 bit quantity initialized when the filter
 *     is created. To be clear, this hash function is used in the
 *     sense of Mitzenmacher & Kirsch [1].
 *
 *   o Instead of assuming any specific hash function on the input
 *     bytes, this library requires the caller to provide a hashed
 *     uint64_t. The caller is responsible for ensuring they pick a
 *     good hash function of their choice (murmur, siphash24, FNV2a
 *     are all good choices). The library makes no other assumption
 *     on the provided input.
 *
 *   o The math above is from [2].
 *
 *   o The library provides facilities to safely marshal and
 *     unmarshall the bloom filter to a file. The marshalled data
 *     uses strong checksums (SHA256) on the filter bits as well as
 *     the header. The checksums are verified during unmarshaling.
 *
 * References:
 * ===========
 * [1] Less Hashing, Same Performance: Building a Better Bloom Filter
 *     http://www.eecs.harvard.edu/~kirsch/pubs/bbbf/rsa.pdf
 *
 * [2] https://en.wikipedia.org/wiki/Bloom_filter
 *
 * [3] Scalable Bloom Filters
 *     http://gsd.di.uminho.pt/members/cbm/ps/dbloom.pdf
 *
 * [4] Approximate caches for packet classification
 *     http://www.ieee-infocom.org/2004/Papers/45_3.PDF
 */

#ifndef ___UTILS_BLOOM_H_3279193_1446926677__
#define ___UTILS_BLOOM_H_3279193_1446926677__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include "utils/utils.h"

/*
 * Denotes whether the bloom filter bits are memory mapped or not.
 */
#define BLOOM_BITMAP_MMAP   (1 << 0)


/*
 * Interface for bloom filter. This contains pointers to actual
 * functions for specific filter types.
 *
 * One calls one of the constructors below to create an instance of
 * this interface.
 *
 * The elements are arranged so that the most important ones fit in
 * a cache line.
 */
struct Bloom
{
    void* filter;
    int  (*find )(void *, uint64_t v);
    void (*probe)(void *, uint64_t v);
    int  (*remov)(void *, uint64_t v);

    void  (*fini) (void *);
    char* (*desc) (void *, char* out, size_t outsz);

    const char* name;
    uint64_t    n;      // initial filter size
    double      e;      // initial error param
    uint8_t     typ;    // bloom filter type
};
typedef struct Bloom Bloom;


/**
 * Create and initialize a new COUNTING bloom filter to hold 'n' elements
 * with 50% fill rate satisfying an False positive error rate of
 * 'e'.
 */
extern Bloom* Counting_bloom_new(uint64_t n, double e);


/**
 * Create and initialize a new bloom filter to hold 'n' elements
 * with 50% fill rate satisfying an False positive error rate of
 * 'e'.
 *
 * If 'scalable' is true, then make this bloom filter automatically
 * scale to maintain the false-positive error rate.
 */
extern Bloom* Standard_bloom_new(uint64_t n, double e, int scalable);


/**
 * Initialize a new COUNTING bloom filter to hold 'n' elements
 * with 50% fill rate satisfying an False positive error rate of
 * 'e'. The caller is expected to provide the storage for the filter
 * instance.
 */
extern Bloom* Counting_bloom_init(Bloom*, uint64_t n, double e);


/**
 * Initialize a new bloom filter to hold 'n' elements
 * with 50% fill rate satisfying an False positive error rate of
 * 'e'. The caller is expected to provide the storage for the filter
 * instance.
 *
 * If 'scalable' is true, then make this bloom filter automatically
 * scale to maintain the false-positive error rate.
 */
extern Bloom* Standard_bloom_init(Bloom*, uint64_t n, double e, int scalable);

/**
 * Delete a bloom filter 'b' and free all storage associated with
 * it.
 */
extern void Bloom_delete(Bloom *b);

/**
 * Uninitialize a bloom filter 'b'. This does not free 'b'; the
 * caller is responsible for it.
 */
extern void Bloom_fini(Bloom *b);


/*
 * Next set of functions are generic and operate on any type of the
 * filter.
 */


/**
 * Probe():
 *   o if value not in table, add it
 *   o else, increment count
 */
static inline void
Bloom_probe(Bloom *b, uint64_t v)
{
    b->probe(b->filter, v);
}


/**
 * Find an element in the bloom filter.
 *
 * Return False if 'val' is definitely NOT in the bloom filter.
 * Return True if 'val' _maybe_ in the bloom filter or not.
 */
static inline int
Bloom_find(Bloom *b, uint64_t v)
{
    return b->find(b->filter, v);
}


/**
 * Remove an element from the filter.
 * Only applicable to counting bloom filters. No-op for the
 * non-counting variant.
 * 
 * Return False if element was not found, True otherwise.
 */
static inline int
Bloom_remove(Bloom *b, uint64_t v)
{
    return b->remov(b->filter, v);
}



/**
 * Return a human readbale name for this filter.
 */
static inline const char*
Bloom_name(Bloom *b)
{
    return b->name;
}



/*
 * Provide a description for this bloom filter
 */
static inline char *
Bloom_desc(Bloom *b, char *buf, size_t sz)
{
    return b->desc(b->filter, buf, sz);
}


/*
 * Compare two bloom filters and return true if they are equal.
 * Return false otherwise.
 */
extern int Bloom_eq(Bloom *a, Bloom *b);

/*
 * Marshal/Unmarshall interface
 */


/**
 * Marshal a bloom-filter into 'fname'.
 *
 * This function _always_ truncates the file before writing the
 * bloom filter.
 *
 * Return: 0 on success; -errno on failure
 *
 * Failure reasons:
 *    - unable to open or create file
 *    - unable to set/extend the file-size.
 *    - unale to mmap the file
 */
extern int Bloom_marshal(Bloom* b, const char *fname);


/**
 * Unmarshall a bloom-filter from 'fname' into bloom filter
 * 'b'. Flags is a bitmap:
 *   o  BLOOM_BITMAP_MMAP:  don't allocate memory for the bitmap,
 *      instead mmap the file contents.
 *
 *  Return:
 *   o 0 on success
 *   o -EINVAL: file is smaller than header size (128 bytes)
 *   o -EILSEQ: header has garbage or the checksum failed
 *   o -ENOMEM: No memory for bitmap
 *   o -errno:  for mmap() and open() related failures
 */
extern int Bloom_unmarshal(Bloom **p_ret,   const char* fname, uint32_t flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_BLOOM_H_3279193_1446926677__ */

/* EOF */
