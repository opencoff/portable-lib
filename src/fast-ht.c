/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * fast-ht.c - Fast hashtable with array based buckets
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
 * o Each hash bucket points to a "bag"; and a bag contains 8 nodes
 *   in an array. Bags are linked in a list.
 * o the value ZERO for a hash-value is a marker to denote deleted
 *   nodes in the array.
 * o Bags once allocated are never freed.
 * o Callers must use a "good" hash function; the hash table relies
 *   on a 64-bit hash value as input.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>

#include "utils/utils.h"
#include "utils/fast-ht.h"

// Return value from an internal function
struct tuple
{
    bag *g;
    int  i;
};
typedef struct tuple tuple;

#define _d(z)       ((double)(z))

extern void arc4random_buf(void *, size_t);


// Mix function from Zi Long Tan's superfast hash
static inline uint64_t
_hashmix(uint64_t h)
{
    h ^= h >> 23;
    h *= 0x2127599bf4325c37ULL;
    h ^= h >> 47;
    return h;
}


/*
 * One round of Zi Long Tan's superfast hash
 */
static uint64_t
__hash(uint64_t hv, uint64_t n, uint64_t salt)
{
    const uint64_t m = 0x880355f21e6d1965ULL;

    hv ^= (_hashmix(hv) ^ salt);
    hv *= m;
    return (n-1) & hv;
}


// Given a ptr to key array and the corresponding bag,return the
// index for the key (also is the index for value) array.
#define _i(x, g)    (x - &(g)->hk[0])

// given a hash 'k', return its 8-bit fingerprint
static inline uint64_t
_hash_fp(uint64_t h)
{
    uint64_t z = 0;

    if ((z = 0xff & (h >> 58))) return z;
    if ((z = 0xff & (h >> 48))) return z;
    if ((z = 0xff & (h >> 40))) return z;
    if ((z = 0xff & (h >> 32))) return z;

    // If we can't find a non-zero byte, pretend it's this.
    return 0xff;
}


// find first zero of a 64-bit word and return the "byte number"
// within the word where the zero exists.
//
// This function counts ZERO from the msb. A return val of 8 implies
// no zero found.
// (Henry Warren's Hacker's Delight also counts ZERO from the MSB).
static inline uint64_t
__find_first_zero(uint64_t w)
{
    uint64_t y = (w & 0x7f7f7f7f7f7f7f7f) + 0x7f7f7f7f7f7f7f7f;
    y = ~(y | w | 0x7f7f7f7f7f7f7f7f);
    uint64_t z = __builtin_clzl(y) >> 3;

    return z;
}



static inline uint64_t
__update_fp(uint64_t oldfp, uint64_t fp, int slot)
{
    uint64_t x = (fp << ((8-slot-1) * 8));
    return oldfp | x;
}


// Insert (k, v) into bucket 'b' - but only if 'k' doesn't already
// exist in the bucket.
static void*
__insert(hb *b, uint64_t k, void *v)
{
    bag *g  = 0;
    int slot = 0;
    bag *bg  = 0;

#define FIND(x) do { \
                    if (*x == k) {              \
                        return g->hv[_i(x,g)];  \
                    }                           \
                    x++;                        \
                } while (0)

    // spread the top-byte of 'k' to all the remaining bytes of 'h_fp'
    uint64_t fp   = _hash_fp(k);
    uint64_t h_fp = fp * 0x0101010101010101;

    SL_FOREACH(g, &b->head, link) {
        uint64_t       n = __find_first_zero(g->fp ^ h_fp);
        uint64_t *x = &g->hk[0];

        // fp XOR key will zero-out the places where the fp may
        // occur in g->fp; so, if we find such a zero, high prob
        // that the key exists in this bag.

        if (likely(n < FASTHT_BAGSZ)) {
            // fast-path
            if (likely(k == x[n])) {
                return g->hv[n];
            }

            // slow path
            switch (FASTHT_BAGSZ) {
                default:                // fallthrough
                case 7: FIND(x);        // fallthrough
                case 6: FIND(x);        // fallthrough
                case 5: FIND(x);        // fallthrough
                case 4: FIND(x);        // fallthrough
                case 3: FIND(x);        // fallthrough
                case 2: FIND(x);        // fallthrough
                case 1: FIND(x);        // fallthrough
            }
        }

        // In case we need to find an empty slot for inserting this
        // kv pair, lets find one in this bag
        if (!bg && ((n = __find_first_zero(g->fp)) < FASTHT_BAGSZ)) {
            bg   = g;
            slot = n;
        }
    }

    if (!bg) {
        bg   = NEWZ(bag);
        slot = 0;
        b->bags++;
        SL_INSERT_HEAD(&b->head, bg, link);
    }


    bg->hk[slot] = k;
    bg->hv[slot] = v;
    bg->fp       = __update_fp(bg->fp, fp, slot);
    b->nodes++;
    return 0;
}


