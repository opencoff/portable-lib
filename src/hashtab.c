/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * hashtab.c - Obj. Oriented hash table interface
 *             This file has most of the common functions - esp.
 *             those that deal with creation, insertion, etc.
 *
 * Copyright (c) 1997-2006 Sudhi Herle <sw at herle.net>
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
 * Creation date: Wed Jul 14 15:22:47 1997
 */

#include "hashtab_imp.h"
#include "utils/hashfunc.h"
#include "utils/sysrand.h"


static int  insert_internal(hash_table * tab, void ** p_data, int op);
static int  resize(hash_table * tab);
static void remove_node(hash_table * tab, hash_bucket * b, hash_node * gone);

// Found on every contemporary system.
extern uint32_t arc4random(void);


/* insert modes */
#define TAB_NODUP       0
#define TAB_REPLACE     1
#define TAB_PROBE       2


static inline uint32_t
hashfunc(hash_table * tab, const void * key)
{
    return tab->random ^ (*tab->hash)(key);
}


/*
 * Create a new hash table.
 */
int
hash_table_new(hash_table_t * ptab, const hash_table_policy * tr)
{
    hash_table * tab;
    size_t i;

    if ( ! (ptab && tr && tr->mem.alloc && tr->hash && tr->cmp) )
        return -EINVAL;

    tab = (hash_table *)memmgr_alloc(&tr->mem, sizeof *tab);
    if (!tab)
        return -ENOMEM;

    memset(tab, 0, sizeof(*tab));

    i = tr->logsize <= 0 || (tr->logsize >= (8*sizeof(size_t))) ? 10 : tr->logsize;

    tab->fillmax = tr->fillmax <= 0 || tr->fillmax >= 100 ? 80 : tr->fillmax;
    tab->size    = 1 << i;
    tab->random  = arc4random();
    tab->hash    = tr->hash;
    tab->cmp     = tr->cmp;
    tab->dtor    = tr->dtor;
    tab->mem     = tr->mem;
    tab->lock    = tr->lock;
    tab->buckets = tNEWA(hash_bucket, tab, tab->size);
    if ( !tab->buckets )
    {
        memmgr_free(&tr->mem, tab);
        return -ENOMEM;
    }

    memset(tab->buckets, 0, sizeof(hash_bucket) * tab->size);

    /* Initialize locks - hash-table and per-bucket */
    lockmgr_create(&tab->lock);

    for (i = 0; i < tab->size; ++i)
    {
        hash_bucket* b = &tab->buckets[i];

        b->lock = tab->lock;
        lockmgr_create(&b->lock);
    }

    *ptab        = tab;

    return 0;
}



/*
 * Delete a hash table.
 */
void
hash_table_delete(hash_table * tab)
{
    size_t i;
    void (*dtor)(void*);
    memmgr mem;
    lockmgr lock;

    if (!tab) return;


    /*
     * Save vital information. We won't have access to it after
     * deleting the table.
     */
    lock = tab->lock;
    dtor = tab->dtor;
    mem  = tab->mem;

    /*
     * Walk thru each node and dispose of stuff.
     */
    lockmgr_lock(&lock);
    for (i = 0; i < tab->size; ++i)
    {
        hash_bucket* b  = &tab->buckets[i];
        hash_node* gone = SL_FIRST(&b->head);
        while (gone)
        {
            hash_node* next = SL_NEXT(gone, link);

            if (dtor) (*dtor)(gone->data);

            memmgr_free(&mem, gone);
            gone = next;
        }

        lockmgr_delete(&b->lock);
    }

    memmgr_free(&mem, tab->buckets);
    memmgr_free(&mem, tab);

    lockmgr_unlock(&lock);
    lockmgr_delete(&lock);
}






/* -- insert and lookup methods -- */


/* insert a node iff it doesn't exist */
int
hash_table_insert(hash_table * tab, const void * data)
{
    if (!(tab && data)) return -EINVAL;

    void * ret = (void *)data;
    return insert_internal(tab, &ret, TAB_NODUP);
}


/* replace a node iff it exists */
int
hash_table_replace(hash_table * tab, const void * data)
{
    if (!(tab && data)) return -EINVAL;

    void * ret = (void *)data;
    return insert_internal(tab, &ret, TAB_REPLACE);
}



/* insert a node; return existing if it already exists */
void *
hash_table_probe(hash_table * tab, const void * data)
{
    if (!(tab && data)) return 0;

    void * ret = (void *)data;
    return insert_internal(tab, &ret, TAB_PROBE) == 0 ?  ret : 0;
}


