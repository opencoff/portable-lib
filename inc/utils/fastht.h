/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * inc/utils/fastht.h - Fast hashtable with array based buckets
 *
 * Copyright (c) 2019 Sudhi Herle <sw at herle.net>
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
 * o C++ variant of 'fast-ht.c'.
 * o Each hash bucket points to a "bag"; and a bag contains 8 nodes
 *   in an array. Bags are linked in a list.
 * o Each entry in the array is a pointer to a node; deleted entries
 *   in the array are marked by a nil pointer.
 * o Bags are freed when they are empty (i.e., all entries point to
 *   nil).
 * o Callers must provide a hash function and an equality
 *   comparator.
 * o We don't parameterize the allocator; perhaps in the future.
 */

#ifndef ___INC_UTILS_FASTHT_HPP__bvPXRcBpxfyui1IT___
#define ___INC_UTILS_FASTHT_HPP__bvPXRcBpxfyui1IT___ 1

#include <assert.h>
#include <utility>
#include <string>
#include <stdlib.h>
#include <stdint.h>
#include "fast/list.h"

#ifndef __OpenBSD__
#include "utils/arc4random.h"
#endif

// Fast Hash table for key K and value V.
// No iterators; only membership, adds/deletes supported
template <typename K, typename V> class fht {
public:
    typedef K        key_type;
    typedef V        value_type;
    typedef uint64_t hashfunc(const K& k);
    typedef bool     equalfunc(const K& a, const K& b);

protected:
    // A node holds the key, value and the computed hash.
    struct node {
        node(uint64_t h, const K& k, const V& v)
            : hash(h), key(k), value(v) {}

        node(const node& n) = default;
        node(node&& n) = default;
        ~node() = default;

        uint64_t hash;  // nonceless hash of key
        K key;
        V value;
    };

#define FASTHT_BAGSZ       4
#define FILLPCT            75

    // A bag is a contiguous array of nodes.
    // It is also an intrusive linked list
    struct bag {
        node *a[FASTHT_BAGSZ];
        SL_ENTRY(bag) link;

        bag() {
            for (int i = 0; i < FASTHT_BAGSZ; i++) a[i] = 0;
        };

        ~bag() {
            for (int i = 0; i < FASTHT_BAGSZ; i++) {
                node *x = a[i];

                delete x;
            }
        }

        // return true if bag is empty.
        bool is_empty() {
            bool r = true;
            for (int i = 0; i < FASTHT_BAGSZ; i++) r = r && (a[i]);
            return r;
        }
    };

    SL_HEAD_TYPEDEF(baghead, bag);

    // A bucket holds the collision chain; each element in the chain
    // is a bag of "BAGSZ" contiguous nodes. In the future, we can
    // add RWlocks to each bucket for multi-threaded systems.
    // We can also add a second set of RWlocks for each bag.
    struct bucket {
        baghead  bags   = {0};
        uint32_t nitems = 0;      // number of items in this bucket
        uint32_t nbags  = 0;      // number of bags

        ~bucket() {
            while (!SL_EMPTY(&bags)) {
                auto r = SL_FIRST(&bags);
                SL_REMOVE_HEAD(&bags, link);
                delete r;
            }
        }
    };

    bucket  *m_tab;         // the actual table (doubles when full)

    uint64_t m_size,        // size of table; *always* a power-of-2
             m_seed;        // random# -- seed for keyed hash function

    hashfunc  *m_hasher;    // The hash function
    equalfunc *m_equal;     // predicate that returns two if two "keys" are identical.

    // stats
    uint64_t m_nodes    = 0, // number of nodes in hash table
             m_fill     = 0; // number of occupied buckets

    uint32_t m_splits   = 0, // number of times we split the table
             m_bagmax   = 0, // max number of bags in a bucket
             m_chainmax = 0; // max number of items in a bucket

public:
    fht(hashfunc *hasher, equalfunc *equal, size_t n = 0)
                : m_hasher(hasher) {

        m_size = n == 0 ? 128 : nextpow2(n);
        m_tab  = new bucket[m_size];

        arc4random_buf(&m_seed, sizeof m_seed);
    }

    fht() = delete;
    fht(const fht&) = delete;
    fht<K,V>& operator=(const fht<K, V>& that) = delete;

    // Move constructor
    fht(fht&& o)
       : m_size(o.m_size), m_seed(o.m_seed),
         m_hasher(o.m_hasher),
         m_nodes(o.m_nodes), m_fill(o.m_fill),
         m_splits(o.m_splits), m_bagmax(o.m_bagmax), m_chainmax(o.m_chainmax)
    {
        if (m_tab) delete[] m_tab;

        m_tab = o.m_tab;
        o.m_tab = 0;
    }

    // Move assignment
    fht<K,V>& operator=(const fht<K, V>&& o) {
        bucket *b = m_tab;
        *this = o;

        if (b) delete[] b;

        o.m_tab = 0;
    }

    virtual ~fht() {
        if (m_tab) delete[] m_tab;
    }


    // Read various fields
    uint64_t nodes()        { return m_nodes;    }
    uint64_t size()         { return m_size;     }
    uint64_t splits()       { return m_splits;   }
    uint64_t maxbags()      { return m_bagmax;   }
    uint64_t maxchainlen()  { return m_chainmax; }

    // Return true if hash table is empty
    bool empty() const { return m_nodes; }

    // Return true if key exists, false otherwise.
    bool exists(const K& key) {
        auto r = xfind(key, false);
        return r.first;
    }

    // # of occupied buckets
    double fill() const {
        return (100.0 * double(m_fill))/ double(m_size);
    }

    // actual nodes/bucket
    double density() const {
        return m_fill > 0 ? double(m_nodes) / double(m_fill) : 0.0;
    }

    // expected nodes/bucket
    double exp_density() const {
        return double(m_nodes) / double(m_size);
    }


    // insert only if not already present; return if already
    // present, false otherwise.
    std::pair<bool, V*> probe(const K& key, const V& value) {
        uint64_t h = m_hasher(key);
        bucket *b  = hash(m_tab, h);

        auto r = insert(b, h, key, value);
        if (r.first) return std::make_pair(true, &r.second->value);

        if (b->nitems == 1) {
            m_fill++;
            if (((m_fill * 100) / (1 + m_size)) > FILLPCT) {
                m_splits++;
                resize();
                goto end;
            }
        }

        if (b->nbags  > m_bagmax)   m_bagmax   = b->nbags;
        if (b->nitems > m_chainmax) m_chainmax = b->nitems;

    end:
        return std::make_pair(false, &r.second->value);
    }

    // Find 'key' in table; if key is found, then return <true,
    // value>. Otherwise, return <false, 0>
    // The returned value is a pointer to the value held in the
    // node. Callers can modify it if they wish.
    std::pair<bool, V*> find(const K& key) {
        uint64_t h = m_hasher(key);
        bucket *b  = hash(m_tab, h);
        node **px  = 0;
        bag *g;

        SL_FOREACH(g, &b->bags, link) {
            px = &g->a[0];

#define SRCHx(x) do { \
                    node *x = *px;                   \
                    if (x && x->hash == h            \
                          && m_equal(key, x->key)) { \
                        return std::make_pair(true, &x->value); \
                    }       \
                    px++;   \
                } while(0)

#define SRCH(x) do { \
                    node *x = *px;                   \
                    if (x && x->hash == h) {         \
                        return std::make_pair(true, &x->value); \
                    }       \
                    px++;   \
                } while(0)

            switch (FASTHT_BAGSZ) {
                default:            // fallthrough
                case 8: SRCH(x);    // fallthrough
                case 7: SRCH(x);    // fallthrough
                case 6: SRCH(x);    // fallthrough
                case 5: SRCH(x);    // fallthrough
                case 4: SRCH(x);    // fallthrough
                case 3: SRCH(x);    // fallthrough
                case 2: SRCH(x);    // fallthrough
                case 1: SRCH(x);    // fallthrough
            }
        }

        return std::make_pair<bool, V*>(false, 0);
    }

    // Remove if key exists; return true if found, false otherwise.
    bool remove(const K& key) {
        uint64_t h = m_hasher(key);
        bucket *b  = hash(m_tab, h);
        node **px  = 0;
        bag *g;

        SL_FOREACH(g, &b->bags, link) {
            px = &g->a[0];
#define DELX(x) do { \
                    node *x = *px;                   \
                    if (x && x->hash == h) {         \
                        goto found;                  \
                    }                                \
                    px++;                            \
                } while(0)

#define DELXx(x) do { \
                    node *x = *px;                   \
                    if (x && x->hash == h            \
                          && m_equal(key, x->key)) { \
                        goto found;                  \
                    }                                \
                    px++;                            \
                } while(0)

            switch (FASTHT_BAGSZ) {
                default:            // fallthrough
                case 8: DELX(x);    // fallthrough
                case 7: DELX(x);    // fallthrough
                case 6: DELX(x);    // fallthrough
                case 5: DELX(x);    // fallthrough
                case 4: DELX(x);    // fallthrough
                case 3: DELX(x);    // fallthrough
                case 2: DELX(x);    // fallthrough
                case 1: DELX(x);    // fallthrough
            }
        }

        return false;

