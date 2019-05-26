/* :vim:ts=4:sw=4:
 *
 * strsplit.c -  Inspired by
 *  "Practice of Programming" --  (Kernighan and Pike)
 *
 * Copyright (c) 1999-2007 Sudhi Herle <sw at herle.net>
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
 * Creation date: Sun Aug 22 19:18:19 1999
 */

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include "utils/utils.h"

#include "bits.h"


struct stra
{
    unsigned char ** arr;
    int size;
    int cap; /* setting to -1 indicates that this is not growable */
};
typedef struct stra stra;




static int __split    (stra *strv, char * str, const delim *delim);
static int __split_sqz(stra *strv, char * str, const delim *delim);


/*
 * Initialize a new str vector.
 */
static void
_stra_init(stra * a)
{
    a->size = 0;
    a->cap  = 16;
    a->arr  = (unsigned char **) calloc(a->cap, sizeof (unsigned char *));
}


/*
 * Initialize from a fixed size vector 'v' of 'n' slots.
 *
 * We use negative capacity to represent fixed sizes.
 */
static inline void
_stra_init_from(stra* a, char**v, int n)
{
    a->size = 0;
    a->cap  = -n;
    a->arr  = (unsigned char**)v;
}


static inline void
_stra_fini(stra* a)
{
    if (a->arr && a->cap > 0)
        free(a->arr);

    a->size = a->cap = 0;
    a->arr  = 0;
}


/*
 * resize and return true if successful, false otherwise.
 */
static inline int
_stra_resize(stra * a)
{
    int retval = 1;
    int newcap = a->cap * 2;
    unsigned char ** newarr;

    newarr  = (unsigned char **) realloc(a->arr,
                                 newcap * sizeof (unsigned char *));
    if ( newarr )
    {
        memset(newarr + a->cap, 0, a->cap * sizeof (unsigned char *));
        a->cap = newcap;
        a->arr = newarr;
    }
    else
    {
        free(a->arr);
        a->arr  = 0;
        a->size = 0;
        a->cap  = 0;
        errno   = ENOMEM;
        retval  = 0;
    }

    return retval;
}


/*
 * Return True if 'a' can hold 'n' more elements
 *
 * Don't resize if one of two conditions hold:
 *  - capacity is pinned (a->cap < 0)
 *  - the size is big enough to accommodate the new request
 */
static inline int
_stra_resize_if(stra* a, int n)
{
    int want = a->size + n;
    if (a->cap < 0)
        return want <= -a->cap;

    if (want >= a->cap)
        return _stra_resize(a);

    return 1;
}


static inline int
__split_common(stra* a, char* str, const char* tok, int sqz_consec)
{
    delim   d;
    unsigned int c;

    __init_delim(&d);
    /*
     * Initialize a fast lookup bitvector to tell us if an input char is
     * a delimter or not.
     */
    for (; (c = (0xff & *tok)); tok++)
        __add_delim(&d, c);

    return sqz_consec ? __split_sqz(a, str, &d) : __split(a, str, &d);
}




/*
 * Quick string split - where there are no memory allocations done.
 * This is for cases when the caller knows the precise number of
 * delimiters in the input string.
 */
int
strsplit_quick(char **strv, int n, char * str, const char* tok, int sqz_consec)
{
    stra a;

    _stra_init_from(&a, strv, n);
    n = __split_common(&a, str, tok, sqz_consec);
    return n ? a.size : -1;
}



/*
 * Flexible string split - where the number of tokens after
 * splitting is not known apriori.
 */
char **
strsplit(int *strv_size, char * str, const char * tok, int sqz_consec)
{
    stra a;

    _stra_init(&a);

    if (!__split_common(&a, str, tok, sqz_consec)) {
        _stra_fini(&a);
        return 0;
    }

    if (strv_size)
        *strv_size = a.size;

    return (char **)a.arr;
}



/*
 * Split string without any compression of multiple delims.
 *
 * Return True on success, False otherwise.
 */
static int
__split(stra * a, char * in_str, const delim *d)
{
    unsigned char  * str = (unsigned char *) in_str,
                   * begin;
    unsigned int c;

    for (begin = str; (c = *str); str++) {
        if (__is_delim(d, c)) {
            if (!_stra_resize_if(a, 1)) return 0;

            *str = 0;
            //printf("#  %d => |%s|\n", a->size, begin);
            a->arr[a->size++] = begin;
            begin = str + 1;
        }
    }


    if (*begin) {
        if (!_stra_resize_if(a, 1)) return 0;

        //printf("#  %d => |%s|\n", a->size, begin);
        a->arr[a->size++] = begin;
    }

    return 1;
}



/*
 * Split string compressing any intermediate delims.
 */
static int
__split_sqz(stra *a, char * in_str, const delim *d)
{
    unsigned char  *str = (unsigned char *) in_str,
                   *begin;
    unsigned int c,
                 prev_c = 0;

    str = (unsigned char *)strtrim ((char *)str);
    for (begin = str; (c = *str); prev_c = c, ++str) {
        if ( __is_delim(d, c)) {
            *str = 0;
        } else if (__is_delim(d, prev_c)) {
            if (!_stra_resize_if(a, 1)) return 0;

            a->arr[a->size++] = begin;
            begin = str;
        }
    }

    if (*begin) {
        if (!_stra_resize_if(a, 1)) return 0;
        a->arr[a->size++] = begin;
    }

    return 1;
}


/* EOF */
