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


#define _d(z)       ((double)(z))

extern void arc4random_buf(void *, size_t);

static uint64_t
__hash(uint64_t hv, uint64_t n, uint64_t salt)
{
    return (n-1) & (hv ^ salt);
}



static void
free_nodes(hb * b, uint64_t n)
{
    hb *e = b + n;

    for (; b < e; b++) {
        bag *g, *next;
        SL_FOREACH_SAFE(g, &b->head, link, next) {
            DEL(g);
        }
    }
}

// Insert node 'p' into bucket 'b' - but only if 'p' doesn't already
// exist in the bucket.
static int
__insert(hb *b, hn *p)
{
    bag *g;
    hn  *pos = 0;

#define FIND(x) do { \
                    if (x->h == p->h) { return 1; } \
                    if (!pos) {                 \
                        if (x->h == 0) pos = x; \
                    }                           \
                    x++;                        \
                } while (0)

    SL_FOREACH(g, &b->head, link) {
        hn *x = &g->a[0];

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
        g   = NEWZ(bag);
        pos = &g->a[0];
        SL_INSERT_HEAD(&b->head, g, link);
        b->bags++;
    }

    *pos = *p;
    b->nodes++;
    return 0;
}


// Insert 'p' into bucket 'b' quickly.
static int
__insert_quick(hb *b, hn *p)
{
    bag *g;
    hn  *pos = 0;

#define PUT(x) do {     \
                    if (x->h == 0) { pos = x; goto done; } \
                    x++; \
               } while(0)

    SL_FOREACH(g, &b->head, link) {
        hn *x = &g->a[0];

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
    pos = &g->a[0];
    SL_INSERT_HEAD(&b->head, g, link);
    b->bags++;

done:
    *pos = *p;
    b->nodes++;
    return 0;
}

// Find hv in 'h'; return the value in p_ret. If zero is true, also
// clear the node.
static hn *
__findx(hb *b, uint64_t hv)
{
    bag *g;

#define SRCH(x) do { \
                    if (likely(x->h == hv)) return x;\
                    x++;                            \
                } while(0)

    SL_FOREACH(g, &b->head, link) {
        hn *x = &g->a[0];

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
                hn *p = &g->a[i];

                if (p->h) {
                    uint64_t j = __hash(p->h, n, salt);
                    hb *x      = b+j;

                    __insert_quick(x, p);
                    if (x->bags > maxbags) maxbags = x->bags;
                    if (x->nodes > maxn)   maxn    = x->nodes;
                    if (x->nodes == 1)     fill++;
                }
            }

            DEL(g);
        }
    }

    DEL(h->b);

    h->rand   = salt;
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
    arc4random_buf(&h->rand, sizeof h->rand);

    return h;
}

/*
 * Finalize a pre-allocated instance.
 */
void
ht_fini(ht* h)
{
    free_nodes(h->b, h->n);
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
 */
int
ht_probe(ht* h, uint64_t hv, void* v)
{
    hb *b = &h->b[__hash(hv, h->n, h->rand)];
    hn x  = { .h = hv, .v = v };

    if (__insert(b, &x)) return 1;

    h->nodes++;

    // time to split?
    if (b->nodes == 1) {
        uint64_t fpct = (++h->fill * 100)/h->n;

        //printf("fill %d/%d (%d ceil %d)%s\n", 
        //      h->fill, h->n, fpct, h->maxfill, fpct > h->maxfill ? " +split" : "");

        if (fpct > h->maxfill) {
            h->splits++;
            b = resize(h);
            b = &b[__hash(hv, h->n, h->rand)];
        }
    }

    if (b->bags  > h->bagmax) h->bagmax = b->bags;
    if (b->nodes > h->maxn)   h->maxn   = b->nodes;

    return 0;
}



/*
 * Find hash value 'hv'; return True if found, False otherwise
 */
int
ht_find(ht* h, uint64_t hv, void** p_ret)
{
    hb *b = &h->b[__hash(hv, h->n, h->rand)];
    hn *x = __findx(b, hv);

    if (x) {
        if (p_ret) *p_ret = x->v;
        return 1;
    }

    return 0;
}


/*
 * Remove hash value 'hv'; return True if found, False otherwise
 */
int
ht_remove(ht* h, uint64_t hv, void** p_ret)
{
    static const hn zn = { .h = 0, .v = 0 };
    hb *b = &h->b[__hash(hv, h->n, h->rand)];
    hn *x = __findx(b, hv);

    if (x) {
        if (p_ret) *p_ret = x->v;
        *x = zn;
        return 1;
    }

    return 0;
}


/* EOF */
