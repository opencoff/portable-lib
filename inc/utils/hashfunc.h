/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/hashfunc.h - Several popular hash functions.
 *
 * Copyright (c) 2011 Sudhi Herle <sw@herle.net>
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
 * Hash functions available here:
 *  
 *  o Hsieh Hash: Paul Hsieh's super-fast hash function
 *    http://www.azillionmonkeys.com/qed/hash.html
 *    Synopsis:
 *     - it beats Jenkins' hash in terms of performance
 *     - it has identical/better collisin distribution than Jenkins
 *     - it is considered the best hash function as of Dec 2009
 *
 *  o Austin Appleby's MurMurHash3
 *     - http://code.google.com/p/smhasher/
 *     - Faster than Hsieh Hash, with better distribution (mixing)
 *     - Comes in 3 variants:
 *          o 32-bit for use in hash tables
 *          o 128-bit for use in 32-bit machines
 *          o 128-bit for use in 64-bit machines
 *       The 128-bit variants can be used for generating unique
 *       identifiers from long blocks of data
 *
 *  o Fowler, Noll, Vo hash function:
 *     - http://www.isthe.com/chongo/tech/comp/fnv/
 *
 *  o Jenkins' Hash:
 *     - http://www.burtleburtle.net/bob/hash/
 */

#ifndef ___UTILS_HASHFUNC_H_9943808_1296677856__
#define ___UTILS_HASHFUNC_H_9943808_1296677856__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h>
#include <stdint.h>


/* 128-bit integer type */
typedef struct uint_128_t
{
    uint64_t v[2];
} uint_128_t;


/**
 * MurMurHash3 32-bit variant.
 *
 * @param pdata     key to be hashed
 * @param len       length of key in bytes
 *
 * @return 32-bit hash value of the input
 */
uint32_t murmur3_hash_32(const void* pdata, size_t len, uint32_t initval);



/**
 * City Hash from Google - 128 bit variant
 */
uint_128_t city_hash_128(const void* pdata, size_t len, uint_128_t seed);

/**
 * City Hash from Google - 128 bit variant
 */
uint64_t city_hash(const void* pdata, size_t len, uint64_t seed);


/**
 * MurMurHash3 128-bit variant for 32-bit arch.
 *
 * @param pdata     key to be hashed
 * @param len       length of key in bytes
 *
 * @return 128-bit hash value of the input
 */
uint_128_t murmur3_hash_128(const void* pdata, size_t len, uint32_t initval);


/**
 * MurMurHash3 128-bit variant for 64-bit arch.
 *
 * @param pdata     key to be hashed
 * @param len       length of key in bytes
 *
 * @return 128-bit hash value of the input
 */
uint_128_t murmur3_hash64_128(const void* pdata, size_t len, uint32_t initval);



/**
 * Hash an opaque buffer into a 32-bit value using Paul Hsieh's hash
 * function.
 *
 * @param pdata     Key to be hashed
 * @param len       Length of key in bytes
 *
 * @return 32-bit value (hash value)
 */
uint32_t hsieh_hash(const void * pdata, size_t len, uint32_t seed);


/**
 * Hash an opaque buffer into a 32-bit value using Fowler, Noll, Vo
 * hash function.
 *
 * @param pdata     Key to be hashed
 * @param len       Length of key in bytes
 *
 * @return 32-bit value (hash value)
 */
uint32_t fnv_hash(const void* pdata, size_t len, uint32_t seed);


/**
 * Hash an opaque buffer into a 64-bit value using Fowler, Noll, Vo
 * hash function.
 *
 * @param pdata     Key to be hashed
 * @param len       Length of key in bytes
 *
 * @return 64-bit value (hash value)
 */
uint64_t fnv_hash64(const void* pdata, size_t len, uint64_t seed);


