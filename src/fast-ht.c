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

/*
 * XXX Do we need anything more complex than this?
 */
static uint64_t
__hash(uint64_t hv, uint64_t n, uint64_t salt)
{
    return (n-1) & (hv ^ salt);
}


// Given a ptr to key array and the corresponding bag,return the
// index for the key (also is the index for value) array.
#define _i(x, g)    (x - &(g)->hk[0])


// Insert (k, v) into bucket 'b' - but only if 'k' doesn't already
// exist in the bucket.
static int
__insert(hb *b, uint64_t k, void *v)
{
    bag *g;
    void    **hv = 0;
    uint64_t *hk = 0;

#define FIND(x) do { \
                    if (*x == k) { return 1; }  \
                    if (0 == *x && !hk) {       \
                        hv = &g->hv[_i(x,g)];   \
                        hk = x;                 \
                    }                           \
                    x++;                        \
                } while (0)

    SL_FOREACH(g, &b->head, link) {
        uint64_t *x = &g->hk[0];

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

    if (!hk) {
        g  = NEWZ(bag);
        hk = &g->hk[0];
        hv = &g->hv[0];
        b->bags++;
        SL_INSERT_HEAD(&b->head, g, link);
    }

    *hk = k;
    *hv = v;
    b->nodes++;
    return 0;
}


// Insert (k, v) into bucket 'b' quickly. In this case, we _know_
// that we are being called when we are re-sized. So, the items
// naturally don't exist in the new bucket. So, all we have to do is
// find the first free slot.
static int
__insert_quick(hb *b, uint64_t k, void *v)
{
    bag *g;
    void    **hv = 0;
    uint64_t *hk = 0;

#define PUT(x) do {     \
                    if (*x == 0) {  \
                        hk = x;     \
                        hv = &g->hv[_i(x,g)]; \
                        goto done;  \
                    }               \
                    x++; \
               } while(0)

    SL_FOREACH(g, &b->head, link) {
        uint64_t *x = &g->hk[0];

        switch (FASTHT_BAGSZ) {
            default:            // fallthrough
            case 8: PUT(x);     // fallthrough
            case 7: PUT(x);     // fallthrough
            case 6: PUT(x);     // fallthrough
            case 5: PUT(x);     // fallthrough
            case 4: PUT(x);     // fallthrough
            case 3: PUT(x);     // fallthrough
            case 2: PUT(x);     // fallthrough
            case 1: PUT(x);     // fallthrough
        }
    }

    // Make a new bag and put our element there.
    g   = NEWZ(bag);
    hk = &g->hk[0];
    hv = &g->hv[0];
    b->bags++;
    SL_INSERT_HEAD(&b->head, g, link);

done:
    *hk = k;
    *hv = v;
    b->nodes++;
    return 0;
}

// Find key 'hv' in 'h'; return the value in 't'.
// Return 1 if key is found, 0 otherwise.
static int
__findx(tuple *t, hb *b, uint64_t hv)
{
    bag *g;

#define SRCH(x) do { \
                    if (likely(*x == hv)) { \
                        t->g = g;           \
                        t->i = _i(x,g);     \
                        return 1;           \
                    }                       \
                    x++;                    \
                } while(0)

    SL_FOREACH(g, &b->head, link) {
        uint64_t *x = &g->hk[0];

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
 * Insert if not already present; return true if present, false otherwise.
 */
int
ht_probe(ht* h, uint64_t k, void* v)
{
    hb *b = &h->b[__hash(k, h->n, h->salt)];

    if (__insert(b, k, v)) return 1;

    h->nodes++;

    // time to split?
    if (b->nodes == 1) {
        uint64_t fpct = (++h->fill * 100)/h->n;

        //printf("fill %d/%d (%d ceil %d)%s\n", 
        //      h->fill, h->n, fpct, h->maxfill, fpct > h->maxfill ? " +split" : "");

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
 * Remove hash value 'k'; return True if found, False otherwise
 * If found, optionally return the value in 'p_ret'
 */
int
ht_remove(ht* h, uint64_t k, void** p_ret)
{
    tuple x;
    hb *b = &h->b[__hash(k, h->n, h->salt)];

    if (__findx(&x, b, k)) {
        bag *g = x.g;

        if (p_ret) *p_ret = g->hv[x.i];

        g->hk[x.i] = 0;
        g->hv[x.i] = 0;

        b->nodes--;
        h->nodes--;
        return 1;
    }

    return 0;
}


/* EOF */
