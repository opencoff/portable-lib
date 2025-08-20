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
 * o Each hash bucket points to a "bag" - a bag is exactly two
 *   cache ines wide:
 *
 *      - First cacheline: 7 keys
 *      - Second cacheline: 7 vals
 *
 * o The first word of first cacheline is a "fingerprint": the
 *   low-byte of each of the 7 keys are stored in the fingerprint.
 *
 * o The last byte of the fingerprint is a "control" byte: Each bit
 *   indicates if a slot is occupied (0) or free (1).
 *
 * o Bags once allocated are never freed.
 *
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

#include "fast/simd.h"
#include "utils/utils.h"
#include "utils/fast-ht.h"
#include "utils/nospec.h"

// Return value from an internal function
struct tuple
{
    bag *bag;
    int  slot;
    void *val;
};
typedef struct tuple tuple;

#define _d(z)       ((double)(z))

// alloc and zero an instance of type 'ty'
#define __NEWZ(ty) ({                           \
                ty *_x = __alloc(sizeof(ty));   \
                _x;                             \
                })

// make an 'n' element array of type 'ty'
#define __NEWZA(ty, n) ({                           \
                ty *_x = __alloc((n) * sizeof(ty)); \
                _x;                                 \
                })


// extract the individual fingerprints
#define __fp(fp)       ((fp) & ((_U64(1) << 56)-1))

#define __h2(fp)        ((fp) & 0xff)

// extract the control bits
#define __control(fp)  _U8(0x7f & (fp) >> 56)

// Create a new fingerprint from constituent bits
#define __mkfp(h2, ctrl) (_U64(h2) | (_U64(ctrl) << 56))


// fast/simd.h exports this symbol if we are not on arm64 or amd64
#ifdef __NO_SIMD__

// software fallback to find matches using bit tricks
static inline uint8_t
__find_matches(uint64_t fp_h2, uint8_t h2)
{
    uint64_t splat = 0x0101010101010101ULL * h2;
    uint64_t m = (fp_h2 ^ splat);
    uint64_t x = ((m - 0x0101010101010101ULL) & ~m) & 0x8080808080808080ULL;

    /* Now use multiplication to gather bits */
    return 0x7f & ((x * 0x0002040810204081ULL) >> 56);
}

#else

// Fast version using SIMD on arm64 and amd64
static inline uint8_t
__find_matches(uint64_t fp_h2, uint8_t h2)
{
    simd_vec128_t h2v = SIMD_SET_EPI64X(0, fp_h2);
    simd_vec128_t qv  = SIMD_SET1_EPI8(h2);
    return 0x7f & SIMD_MOVEMASK_EPI8(SIMD_CMPEQ_EPI8(h2v, qv));
}

#endif  // __NO_SIMD__


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
static inline uint64_t
__hash(uint64_t hv, uint64_t n, uint64_t salt)
{
    const uint64_t m = 0x880355f21e6d1965ULL;

    hv ^= (_hashmix(hv) ^ salt);
    hv *= m;
    return (n-1) & hv;
}


static inline void *
__alloc(size_t n)
{
    void *ptr = 0;
    int     r = posix_memalign(&ptr, CACHELINE_SIZE, n);
    assert(r == 0);
    assert(ptr);

    return memset(ptr, 0, n);
}


// Update the fingerprint to show slot as now occupied.
// NB: 0 is occupied, 1 is free.
static inline uint64_t
__update_fp(uint64_t fp, uint64_t k, int slot)
{
    uint8_t ctrl = __control(fp) & ~(1 << slot);
    uint64_t h2  = ((k & 0xff) << (8 * slot)) | __h2(fp);

    return h2 | (_U64(ctrl) << 56);
}


// clear the fingerprint in 'slot'
// NB: 0 is occupied, 1 is free.
static inline uint64_t
__clear_fp(uint64_t fp, int slot)
{
    uint8_t ctrl = __control(fp) | (1 << slot);
    uint64_t h2  = __h2(fp) & ~(_U64(0xff) << (8 * slot));
    return h2 | (_U64(ctrl) << 56);
}


// Find the key 'k' in bag 'g' and return the matching index.
// Return -1 on ENOENT
static inline int
__find_key(bag *g, uint64_t k, void **p_val)
{
    uint64_t fp = g->fp;
    uint8_t h2  = __h2(k);
    uint64_t fp_h2   = __fp(fp);
    uint8_t  control = __control(fp);

    // first find matches
    uint16_t m  = __find_matches(fp_h2, h2);

    // only keep the occ buckets
    uint16_t p  = m  & (~control & 0x7f);

    while (p) {
        int j = __builtin_ctz(p);
        p &= ~(1 << _U16(j));
        if (likely(g->hk[j] == k)) {
            *p_val = g->hv[j];
            return j;
        }
    }

    SIMD_PREFETCH_T0(&SL_NEXT(g, link));
    return -1;
}

