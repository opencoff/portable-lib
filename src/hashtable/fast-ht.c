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
#include <unistd.h>
#include <sys/random.h>

#include "utils/utils.h"
#include "utils/fast-ht.h"
#include "utils/nospec.h"

// Return value from an internal function
struct tuple
{
    bag *g;
    int  i;
};
typedef struct tuple tuple;

#define _d(z)       ((double)(z))


// get me a random 64-bit number
static inline uint64_t
rand64()
{
    uint64_t v;

    getentropy(&v, sizeof v);
    return v;
}


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


static void *
__alloc(size_t n)
{
    void *ptr = 0;
    int     r = posix_memalign(&ptr, CACHELINE_SIZE, n);
    assert(r == 0);
    assert(ptr);

    return memset(ptr, 0, n);
}

#define __NEWZ(ty) ({                           \
                ty *_x = __alloc(sizeof(ty));   \
                _x;                             \
                })

#define __NEWZA(ty, n) ({                           \
                ty *_x = __alloc((n) * sizeof(ty)); \
                _x;                                 \
                })


static inline int
__find_empty_slot(uint64_t *x)
{

#define __FIND2(x,y) do {   \
            if (*x == 0) {  \
                return y;   \
            }               \
            x++;            \
            y++;            \
        } while (0)

    int i = 0;
    switch (FASTHT_BAGSZ) {
        case 8: __FIND2(x, i);  // fallthrough
        case 7: __FIND2(x, i);  // fallthrough
        case 6: __FIND2(x, i);  // fallthrough
        case 5: __FIND2(x, i);  // fallthrough
        case 4: __FIND2(x, i);  // fallthrough
        case 3: __FIND2(x, i);  // fallthrough
        case 2: __FIND2(x, i);  // fallthrough
        case 1: __FIND2(x, i);  // fallthrough
        default:
                break;
    }
    return -1;
}

// Insert (k, v) into bucket 'b' - but only if 'k' doesn't already
// exist in the bucket.
static void*
__insert(hb *b, uint64_t k, void *v)
{
    bag *g   = 0;
    int slot = 0;
    bag *bg  = 0;

#define FIND(x,y) do {                          \
                    if (likely(*x == k)) {      \
                        return *y;              \
                    }                           \
                    x++;                        \
                    y++;                        \
                } while (0)

    SL_FOREACH(g, &b->head, link) {
        uint64_t *x = &g->hk[0];
        void    **y = &g->hv[0];
        switch (FASTHT_BAGSZ) {
            case 8: FIND(x, y);        // fallthrough
            case 7: FIND(x, y);        // fallthrough
            case 6: FIND(x, y);        // fallthrough
            case 5: FIND(x, y);        // fallthrough
            case 4: FIND(x, y);        // fallthrough
            case 3: FIND(x, y);        // fallthrough
            case 2: FIND(x, y);        // fallthrough
            case 1: FIND(x, y);        // fallthrough
            default:
                    break;
        }

        if (!bg) {
            x = &g->hk[0];
            if ((slot = __find_empty_slot(x)) >= 0) {
                bg = g;
            }
        }
    }

    if (!bg) {
        bg   = __NEWZ(bag);
        slot = 0;
        b->bags++;
        SL_INSERT_HEAD(&b->head, bg, link);
    }

_end:
    bg->hk[slot] = k;
    bg->hv[slot] = v;
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
    bag *g      = SL_FIRST(&b->head);
    if (g) {
        uint64_t *x = &g->hk[0];
        int j       = __find_empty_slot(x);
        if (j >= 0) {
            assert(!g->hk[j]);
            g->hk[j] = k;
            g->hv[j] = v;
            b->nodes++;
            return;
        }
    }

    // Make a new bag and put our element there.
    g = __NEWZ(bag);
    g->hk[0] = k;
    g->hv[0] = v;
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

#define SRCH(x,y) do {                      \
                    if (likely(*x == hk)) { \
                        t->g = g;           \
                        t->i = i;           \
                        return 1;           \
                    }                       \
                    x++;                    \
                    i++;                    \
                } while(0)

    SL_FOREACH(g, &b->head, link) {
        uint64_t *x = &g->hk[0];
        uint64_t  i = 0;
        switch (FASTHT_BAGSZ) {
            case 8: SRCH(x, i);    // fallthrough
            case 7: SRCH(x, i);    // fallthrough
            case 6: SRCH(x, i);    // fallthrough
            case 5: SRCH(x, i);    // fallthrough
            case 4: SRCH(x, i);    // fallthrough
            case 3: SRCH(x, i);    // fallthrough
            case 2: SRCH(x, i);    // fallthrough
            case 1: SRCH(x, i);    // fallthrough
            default:
                    break;
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
    uint64_t salt = rand64();
    uint64_t n    = h->n * 2;

    hb *b = __NEWZA(hb, n),
       *o = h->b,
       *e = o + h->n;
    uint64_t maxbags = 0,
             maxn    = 0,
             fill    = 0;


    for (; o < e; o++) {
        bag *g, *tmp;

        SL_FOREACH_SAFE(g, &o->head, link, tmp) {
            int i = 0;
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
    h->b = __NEWZA(hb, h->n);

    h->maxfill = maxfill;
    h->salt    = rand64();

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
    ht* h = __NEWZ(ht);

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
    uint64_t hh  =__hash(k, h->n, h->salt);
    hb   *b  = &h->b[hh];
    void *nv = __insert(b, k, v);

    if (nv) return nv;

    h->nodes++;

    // time to split?
    if (b->nodes == 1) {
        uint64_t fpct = (++h->fill * 100)/h->n;

        if (fpct > h->maxfill) {
            // h will be updated by resize; we don't want clever
            // compilers from caching h->n and h->salt.
            OPTIMIZER_HIDE_VAR(h);

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

        if (p_ret) *p_ret = g->hv[slot];

        g->hk[slot] = 0;
        g->hv[slot] = 0;
        b->nodes--;
        h->nodes--;
        return 1;
    }

    return 0;
}


/*
 * Dump hash table via caller provided output function.
 */
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
            start, h, h->nodes, h->fill, h->n);

    for (uint64_t i = 0; i < h->n; i++) {
        hb *b = &h->b[i];
        pr("[%" PRIu64 "]: %u elems in %u bags\n", i, b->nodes, b->bags);

        bag *g   = 0,
            *tmp = 0;

        int bn = 0;
        SL_FOREACH_SAFE(g, &b->head, link, tmp) {
            pr("  bag.%d %p:\n", bn, g);
            for (int j = 0; j < FASTHT_BAGSZ; j++) {
                pr("     [%#16.16" PRIx64 ", %p]\n", g->hk[j], g->hv[j]);
            }
            bn++;
        }
    }
}

/* EOF */