found:
        delete *px;
        *px = 0;

        if (--b->nitems == 0) m_fill--;
        m_nodes--;

#if 0
        if (bg->is_empty()) {
            b->bags.remove(bg);
            b->nbags--;
        }
#endif

        return true;
    }


    // consistency check on the whole table
    void check() {
        bucket *o = m_tab,
               *e = o + m_size;

        uint64_t n = 0;
        for (; o < e; o++) {
            bag *g;
            SL_FOREACH(g, &o->bags, link) {
                for (int i = 0; i < FASTHT_BAGSZ; i++) {
                    node *p = g->a[i];
                    if (p) {
                        n++;

                        auto j = hashseed(p->hash, m_seed);
                        auto x = &m_tab[j & (m_size-1)];

                        assert(x == o);
                    }
                }
            }
        }

        assert(n == m_nodes);
    }


protected:

    // Compression function from fasthash
    static inline uint64_t __mix(uint64_t h) {
        h ^= (h >> 23);
        h *= 0x2127599bf4325c37ULL;
        h ^= (h >> 47);
        return h;
    }

    // fasthash64() - but tuned for exactly _one_ round and
    // one 64-bit word.
    //
    // Borrowed from Zilong Tan's superfast hash.
    // Copyright (C) 2012 Zilong Tan (eric.zltan@gmail.com)
    //
    // given a key-hash, return its bucket. Use the random nonce to
    // mix the hash.
    static inline uint64_t hashseed(uint64_t k, uint64_t seed) {
        const uint64_t m = 0x880355f21e6d1965ULL;
        uint64_t       h = 8 * m;

        h ^= __mix(k);
        h *= m;

        h ^= __mix(seed);
        h *= m;

        return h;
        //return (seed * m) ^ k;
    }

    // hash with the default seed
    bucket *hash(bucket *tab, uint64_t k) {
        uint64_t h = hashseed(k, m_seed);
        return &tab[h & (m_size-1)];
    }


    // grow the hash table (double the # of buckets); redistribute
    // every element. For added security, re-seed the nonce.
    bucket *resize() {
        uint64_t seed;
        uint64_t n = m_size * 2;

        bucket *b = new bucket[n],
               *o = m_tab,
               *e = o + m_size;

        uint64_t bagmax   = 0,
                 chainmax = 0,
                 fill     = 0;

        arc4random_buf(&seed, sizeof seed);

        // redistribute each node into the new table.
        // XXX We need to find a way to only move 50% of the items.
        for (; o < e; o++) {
            while (!SL_EMPTY(&o->bags)) {
                auto g = SL_FIRST(&o->bags);
                SL_REMOVE_HEAD(&o->bags, link);

                assert(g);
                for (int i = 0; i < FASTHT_BAGSZ; i++) {
                    node *p = g->a[i];

                    if (p) {
                        uint64_t h = hashseed(p->hash, seed);
                        bucket  *x = &b[h & (n-1)];

                        insert_quick(x, p);

                        if (x->nbags  > bagmax)   bagmax   = x->nbags;
                        if (x->nitems > chainmax) chainmax = x->nitems;
                        if (x->nitems == 1)       fill++;

                        g->a[i] = 0;
                    }
                }

                delete g;
            }
        }

        delete[] m_tab;

        m_seed = seed;
        m_tab  = b;
        m_size = n;
        m_fill = fill;

        m_bagmax   = bagmax;
        m_chainmax = chainmax;

        return b;
    }


    // Insert if not present
    // If present: return pointer to its node
    // If not present: allocate new node and return pointer to it
    std::pair<bool, node *> insert(bucket *b, uint64_t hash, const K& key, const V& val) {
        node **pos = 0;
        bag *g;

        SL_FOREACH(g, &b->bags, link) {
            node **px = &g->a[0];
            node *x   = *px;

#define FIND(x) do {                \
                    if (x) {                    \
                        if (x->hash == hash)    \
                            return std::make_pair(true, x);\
                    } else {                    \
                        if (!pos) pos = px;     \
                    }                           \
                    px++;                       \
                } while (0)
#define FINDx(x) do {                \
                    if (!x) {       \
                        if (!pos) pos = px;   \
                    } else {        \
                        if (x->hash == hash && m_equal(x->key, key)) { \
                            return std::make_pair(true, x);            \
                        }                       \
                    }                           \
                    px++;                       \
                } while (0)

            switch (FASTHT_BAGSZ) {
                default:                // fallthrough
                case 8: FIND(x);        // fallthrough
                case 7: FIND(x);        // fallthrough
                case 6: FIND(x);        // fallthrough
                case 5: FIND(x);        // fallthrough
                case 4: FIND(x);        // fallthrough
                case 3: FIND(x);        // fallthrough
                case 2: FIND(x);        // fallthrough
                case 1: FIND(x);        // fallthrough
            }
        }

        if (!pos) {
            g   = new bag;
            pos = &g->a[0];

            SL_INSERT_HEAD(&b->bags, g, link);
            b->nbags++;
        }

        *pos   = new node(hash, key, val);
        b->nitems++;
        m_nodes++;
        return std::make_pair(false, *pos);
    }

    // Used by resize() to quickly insert into bucket 'b'
    void insert_quick(bucket *b, node* n) {
#define xPUT(x) do {     \
                    if (!*x) { \
                        *x = n; \
                        return;   \
                    }    \
                    x++; \
               } while(0)

        bag *g;
        SL_FOREACH(g, &b->bags, link) {
            node **x = &g->a[0];

            switch (FASTHT_BAGSZ) {
                default:             // fallthrough
                case 8: xPUT(x);     // fallthrough
                case 7: xPUT(x);     // fallthrough
                case 6: xPUT(x);     // fallthrough
                case 5: xPUT(x);     // fallthrough
                case 4: xPUT(x);     // fallthrough
                case 3: xPUT(x);     // fallthrough
                case 2: xPUT(x);     // fallthrough
                case 1: xPUT(x);     // fallthrough
            }
        }

        g = new bag;
        g->a[0] = n;
        SL_INSERT_HEAD(&b->bags, g, link);
        b->nbags++;
        b->nitems++;
    }

    static uint64_t nextpow2(uint64_t v) {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        return v+1;
    }
};


#endif /* ! ___INC_UTILS_FASTHT_HPP__bvPXRcBpxfyui1IT___ */

/* EOF */