// Insert (k, v) into bucket 'b' quickly. In this case, we _know_
// that we are being called when we are re-sized. So, the items
// naturally don't exist in the new bucket. So, all we have to do is
// find the first free slot in the _first_ bag.
static void
__insert_quick(hb *b, uint64_t k, void *v)
{
    uint64_t fp = _hash_fp(k);
    bag *g      = SL_FIRST(&b->head);
    if (g) {
        uint64_t n = __find_first_zero(g->fp);
        if (n < FASTHT_BAGSZ) {
            assert(!g->hk[n]);
            g->hk[n] = k;
            g->hv[n] = v;
            g->fp    = __update_fp(g->fp, fp, n);
            b->nodes++;
            return;
        }
    }


    // Make a new bag and put our element there.
    g = NEWZ(bag);
    g->hk[0] = k;
    g->hv[0] = v;
    g->fp    = __update_fp(g->fp, fp, 0);
    b->bags++;
    b->nodes++;
    SL_INSERT_HEAD(&b->head, g, link);
}

// Find key 'hk' in 'b'; return the value in 't'.
// Return 1 if key is found, 0 otherwise.
static int
__findx(tuple *t, hb *b, uint64_t hk)
{
    bag *g = 0;

#define SRCH(x) do { \
                    if (likely(*x == hk)) { \
                        t->g = g;           \
                        t->i = _i(x,g);     \
                        return 1;           \
                    }                       \
                    x++;                    \
                } while(0)

    // spread the top-byte of 'k' to all the remaining bytes of 'h_fp'
    uint64_t   fp = _hash_fp(hk);
    uint64_t h_fp = fp * 0x0101010101010101;

    SL_FOREACH(g, &b->head, link) {
        uint64_t *x = &g->hk[0];

        // if this bag is empty - skip it.
        if (unlikely(g->fp == 0)) continue;

        // Maybe the key is in this bag?
        uint64_t n = __find_first_zero(g->fp ^ h_fp);
        if (unlikely(n >= FASTHT_BAGSZ)) continue;

        // Here:
        //  a) the key is in slot 'n'
        //  b) maybe there are other slots that also match (ie have
        //  "zeroes")


        // We try the fast path first - then just unroll
        if (likely(x[n] == hk)) {
                t->g = g;
                t->i = n;
                return 1;
        }

        // Slow path: check every slot!
        switch (FASTHT_BAGSZ) {
            default:            // fallthrough
            case 7: SRCH(x);    // fallthrough
            case 6: SRCH(x);    // fallthrough
            case 5: SRCH(x);    // fallthrough
            case 4: SRCH(x);    // fallthrough
            case 3: SRCH(x);    // fallthrough
            case 2: SRCH(x);    // fallthrough
            case 1: SRCH(x);    // fallthrough
        }
    }

    return 0;
}



/*
 * Double the hash buckets and redistribute all the nodes.
 */
static hb*
resize(ht* h)
{
    uint64_t salt;
    uint64_t n = h->n * 2;
    hb *b = NEWZA(hb, n),
       *o = h->b,
       *e = o + h->n;
    uint64_t maxbags = 0,
             maxn    = 0,
             fill    = 0;
    int i;

    arc4random_buf(&salt, sizeof salt);

    for (; o < e; o++) {
        bag *g, *tmp;

        SL_FOREACH_SAFE(g, &o->head, link, tmp) {
            for (i = 0; i < FASTHT_BAGSZ; i++) {
                uint64_t p = g->hk[i];

                if (p) {
                    uint64_t j = __hash(p, n, salt);
                    hb *x      = b+j;

                    __insert_quick(x, p, g->hv[i]);
                    if (x->bags > maxbags) maxbags = x->bags;
                    if (x->nodes > maxn)   maxn    = x->nodes;
                    if (x->nodes == 1)     fill++;
                }
            }

            DEL(g);
        }
    }

    DEL(h->b);

    h->salt   = salt;
    h->n      = n;
    h->b      = b;
    h->bagmax = maxbags;
    h->maxn   = maxn;
    h->fill   = fill;
    return h->b;
}


/*
 * Public API
 */


/*
 * Initialize a pre-allocated instance.
 */