// Given a bucket, find an empty slot
static inline int
__find_empty_slot(bag *g)
{
    uint64_t fp = g->fp;
    uint8_t  p  = __control(fp);
    if (p) {
        return __builtin_ctz(p);
    }
    return -1;
}


// Given this bag 'g' - find the key 'k' or an empty slot.
static inline int
__find_key_or_add(bag *g, uint64_t k, tuple *r)
{
    uint64_t fp = g->fp;
    uint8_t h2  = __h2(k);
    uint64_t fp_h2   = __fp(fp);
    uint8_t  control = __control(fp);

    uint16_t m  = __find_matches(fp_h2, h2);
    uint16_t p  = m  & (~control & 0x7f);

    r->val  = 0;
    r->slot = -1;

    while (p) {
        int j = __builtin_ctz(p);
        p &= ~(1 << _U16(j));
        if (likely(g->hk[j] == k)) {
            r->val  = g->hv[j];
            r->slot = j;
            return j;
        }
    }

    // At this point, we know we didn't find the key; so we have to
    // walk the list. Hence prefetch the next ptr.
    SIMD_PREFETCH_T0(&SL_NEXT(g, link));

    // see if there is an empty slot for us to use
    if (control) {
        r->slot = __builtin_ctz(control);
    }
    return -1;
}

// Insert (k, v) into bucket 'b' - but only if 'k' doesn't already
// exist in the bucket.
static inline void*
__insert(hb *b, uint64_t k, void *v)
{
    bag *g   = 0;
    int slot = 0;
    bag *bg  = 0;

    tuple r;
    SL_FOREACH(g, &b->head, link) {
        int j = __find_key_or_add(g, k, &r);
        if (unlikely(j >= 0)) {
            return r.val;
        }

        if (!bg && r.slot >= 0) {
            bg = g;
            slot = r.slot;
        }
    }

    if (!bg) {
        bg   = __NEWZ(bag);
        slot = 0;
        b->bags++;
        SL_INSERT_HEAD(&b->head, bg, link);
    }

    // Now we have to update the fp and control bits
    bg->fp = __update_fp(bg->fp, k, slot);
    bg->hk[slot] = k;
    bg->hv[slot] = v;
    b->nodes++;
    return 0;
}


// Insert (k, v) into bucket 'b' quickly. In this case, we _know_
// that we are being called when we are re-sized. So, the items
// naturally don't exist in the new bucket. So, all we have to do is
// find the first free slot in the _first_ bag.
static inline void
__insert_quick(hb *b, uint64_t k, void *v)
{
    bag *g  = SL_FIRST(&b->head);

    if (g) {
        int j = __find_empty_slot(g);
        if (j >= 0) {
            assert(!g->hk[j]);
            g->fp = __update_fp(g->fp, k, j);
            g->hk[j] = k;
            g->hv[j] = v;
            b->nodes++;
            return;
        }
    }

    // Make a new bag and put our element there.
    g = __NEWZ(bag);
    g->fp = __update_fp(g->fp, k, 0);
    g->hk[0] = k;
    g->hv[0] = v;
    b->bags++;
    b->nodes++;
    SL_INSERT_HEAD(&b->head, g, link);
}

// Find key 'hk' in 'b'; return the value in 't'.
// Return 1 if key is found, 0 otherwise.
static inline int
__findx(tuple *t, hb *b, uint64_t hk)
{
    bag *g = 0;

    SL_FOREACH(g, &b->head, link) {
        void *val = 0;
        int j = __find_key(g, hk, &val);
        if (likely(j >= 0)) {
            t->bag  = g;
            t->slot = j;
            t->val  = val;
            return 1;
        }
    }

    return 0;
}


/*
 * Double the hash buckets and redistribute all the nodes.
 */
static ht*
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
            uint8_t ctrl = __control(g->fp);

            SIMD_PREFETCH_T0(&SL_NEXT(g, link));

            for (int i = 0; i < FASTHT_BAGSZ; i++) {
                // skip empty slots
                if (ctrl & (1 << i)) continue;

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
    return h;
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
            h = resize(h);
            b = &h->b[__hash(k, h->n, h->salt)];
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
    tuple r;
    hb *b = &h->b[__hash(k, h->n, h->salt)];

    if (__findx(&r, b, k)) {
        if (p_ret) *p_ret = r.val;
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
    tuple r;
    hb *b = &h->b[__hash(k, h->n, h->salt)];

    if (__findx(&r, b, k)) {
        bag *g = r.bag;

        g->hv[r.slot] = val;
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
    tuple r;
    hb *b = &h->b[__hash(k, h->n, h->salt)];

    if (__findx(&r, b, k)) {
        bag *g   = r.bag;
        int slot = r.slot;

        if (p_ret) *p_ret = r.val;

        g->fp = __clear_fp(g->fp, slot);
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
