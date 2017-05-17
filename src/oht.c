/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * oht.c - Open Addressed hash table.
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
 * o Callers must use a "good" hash function; the hash table relies
 *   on a 64-bit hash value as input.
 * o Each hash bucket points to a "bag"; and a bag contains 8 nodes
 *   in an array.
 * o the value ZERO for a hash-value is a marker to denote deleted
 *   nodes.
 * o When bags are full, we use a secondary, linearly probed table.
 *   The linear-probed table starts off with 64 entries and grows as
 *   "n/16" (where 'n' is the size of the parent table).
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include "utils/utils.h"
#include "utils/oht.h"

#define _d(z)       ((double)(z))

extern void arc4random_buf(void *, size_t);

static inline uint64_t
__hash(uint64_t hv, uint64_t n, uint64_t salt)
{
    return (hv ^ salt) & (n-1);
}


static inline oht *
__init(oht *h, uint64_t n)
{
    memset(h, 0, sizeof *h);

    h->n = n;
    h->novf = n / 16;
    if (h->novf == 0) h->novf = 128;    // 128 =  1024 / HB

    h->b   = NEWZA(ohb, h->n);
    h->ovf = NEWZA(ohn, h->novf);
    arc4random_buf(&h->rand, sizeof h->rand);
    return h;
}



// Return 1 if already present, 0 if not found
static int
__insert(oht *t, ohn *p)
{
    ohb *b   = &t->b[__hash(p->h, t->n, t->rand)];
    ohn *x   = &b->a[0];
    ohn *pos = 0;

#define ISRCH(x)    do { \
                        if (x->h == p->h)      { return 1; } \
                        if (x->h == 0 && !pos) { pos = x;  } \
                        x++;                                 \
                    } while (0)

    ISRCH(x);
    ISRCH(x);
    ISRCH(x);
    ISRCH(x);

    ISRCH(x);
    ISRCH(x);
    ISRCH(x);
    ISRCH(x);

    // Overflow buckets
    x = &t->ovf[__hash(p->h, t->novf, t->rand)];
    ohn *e = t->ovf + t->novf;
    for (; x < e; x++) {
        if (x->h == p->h)      { return 1; }
        if (x->h == 0 && !pos) { pos = x;  }
    }

    // We _must_ find a free slot at some point!
    assert(pos);
    *pos = *p;

    t->nodes++;
    if (pos >= t->ovf && pos < e) {
        t->povf++;
    } else {
        b->n++;

        if (b->n == 1)      t->fill++;
        if (b->n > t->maxn) t->maxn = b->n;
    }
    return 0;
}



static oht*
resize(oht* h)
{
    oht z;

    __init(&z, h->n * 2);

#define RSRCH(p)    do { \
                        if (p->h) __insert(&z, p); \
                        p++;                       \
                    } while (0)

    ohb *o = h->b,
        *e = o + h->n;
    for (; o < e; o++) {
        ohn *p = &o->a[0];

        RSRCH(p);
        RSRCH(p);
        RSRCH(p);
        RSRCH(p);

        RSRCH(p);
        RSRCH(p);
        RSRCH(p);
        RSRCH(p);
    }

    ohn * a = h->ovf,
        * b = a + h->novf;

    for (; a < b; a++) {
        if (a->h) __insert(&z, a);
    }

    DEL(h->b);
    DEL(h->ovf);

    *h = z;
    return h;
}





oht*
oht_init(oht* h, uint32_t nlog2)
{
    if (!nlog2) nlog2 = 10;

    return __init(h, 1 << nlog2);
}

void
oht_fini(oht* h)
{
    DEL(h->b);
    DEL(h->ovf);
}


oht*
oht_new(uint32_t nlog2)
{
    oht* h = NEWZ(oht);

    return oht_init(h, nlog2);
}

void
oht_del(oht* h)
{
    oht_fini(h);
    DEL(h);
}


/*
 * Insert if not already present.
 */
int
oht_probe(oht* h, uint64_t hv, void* v)
{
    ohn x     = { .h = hv, .v = v };

    if (__insert(h, &x)) return 1;

    // time to split?
    if ( ((h->fill * 100) / (1 + h->n)) > FILLPCT) {
        resize(h);
        h->splits++;
    }

    return 0;
}



// Find 'hv' and put the found "value" in 'p_ret'. If 'rm' is true,
// delete the node we find. 
// Return 1 if found, 0 otherwise
static inline int
__find(oht *h, uint64_t hv, void **p_ret, int rm)
{
    assert(p_ret);
    ohb * b = &h->b[__hash(hv, h->n, h->rand)];
    ohn * x;

#define SRCH(x) do { \
                    if (x->h == hv) goto found; \
                    x++;    \
                } while(0)

    x = &b->a[0];
    SRCH(x);
    SRCH(x);
    SRCH(x);
    SRCH(x);

    SRCH(x);
    SRCH(x);
    SRCH(x);
    SRCH(x);

    x = &h->ovf[__hash(hv, h->novf, h->rand)];
    ohn *e = h->ovf + h->novf;
    for (; x < e; x++) {
        if (x->h == hv) goto found;
    }
    return 0;

found:
    if (p_ret) *p_ret = x->v;

    if (rm) {
        x->h = 0;
        x->v = 0;
    }
    return 1;
}



// Return true on success, false if not found
int
oht_find(oht* h, uint64_t hv, void** p_ret)
{
    return __find(h, hv, p_ret, 0);
}


int
oht_remove(oht* h, uint64_t hv, void** p_ret)
{
    return __find(h, hv, p_ret, 1);
}


/* EOF */