/*
 * Bob Jenkins' hash function
 *
 * http://www.burtleburtle.net/bob/hash/
 *
 * jenkins_hash.h, by Bob Jenkins, December 1996, Public Domain.
 * You can use this free for any purpose.  It has no warranty.
 *
 * Portable interface by Sudhi Herle <sw at herle.net>
 * If you cut-and-paste the section containing Jenkins' hash
 * function, then it is licensed under the same terms as Bob
 * Jenkins' original file. If you keep this entire header file
 * (hashfuncs.h) as is, then it is licensed under the terms of
 * GPLv2.
 */


/**
 * @memo Jenkins' hash function operating on byte arrays.
 *
 * @doc  Hash a variable-length key into a 32-bit value
 *   k     : the key (the unaligned variable-length array of bytes)
 *   len   : the length of the key, counting by bytes
 *   level : can be any 4-byte value
 * Returns a 32-bit value.  Every bit of the key affects every bit of
 * the return value.  Every 1-bit and 2-bit delta achieves avalanche.
 * About 36+6len instructions.
 * 
 * The best hash table sizes are powers of 2.  There is no need to do
 * mod a prime (mod is sooo slow!).  If you need less than 32 bits,
 * use a bitmask.  For example, if you need only 10 bits, do
 *   h = (h & hashmask(10));
 * In which case, the hash table should have hashsize(10) elements.
 * 
 * If you are hashing n strings (ub1 **)k, do it like this:
 *   for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);
 * 
 * By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  You may use this
 * code any way you wish, private, educational, or commercial.  It's free.
 * 
 * See http://burlteburtle.net/bob/hash/evahash.html
 * Use for hash table lookup, or anything where one collision in 2^32 is
 * acceptable.  Do NOT use for cryptographic purposes.
 *
 * If you didn't already figure out what hashsize(n) and hashmask(n)
 * are all about, here they are:
 *  #define hashsize(n) ((ub4)1<<(n))
 *  #define hashmask(n) (hashsize(n)-1)
 *
 * @param k         Key to be hashed
 * @param length    Length of key in bytes
 * @param initval   previous hash value (or aribtrary value)
 *
 * @return 32-bit value (hash value)
 */
uint32_t jenkins_hash(const void* k, size_t length, uint32_t initval);

#define jenkins_hash_str(str) jenkins_hash((const void*)(str), \
                                                strlen(str), 0)


/**
 * @memo Jenkins' hash that works with long-word quantities.
 *
 * @doc This works on all machines.  This is identical to
 * jenkins_hash() on little-endian machines, except that the
 * length has to be measured in ub4s instead of bytes.  It is
 * much faster than hash().  It  requires:
 *  -- that the key be an array of ub4's, and
 *  -- that all your machines have the same endianness, and
 *  -- that the length be the number of ub4's in the key
 *
 * @param k         key to be hashed
 * @param length    Length of key in long-words
 * @param initval   Previous hash value (or some aribtrary value)
 *
 * @return 32-bit value (hash value)
 */
uint32_t jenkins_hash_long(uint32_t * k, size_t length, size_t initval);


/*
 * The next two hash functions are by Zilong Tan
 *
 * The code is  (C) 2012 Zilong Tan (eric.zltan@gmail.com)
 */

/**
 * fasthash32 - 32-bit implementation of fasthash
 * @buf:  data buffer
 * @len:  data size
 * @seed: the seed
 */
uint32_t fasthash32(const void *buf, size_t len, uint32_t seed);

/**
 * fasthash64 - 64-bit implementation of fasthash
 * @buf:  data buffer
 * @len:  data size
 * @seed: the seed
 */
uint64_t fasthash64(const void *buf, size_t len, uint64_t seed);


/**
 * yorrike_hash32 - Derivative of FNV1A from http://www.sanmayce.com/Fastest_Hash/
 */
uint32_t yorrike_hash32(const void* x, size_t n, uint32_t seed);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_HASHFUNC_H_9943808_1296677856__ */

/* EOF */