ht*
ht_init(ht* h, uint32_t size, uint32_t maxfill)
{
    if (!size) size = 128;
    else if (size & (size-1)) size = NEXTPOW2(size);

    if (!maxfill) maxfill = FILLPCT;

    memset(h, 0, sizeof *h);

    h->n = size;
    h->b = NEWZA(hb, h->n);
    h->maxfill = maxfill;
    arc4random_buf(&h->salt, sizeof h->salt);

    return h;
}

/*
 * Finalize a pre-allocated instance.
 */
void
ht_fini(ht* h)
{
    hb *b = h->b,
       *e = b + h->n;

    for (; b < e; b++) {
        bag *g, *next;
        SL_FOREACH_SAFE(g, &b->head, link, next) {
            DEL(g);
        }
    }

    DEL(h->b);
    memset(h, 0, sizeof *h);
}


/*
 * Allocate a new hash table instance and initialize it.
 */
ht*
ht_new(uint32_t size, uint32_t maxfill)
{
    ht* h = NEWZ(ht);

    return ht_init(h, size, maxfill);
}


/*
 * Finalize a dynamically allocated hash table instance and free
 * memory.
 */
void
ht_del(ht* h)
{
    ht_fini(h);
    DEL(h);
}


/*
 * Insert if not already present.
 *
 * Return existing value if present, 0 otherwise.
 */
void*
ht_probe(ht* h, uint64_t k, void* v)
{
    hb   *b  = &h->b[__hash(k, h->n, h->salt)];
    void *nv = __insert(b, k, v);

    if (nv) return nv;

    h->nodes++;

    // time to split?
    if (b->nodes == 1) {
        uint64_t fpct = (++h->fill * 100)/h->n;

        if (fpct > h->maxfill) {
            h->splits++;
            b = resize(h);
            b = &b[__hash(k, h->n, h->salt)];
        }
    }

    if (b->bags  > h->bagmax) h->bagmax = b->bags;
    if (b->nodes > h->maxn)   h->maxn   = b->nodes;

    return 0;
}


/*
 * Find hash value 'k'; return True if found, False otherwise
 *
 * If found, optionally return the value in 'p_ret'
 */
int
ht_find(ht* h, uint64_t k, void** p_ret)
{
    tuple x;
    hb *b = &h->b[__hash(k, h->n, h->salt)];

    if (__findx(&x, b, k)) {
        bag *g = x.g;

        if (p_ret) *p_ret = g->hv[x.i];
        return 1;
    }

    return 0;
}


/*
 * Replace an existing value.
 *
 * Return true if found, false otherwise.
 */
int
ht_replace(ht* h, uint64_t k, void* val)
{
    tuple x;
    hb *b = &h->b[__hash(k, h->n, h->salt)];

    if (__findx(&x, b, k)) {
        bag *g = x.g;

        g->hv[x.i] = val;
        return 1;
    }

    return 0;
}


/*
 * Remove hash value 'k'; return True if found, False otherwise
 * If found, optionally return the value in 'p_ret'
 */
int
ht_remove(ht* h, uint64_t k, void** p_ret)
{
    tuple x;
    hb *b = &h->b[__hash(k, h->n, h->salt)];

    if (__findx(&x, b, k)) {
        bag *g   = x.g;
        int slot = x.i;

        // Erase this FP - by using 0x0 as the "new" FP
        g->fp = __update_fp(g->fp, 0x00, slot);

        if (p_ret) *p_ret = g->hv[slot];

        g->hk[slot] = 0;
        g->hv[slot] = 0;

        b->nodes--;
        h->nodes--;
        return 1;
    }

    return 0;
}


void
ht_dump(ht *h, const char *start, void (*dump)(const char *str, size_t n))
{
    char buf[256];

    assert(dump);

#define pr(a, ...) do { \
    size_t n = snprintf(buf, sizeof buf, a, __VA_ARGS__); \
    (*dump)(buf, n); \
} while (0)


    pr("%s: ht %p: %" PRIu64 " elems; %" PRIu64 "/%" PRIu64 " buckets filled\n",
            start, h, h->nodes, h->n, h->fill);

    for (uint64_t i = 0; i < h->n; i++) {
        hb *b = &h->b[i];
        pr("[%" PRIu64 "]: %u elems in %u bags\n", i, b->nodes, b->bags);

        bag *g   = 0,
            *tmp = 0;

        SL_FOREACH_SAFE(g, &b->head, link, tmp) {
            pr("  bag %p: fp %#16.16" PRIx64 ":\n", g, g->fp);
            for (int j = 0; j < FASTHT_BAGSZ; j++) {
                pr("     [%#16.16" PRIx64 ", %p]\n", g->hk[j], g->hv[j]);
            }
        }
    }
}

/* EOF */
