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

static inline uint64_t
__hash(uint64_t hv, uint64_t n, uint64_t salt)
{
    return (hv ^ salt) & (n-1);
}



static void
free_nodes(hb * b, uint64_t n)
{
    hb *e = b + n;

    for (; b < e; b++) {
        bag *g, *n;
        SL_FOREACH_SAFE(g, &b->head, link, n) {
            DEL(g);
        }
    }
}

// Insert node 'p' into bucket 'b'
static int
__insert(hb *b, hn *p)
{
    bag *g;
    hn  *pos = 0;

#define FIND(x) do { \
                    if (likely(x->h == p->h)) { return 1; } \
                    if (x->h == 0 && !pos)    { pos = x; }  \
                    x++;                                    \
                } while (0)

    SL_FOREACH(g, &b->head, link) {
        hn *x = &g->a[0];

        FIND(x);
        FIND(x);
        FIND(x);
        FIND(x);
        FIND(x);
        FIND(x);
        FIND(x);
        FIND(x);
    }

    if (!pos) {
        g   = NEWZ(bag);
        pos = &g->a[0];
        SL_INSERT_HEAD(&b->head, g, link);
        b->bags++;
    }

    *pos = *p;
    b->n++;
    return 0;
}


// Find hv in 'h'; return the value in p_ret. If zero is true, also
// clear the node.
static int
__findx(ht *h, uint64_t hv, void** p_ret, int zero)
{
    static const hn zn = { .h = 0, .v = 0 };
    hb * b = &h->b[__hash(hv, h->n, h->rand)];

    bag *g;
    hn  *x = 0;

#define SRCH(x) do { \
                    if (likely(x->h == hv)) goto found; \
                    x++;    \
                } while(0)

    SL_FOREACH(g, &b->head, link) {
        x = &g->a[0];

        SRCH(x);
        SRCH(x);
        SRCH(x);
        SRCH(x);
        SRCH(x);
        SRCH(x);
        SRCH(x);
        SRCH(x);
    }
    return 0;

found:
    if (p_ret) *p_ret = x->v;
    if (zero)      *x = zn;
    return 1;
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
            for (i = 0; i < HB; i++) {
                hn *p = &g->a[i];
                if (p->h) {
                    uint64_t j = __hash(p->h, n, salt);
                    hb *x      = b+j;
                    __insert(x, p);
                    if (x->bags > maxbags) maxbags = x->bags;
                    if (x->n    > maxn)    maxn    = x->n;
                    if (x->n == 1)         fill++;
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
ht_init(ht* h, uint32_t nlog2)
{
    if (!nlog2) nlog2 = 10;

    memset(h, 0, sizeof *h);

    h->n = 1 << nlog2;
    h->b = NEWZA(hb, h->n);
    arc4random_buf(&h->rand, sizeof h->rand);

    //printf("ht_init: nlog2 %lu, n %llu total %llu\n", nlog2, n, m);
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
ht_new(uint32_t nlog2)
{
    ht* h = NEWZ(ht);

    return ht_init(h, nlog2);
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
    if (b->n == 1) {
        h->fill++;

        if ( ((h->fill * 100) / (1 + h->n)) > FILLPCT) {
            hb *base = resize(h);
            h->splits++;
            b = &base[__hash(hv, h->n, h->rand)];
        }
    }

    if (b->bags > h->bagmax) h->bagmax = b->bags;
    if (b->n    > h->maxn)   h->maxn   = b->n;

    return 0;
}



/*
 * Find hash value 'hv'; return True if found, False otherwise
 */
int
ht_find(ht* h, uint64_t hv, void** p_ret)
{
    return __findx(h, hv, p_ret, 0);
}


/*
 * Remove hash value 'hv'; return True if found, False otherwise
 */
int
ht_remove(ht* h, uint64_t hv, void** p_ret)
{
    return __findx(h, hv, p_ret, 1);
}


/* EOF */