/* find a node.
 *
 * Return True on success, False otherwise.
 */
int
hash_table_lookup(hash_table * tab, const void * key, void** p_ret)
{
    void * ret = 0;
    hash_bucket * b;
    int (*cmp)(const void*, const void*);
    hash_node   * p;
    int retval = 0;
    uint32_t   hash;

    if (!(tab && key)) return -EINVAL;

    hash  = hashfunc(tab, key);
    b     = &tab->buckets[hash & (tab->size -1)];
    cmp   = tab->cmp;

    lockmgr_lock(&b->lock);

    SL_FOREACH(p, &b->head, link) {
        if (p->hash != hash)            continue;
        if (0 != (*cmp)(key, p->data))  continue;

        ret    = p->data;
        retval = 1;
        ++tab->stats.lookups;
        break;
    }

    lockmgr_unlock(&b->lock);

    if (!ret) ++tab->stats.failed_lookups;
    if (p_ret) *p_ret = ret;

    return retval;
}


/*
 * remove a node if it exists
 */
int
hash_table_remove(hash_table * tab, const void * key, void** p_val)
{
    int retval;
    uint32_t   hash;
    hash_bucket * b;
    int (*cmp)(const void*, const void*);
    hash_node   * p,
                ** p_next;

    if (!(tab && key)) return -EINVAL;

    retval = -ENOENT;
    hash   = hashfunc(tab, key);
    b      = &tab->buckets[hash & (tab->size -1)];
    cmp    = tab->cmp;

    lockmgr_lock(&b->lock);

    p_next = &SL_FIRST(&b->head);
    while ((p = *p_next)) {
        if (p->hash == hash && (0 == (*cmp)(key, p->data))) {
            *p_next = SL_NEXT(p, link);
            retval  = 0;
            if (p_val) *p_val = p->data;
            remove_node(tab, b, p);
            // We continue with our loop - in case there are
            // duplicates
        } else {
            p_next = &SL_NEXT(p, link);
        }
    }

    lockmgr_unlock(&b->lock);

    return retval;
}



/* conditionally remove an element */
int
hash_table_remove_if(hash_table * tab,
         int (*pred)(void *, const void * p), void * cookie)
{
    int retval;
    size_t i, 
           n;

    if (!(tab && pred)) return -EINVAL;


    // have to lock the entire table to protect against resizes when
    // we are traversing!
    lockmgr_lock(&tab->lock);

    retval = 0;
    n      = tab->size;
    for (i = 0; i < n; ++i) {
        hash_bucket* b = &tab->buckets[i];
        hash_node *  p,
                  ** p_next;

        // And, protect the bucket against nodes that are being
        // deleted whilst we are walking the bucket.
        lockmgr_lock(&b->lock);
        p_next = &SL_FIRST(&b->head);
        while ((p = *p_next)) {
            if ((*pred)(cookie, &p->data)) {
                ++retval;
                *p_next = SL_NEXT(p, link);
                remove_node(tab, b, p);
            }
            else
                p_next = &SL_NEXT(p, link);
        }
        lockmgr_unlock(&b->lock);
    }

    lockmgr_unlock(&tab->lock);
    return retval;
}



/* apply a function to all nodes */
void
hash_table_apply(hash_table * tab,
        void (*apply)(void*, const void*), void* cookie)
{
    size_t i, n;

    if (!tab || !apply) return;


    lockmgr_lock(&tab->lock);
    n = tab->size;
    for (i = 0; i < n; ++i) {
        hash_node * p;
        hash_bucket* b = &tab->buckets[i];

        lockmgr_lock(&b->lock);
        SL_FOREACH(p, &b->head, link) {
            (*apply)(cookie, p->data);
        }
        lockmgr_unlock(&b->lock);
    }

    lockmgr_unlock(&tab->lock);
}


/* return statistics */
hash_table_stat *
hash_table_stats(hash_table * tab, hash_table_stat * stat)
{
    if (!(tab && stat)) return 0;

    lockmgr_lock(&tab->lock);

    *stat      = tab->stats;
    stat->size = tab->size;

    lockmgr_unlock(&tab->lock);
    return stat;
}


/*
 * Internal functions
 */


/*
 * Return 0 on success, -errno on failure.
 */
