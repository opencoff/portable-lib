/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * bloom_internal.h - Internal header file for the implementation of
 * bloom filters.
 *
 * Copyright (c) 2016 Sudhi Herle <sw at herle.net>
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

#ifndef ___BLOOM_INTERNAL_H_1039032_1468443371__
#define ___BLOOM_INTERNAL_H_1039032_1468443371__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <math.h>

/*
 * Bloom filter types
 */
#define BLOOM_TYPE_COUNTING       0
#define BLOOM_TYPE_QUICK          1
#define BLOOM_TYPE_SCALE          2         // scalable quick bloom

// List of checksum algorithms we support
#define BLOOM_CKSUM_SHA256        0
#define BLOOM_CKSUM_BLAKE2b       1
#define BLOOM_CKSUM_DEFAULT       BLOOM_CKSUM_SHA256

#define BLOOM_HASH_FASTHALF       0         // Half a round of super fast hash

/*
 * Current version of the bloom filter that's serialized.
 */
#define BLOOM_VER0                0


/*
 * Common bloom filter header.
 *
 * For counting bloom filter: The counter is 8 bit to keep the arithmetic
 * simple. Memory is cheap - so unless we are doing Zillions of
 * items, this strategy will work fine.
 *
 * We keep the most frequently used elements upfront - so they fill
 * an entire cache line.
 */
struct bloom
{
    uint64_t m;         // 'm' bits per partition
    uint64_t k;         // 'k' distinct partitions
    uint64_t salt;      // hash salt (generated randomly for each instance)
    uint8_t *bitmap;    // bloom filter bits (may be mmap'd)

    uint64_t size;      // number of elements in the filter
    uint64_t bmsize;    // size of bitmap in bytes
    uint32_t flags;     // Whether the bitmap is allocated or not
    uint32_t __pad0;    // padding
    double   e;         // expected error rate

};
typedef struct bloom bloom;


/*
 * Scalable bloom filter.
 *
 * A scalable bloom filter is defined by two parameters:
 *
 *   a) Growth scale factor: This controls the factor by which the
 *      next successive filter is sized. Default is 2. i.e., sizes
 *      are doubled (exponential growth).
 *
 *   b) Tightening ratio: This controls the FP probability for
 *      the next successive filter. In turn, this dictates the number
 *      of partitions (or hashes) used by the next successive filter.
 *      Default: 0.9
 */
struct scalable_bloom
{
    bloom   *bfa;   // array of bloom filters
    uint32_t len;   // number of bloom filters - each with successively larger partitions
    uint32_t cap;   // array capacity

    uint32_t scale; // we increase the size of the underlying filter by this factor
    double   r;     // tighetening ratio for error prob for filter growth
    Bloom   *b;     // back pointer to parent Bloom instance
};
typedef struct scalable_bloom scalable_bloom;



/**
 * Return estimated fill ratio for the bloom filter.
 */
static inline double
bloom_fill_ratio_est(bloom *b)
{
    double r = (double)b->size / (double)b->m;
    return 1 - exp(-r);
}




/*
 * Internal routine to setup a naked bloom filter and its function
 * pointers.
 *
 * The actual filter memory and individual filters are done by the
 * unmarshal code.
 *
 * Returns:
 *      ptr to allocated & setup bloom filter
 *      0 on failure
 */
extern Bloom* __alloc_bloom(int typ, uint32_t n);


// Free a bloom filter
extern void __free_bloom(Bloom *b);


#define _d(x)   ((double)(x))
// Make 'k' from 'e'
static inline uint64_t
make_k(double e)
{
    double z = ceil(-log(e)/log(2.0));
    return _U64(z);
}


// Given n, e as above, return 'm' the number of bits
static inline uint64_t
make_m(uint64_t n, double e)
{
    double z0 = -log(e) / (log(2.0) * log(2.0));
    return n * _U64(ceil(z0));
}


// Given m, e as above, return 'n' the number of filter elements we
// can safely hold
static inline uint64_t
make_n(uint64_t m, double e)
{
    double z0 = (log(2.0) * log(2.0)) / -log(e);
    double n0 = _d(m) * z0;
    return _U64(ceil(n0));
}


static inline double
make_e(uint64_t k)
{
    /*
     * Finally, calculate the expected error rate given 'k' and 'm'
     *
     * k = -log(e) / log(2); so,
     *
     * e = exp(-k * log(2))
     */
#define _d(x)   ((double)(x))
    return exp(-_d(k) * log(2.0));
}

#define inline /**/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___BLOOM_INTERNAL_H_1039032_1468443371__ */

/* EOF */
