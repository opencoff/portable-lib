/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * hashtab_iter.c - Obj. Oriented hash table iterator interface.
 *                  This file provides an iterator interface to the
 *                  hash table.
 *
 * Copyright (C) 1997-2006 Sudhi Herle <sw at herle.net>
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
#include "utils/utils.h"


#define VANILLA_ITER        0 /* unsorted iter */
#define SORTED_ITER         1 /* sorted iter */
#define BUCKET_ITER         2 /* bucket iter */

/* unsorted iterator ops */
static int vanilla_iter_end(hash_table_iter * it);
static int vanilla_iter_next(hash_table_iter * it);
static int vanilla_iter_begin(hash_table_iter * it);
static void * vanilla_iter_item(hash_table_iter * it);


/* sorted iterator ops */
static int sorted_iter_init(hash_table_iter * it, const void * param);
static int sorted_iter_begin(hash_table_iter * it);
static int sorted_iter_end(hash_table_iter * it);
static int sorted_iter_next(hash_table_iter * it);
static void * sorted_iter_item(hash_table_iter * it);


/* bucket iterator ops */
static int bucket_iter_init(hash_table_iter * it, const void * key);
static int bucket_iter_begin(hash_table_iter * it);
static int bucket_iter_end (hash_table_iter * it);
static int bucket_iter_next (hash_table_iter * it);
static void * bucket_iter_item(hash_table_iter * it);


static const hash_table_iter_op Hash_vanilla_iter_op =
                _OPINIT(0, vanilla_iter_begin,
                        vanilla_iter_next, vanilla_iter_end, vanilla_iter_item);

static const hash_table_iter_op Hash_sorted_iter_op =
                _OPINIT(sorted_iter_init, sorted_iter_begin,
                        sorted_iter_next, sorted_iter_end, sorted_iter_item);

static const hash_table_iter_op Hash_bucket_iter_op =
                _OPINIT(bucket_iter_init, bucket_iter_begin,
                        bucket_iter_next, bucket_iter_end, bucket_iter_item);

static const hash_table_iter_op * Iter_tab[] =
{
    &Hash_vanilla_iter_op,
    &Hash_sorted_iter_op,
    &Hash_bucket_iter_op,
};

static int _mk_iter(hash_table_iter_t* p_ret,
                    hash_table * tab, int type, const void * param);




/*
 * Create a hash table iterator to walk thru' the elements of the
 * entire hash table.
 * Create a sorted iterator if 'sorted' is true.
 *
 * Return pointer to the iterator if successful, 0 otherwise.
 */
int
hash_table_iter_new(hash_table_iter_t* p_ret, hash_table * tab, int sorted)
{
    if (!(tab && p_ret)) return -EINVAL;

    return _mk_iter(p_ret, tab, sorted ? SORTED_ITER : VANILLA_ITER, 0);
}



/*
 * Create a hash table iterator to walk the collision chain of a
 * particular key.
 *
 * Return pointer to the iterator if successful, 0 otherwise.
 */
int
hash_table_bucket_iter_new(hash_table_iter_t* p_ret, hash_table * tab,
                           const void * k)
{
    if (!(tab && p_ret)) return -EINVAL;

    return _mk_iter(p_ret, tab, BUCKET_ITER, k);
}



/*
 * Delete and finalize a created iterator.
 */
void
hash_table_iter_delete(hash_table_iter * it)
{
    if (it) {
        hash_table * tab = it->table;

        assert (it->table);
        assert (it->op);

        lockmgr_lock(&tab->lock);

        (*it->op->end)(it);

        tFREE(tab, it);

        lockmgr_unlock(&tab->lock);
    }
}



/*
 * Return the item at the current position of the iterator.
 * Return 0 if at the end of the sequence.
 */
void *
hash_table_iter_item(hash_table_iter * it)
{
    void * p;

    if (!it) return 0;

    assert(it->table);
    assert(it->op);

    lockmgr_lock(&it->table->lock);

    p = (*it->op->item)(it);

    lockmgr_unlock(&it->table->lock);

    return p;
}


/*
 * Begin usage of the iterator by pointing to the first item to be
 * retrieved.
 * Return 0 on success, < 0 on failure.
 */
int
hash_table_iter_first(hash_table_iter * it)
{
    int v;

    if ( !it )
        return -EINVAL;

    assert(it->table);
    assert(it->op);

    lockmgr_lock(&it->table->lock);

    v = (*it->op->begin)(it);

    lockmgr_unlock(&it->table->lock);

    return v;
}



/*
 * Go to the next item in the iterator.
 * Return 0 on success, < 0 on failure.
 */
int
hash_table_iter_next(hash_table_iter * it)
{
    int v;

    if (!it)
        return -EINVAL;

    assert(it->table);
    assert(it->op);

    lockmgr_lock(&it->table->lock);

    v = (*it->op->next)(it);

    lockmgr_unlock(&it->table->lock);

    return v;
}





/* 
 * Internal functions
 */


static int
_mk_iter(hash_table_iter_t* p_ret, 
         hash_table * tab, int type, const void * param)
{
    hash_table_iter * it;
    int retval = 0;

    lockmgr_lock(&tab->lock);

    it = tNEW(hash_table_iter, tab);
    if (!it) {
        retval = -ENOMEM;
        goto _done;
    }

    memset (it, 0, sizeof (*it));
    it->table = tab;
    it->op    = Iter_tab[type];

    if (it->op->init)
        (*it->op->init)(it, param);

_done:
    lockmgr_unlock(&tab->lock);

    *p_ret = it;
    return retval;
}