static int
insert_internal(hash_table * tab, void ** p_data, int op)
{
    void * data = *p_data;
    int retval  = -ENOMEM;
    hash_node* e;
    int (*cmp)(const void*, const void*) = tab->cmp;

    uint32_t hash  = hashfunc(tab, data);
    hash_bucket* b = &tab->buckets[hash & (tab->size-1)];

    lockmgr_lock(&b->lock);
    SL_FOREACH(e, &b->head, link) {
        if (e->hash != hash)            continue;
        if (0 != (*cmp)(data, e->data)) continue;


        /*
         * What to do on collision:
         */
        switch (op) {
            default:
            case TAB_NODUP:
                *p_data = e->data;
                retval  = -EEXIST;
                break;

            case TAB_REPLACE:
                if (tab->dtor) (*tab->dtor)(e->data);
                *p_data = e->data;
                e->data = data;
                ++tab->stats.replaces;
                retval  = 0;
                break;

            case TAB_PROBE:
                *p_data = e->data;
                retval  = 0;
                break;
        }

        goto _done;
    }


    /*
     * Either the op is insert or the loop above didn't find the
     * element in question.
     */

    e = tNEW(hash_node, tab);
    if (!e) goto _done;

    retval  = 0;
    e->data = data;
    e->hash = hash;
    SL_INSERT_HEAD(&b->head, e, link);
    lockmgr_unlock(&b->lock);


    /*
     * Update statistics and see if table needs to grow.
     */
    lockmgr_lock(&tab->lock);

    ++tab->stats.inserts;
    ++tab->stats.nodes;
    ++b->count;
    if (b->count > tab->stats.maxchainlen)
        tab->stats.maxchainlen = b->count;


    /*
     * If one more bucket got filled, examine the total number of
     * filled buckets and determine if we need to grow the hash
     * table.
     */
    if (b->count == 1) {
        ++tab->stats.fill;
        if ( ((tab->stats.fill * 100) / (tab->size + 1)) > tab->fillmax )
            retval = resize(tab);
    }

    lockmgr_unlock(&tab->lock);

_done:
    return retval;
}



/*
 * Grow the hash table.
 */
static int
resize(hash_table * tab)
{
    size_t  i,
            n       = tab->size,
            newsize = n << 1,
            fill    = 0,
            maxchainlen = 0;

    lockmgr* lock = &tab->lock;
    hash_bucket * buckets = tNEWA(hash_bucket, tab, newsize);

    if (!buckets) return -ENOMEM;

    memset(buckets, 0, newsize * sizeof(hash_bucket));

    // Now, redistribute old nodes into new buckets
    for (i = 0; i < n; ++i) {
        hash_bucket* b = &tab->buckets[i];

        lockmgr_lock(&b->lock);
        hash_node * p  = SL_FIRST(&b->head);
        while (p) {
            hash_node * next   = SL_NEXT(p, link);
            hash_bucket * newb = &buckets[p->hash & (newsize-1)];

            SL_INSERT_HEAD(&newb->head, p, link);
            if ( ++newb->count > maxchainlen )
                maxchainlen = newb->count;

            if (newb->count == 1)
                ++fill;

            p = next;
        }
        lockmgr_unlock(&b->lock);

        // Copy the lock to the new buckets
        buckets[i].lock = b->lock;
    }

    // initialize locks for new buckets.
    for (i = n; i < newsize; ++i) {
        hash_bucket* new = &buckets[i];
        new->lock = *lock;
        lockmgr_create(&new->lock);
    }

    hash_bucket* oldb = tab->buckets;
    tab->buckets = buckets;
    tab->size    = newsize;

    ++tab->stats.splits;
    tab->stats.maxchainlen = maxchainlen;
    tab->stats.fill        = fill;

    tFREE(tab, oldb);

    return 0;
}


/* delete 'gone' and adjust statistics for bucket 'b' */
static void
remove_node(hash_table * tab, hash_bucket * b, hash_node * gone)
{
    if (tab->dtor) (*tab->dtor) (gone->data);

    tFREE(tab, gone);

    ++tab->stats.deletes;
    --tab->stats.nodes;

    if (0 == --b->count) {
        --tab->stats.fill;
        assert(!SL_FIRST(&b->head));
    } else {
        assert(SL_FIRST(&b->head));
    }

    /*
     * XXX Cannot fix maxchainlen unless we traverse entire hash
     * table.
     *
     * e.g., consider two buckets each with a chainlen of 4.
     * Further, assume that maxchainlen is also 4.
     * Now, if one of the overflow nodes in one of the buckets
     * is deleted, the maxchainlen is still 4.
     */

}



/* EOF */
