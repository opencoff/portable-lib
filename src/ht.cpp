/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * ht.cpp - Fast hashtable with array based buckets
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
#include <list>
#include <utility>
#include <string>
#include <stdint.h>

/*
 * Round 'v' to the next closest power of 2.
 */
#define NEXTPOW2(v)     ({\
                            uint64_t n_ = ((uint64_t)(v))-1;\
                            n_ |= n_ >> 1;      \
                            n_ |= n_ >> 2;      \
                            n_ |= n_ >> 4;      \
                            n_ |= n_ >> 8;      \
                            n_ |= n_ >> 16;     \
                            n_ |= n_ >> 32;     \
                            (__typeof__(v))(n_+1); \
                      })



// Fast Hash table for key K and value V.
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

        uint64_t hash;
        K key;
        V value;
    };

#define FASTHT_BAGSZ       4
#define FILLPCT            75

    // A bag is a contiguous array of nodes.
    struct bag {
        node *a[FASTHT_BAGSZ];
        bag() {
            for (int i = 0; i < FASTHT_BAGSZ; i++) a[i] = 0;
        };

        ~bag() {
            for (int i = 0; i < FASTHT_BAGSZ; i++) {
                node *x = a[i];
                if (x) delete x;
            }
        }

        // return true if bag is empty.
        bool is_empty() {
            bool r = true;
            for (int i = 0; i < FASTHT_BAGSZ; i++) r = r && (a[i]);
            return r;
        }
    };

    // A bucket holds the collision chain; each element in the chain
    // is a bag of "BAGSZ" contiguous nodes. In the future, we can
    // add RWlocks to each bucket for multi-threaded systems.
    // We can also add a second set of RWlocks for each bag.
    struct bucket {
        std::list<bag *> bags;
        uint32_t nitems = 0;      // number of items in this bucket
        uint32_t nbags  = 0;      // number of bags

        ~bucket() {
            while (!bags.empty()) {
                auto r = bags.front();
                bags.pop_front();
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
                : m_hasher(hasher), m_equal(equal) {

        if (n == 0) n = 128;
        setup(n);
    }

    fht() = delete;
    fht(const fht&) = delete;
    fht<K,V>& operator=(const fht<K, V>& that) = delete;

    // Move constructor
    fht(fht&& o)
       : m_size(o.m_size), m_seed(o.m_seed),
         m_hasher(o.m_hasher), m_equal(o.m_equal),
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


    // insert only if not already present; return if already
    // present, false otherwise.
    std::pair<bool, V*> probe(const K& key, const V& value) {
        uint64_t h = m_hasher(key);
        bucket *b  = hash(m_tab, h);

        auto r = insert(b, h, key, value);
        if (r.first) return std::make_pair(true, &r.second->value);

        if (b->nitems == 1) {
            m_fill++;
            if (((m_fill * 100) / (1 + m_nodes)) > FILLPCT) {
                m_splits++;
                b = resize();
                b = hash(b, h);
            }
        }

        if (b->nbags  > m_bagmax)   m_bagmax   = b->nbags;
        if (b->nitems > m_chainmax) m_chainmax = b->nitems;

        return std::make_pair(false, &r.second->value);
    }

    // Find 'key' in table; if key is found, then return <true,
    // value>. Otherwise, return <false, 0>
    std::pair<bool, V*> find(const K& key) {
        return xfind(key, false);
    }


    // Remove if key exists. Retval:
    //   <true, value> if key exists
    //   <false, undef> if key doesn't exist.
    std::pair<bool, V*> remove(const K& key) {
        return xfind(key, true);
    }


protected:

    // given a key-hash, return its bucket.
    bucket *hash(bucket *tab, uint64_t k) {
        uint64_t i = (k ^ m_seed) & (m_size-1);
        return &tab[i];
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
            while (!o->bags.empty()) {
                bag *&g = o->bags.front();
                o->bags.pop_front();

                for (int i = 0; i < FASTHT_BAGSZ; i++) {
                    node *p = g->a[i];

                    if (p) {
                        bucket *x  = hash(b, p->hash);

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


    // find the key and delete it if delit is true.
    std::pair<bool, V*> xfind(const K& key, bool delit) {
        uint64_t h = m_hasher(key);
        bucket *b  = hash(m_tab, h);
        node **px  = 0;
        bag  *bg   = 0;

        for (bag*& g: b->bags) {
            px = &g->a[0];

            node *x = *px;

#define SRCH(x) do { \
                    if (x && x->hash == h \
                          && m_equal(key, x->key)) { \
                        bg = g;     \
                        goto found; \
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

    found:
        node *x = *px;
        auto r  = std::make_pair<bool, V*>(true, &x->value);

        if (delit) {
            *px = 0;
            delete x;

            if (bg->is_empty()) {
                b->bags.remove(bg);
                b->nbags--;
            }

            if (--b->nitems == 0) m_fill--;
            m_nodes--;
        }

        return r;
    }

    // Setup the table
    void setup(size_t n) {
        n = NEXTPOW2(n);

        m_size = n;
        m_tab  = new bucket[m_size];

        arc4random_buf(&m_seed, sizeof m_seed);
    }

    // Insert if not present
    // If present: return pointer to its node
    // If not present: allocate new node and return pointer to it
    std::pair<bool, node *> insert(bucket *b, uint64_t hash, const K& key, const V& val) {
        node **pos = 0;

        for (bag*& g: b->bags) {
            node **px = &g->a[0];
            node *x   = *px;

#define FIND(x) do {                \
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
            bag *g = new bag;
            pos    = &g->a[0];

            b->bags.push_front(g);
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

        for (bag*& g: b->bags) {
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

        bag *g = new bag;

        g->a[0] = n;
        b->bags.push_front(g);
        b->nbags++;
        b->nitems++;
    }

};



#if 0

typedef  fht<std::string, void *> cache;


void *
cache_find(cache *c, const std::string& k)
{
    auto r = c->find(k);

    return r.first ? r.second : 0;
}

void *
cache_add(cache *c, const std::string& k, void *v)
{
    auto r = c->probe(k, v);
    return r.first ? r.second : 0;
}

void *
cache_del(cache *c, const std::string& k, void *v)
{
    auto r = c->remove(k);
    return r.first ? r.second : 0;
}
#endif