/* -- Unsorted iterator -- */

static inline void
find_next_node(hash_table_iter* it, size_t i)
{
    hash_table* tab = it->table;
    size_t n = tab->size;

    for (; i < n; ++i) {
        it->cur = SL_FIRST(&tab->buckets[i].head);
        if (it->cur)  {
            it->un.table.bucket = i;
            return;
        }
    }
}


/* Start the iterator by pointing at the first usable element.  */
static int
vanilla_iter_begin(hash_table_iter * it)
{
    it->un.table.bucket = -1;
    it->cur             = 0;

    find_next_node(it, 0);

    return it->cur ? 0 : EOF;
}


/* "end" the iterator. */
static int
vanilla_iter_end(hash_table_iter * it)
{
    it->un.table.bucket = -1;
    it->cur          = 0;
    return 0;
}


/* Go to the next entry in the hash table. */
static int
vanilla_iter_next(hash_table_iter * it)
{
    if (it->cur) it->cur = SL_NEXT(it->cur, link);
    if (it->cur) return 0;

    find_next_node(it, it->un.table.bucket+1);
    return it->cur ? 0 : EOF;
}


/* Return item pointed at the current position of the iterator. */
static void *
vanilla_iter_item(hash_table_iter * it)
{
    return it->cur ? it->cur->data : 0;
}



/* -- sorted iterator -- */

static int
qsort_cmp(const void * a, const void * b)
{
    indir_node * p = (indir_node *)a,
               * q = (indir_node *)b;

    return (*p->cmp)(&p->node->data, &q->node->data);
}


/* Initialize the sorted iterator. */
static int
sorted_iter_init(hash_table_iter * it, const void * param)
{
    hash_table * tab = it->table;
    indir_node * nodes;
    sorted_iter * sorted = &it->un.sorted;
    size_t i, n;
    hash_cmp_f * cmp;
    int ok = EOF;

    assert(tab);

    USEARG(param);

    lockmgr_lock(&tab->lock);
    sorted->max = tab->stats.nodes;
    if (sorted->max == 0) goto _done;

    cmp         = tab->cmp;
    sorted->nodes = tNEWA(indir_node, tab, sorted->max);

    nodes = sorted->nodes;
    n     = tab->size;
    for (i = 0; i < n; ++i)
    {
        hash_bucket * b = &tab->buckets[i];
        hash_node * node;

        lockmgr_lock(&b->lock);
        SL_FOREACH(node, &b->head, link)
        {
            nodes->node = node;
            nodes->cmp = cmp;
            ++nodes;
        }
        lockmgr_unlock(&b->lock);
    }

    // XXX We are holding a pointer to 'node'. A different thread
    // can delete it after we relinquish the lock!
    if (sorted->max > 1)
        qsort(sorted->nodes, sorted->max, sizeof sorted->nodes[0], qsort_cmp);

    ok = 0;

_done:
    lockmgr_unlock(&tab->lock);
    return ok;
}


/* Start the iterator by pointing at the first usable element. */
static int
sorted_iter_begin(hash_table_iter * it)
{
    assert(it->table);
    it->un.sorted.index = 0;
    return it->un.sorted.index < it->un.sorted.max ? 0 : EOF;
}



/* "end" the iterator. */
static int
sorted_iter_end(hash_table_iter * it)
{
    assert(it->table);

    tFREE(it->table, it->un.sorted.nodes);
    it->un.sorted.max   = 0;
    it->un.sorted.nodes = 0;
    return 0;
}



/* Go to the next entry in the hash table. */
static int
sorted_iter_next(hash_table_iter * it)
{
    size_t i;

    assert (it->table);

    i = it->un.sorted.index+1;
    if ( i >= it->un.sorted.max )
    {
        it->un.sorted.index = it->un.sorted.max;
        return EOF;
    }

    it->un.sorted.index = i;
    return 0;
}


/* Return item pointed at the current position of the iterator. */
static void *
sorted_iter_item(hash_table_iter * it)
{
    sorted_iter * sorted = &it->un.sorted;
    size_t i = sorted->index;
    void * pair = 0;

    if ( i < sorted->max )
    {
        assert(sorted->nodes);

        pair = sorted->nodes[i].node->data;
    }
    return pair;
}



/* -- hash bucket iterator -- */


/* Initialize the bucket iterator. */
static int
bucket_iter_init(hash_table_iter * it, const void * key)
{
    hash_table * tab = it->table;
    uint32_t  hash  = (*tab->hash)(key);
    hash_bucket * b  = &tab->buckets[hash & (tab->size -1)];

    it->un.bucket.bucket = b;
    it->cur = SL_FIRST(&b->head);

    return 0;
}


/* Start the iterator by pointing at the first usable element. */
static int
bucket_iter_begin(hash_table_iter * it)
{
    bucket_iter * bi = &it->un.bucket;

    it->cur = SL_FIRST(&bi->bucket->head);
    return it->cur ? 0 : EOF;
}



/* "end" the iterator. */
static int
bucket_iter_end(hash_table_iter * it)
{
    USEARG(it);
    return 0;
}



/* Go to the next entry in the hash table. */
static int
bucket_iter_next(hash_table_iter * it)
{
    it->cur = SL_NEXT(it->cur, link);
    return it->cur ? 0 : EOF;
}



/* Return item pointed at the current position of the iterator. */
static void *
bucket_iter_item(hash_table_iter * it)
{
    return it->cur ? it->cur->data : 0;
}

/* EOF */
