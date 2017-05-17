/* :vim: ts=4:sw=4:expandtab:tw=68:
 * 
 * pair.h - Definition of key-value pair and function typedefs that
 * can act on it.
 *
 * Copyright (c) 1997-2005 Sudhi Herle <sw at herle.net>
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

#ifndef __PAIR_H_1137905286_65488__
#define __PAIR_H_1137905286_65488__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h>


struct keyval_pair
{
    void * key;
    size_t keylen;

    void * val;

    /* opaque cookie for callers to use it in any way they choose
     * to.. */
    void * cookie;
};
typedef struct keyval_pair keyval_pair;


/*
 * Key comparison function that must return:
 *      > 0 if lhs > rhs
 *      < 0 if lhs < rhs
 *      0   if lhs == rhs
 */
typedef int key_cmp_f(const void * lhs_key, const void * rhs_key);



/*
 * Function type for different operations that can be performed on
 * the key-val pair. e.g., destructor, application etc.
 */
typedef void keyval_op_f(keyval_pair *);



/*
 * Predicate that returns 'true' or 'false' for the given key-value
 * pair 'p'. The logic to determine this is application specific.
 * 'cookie' is an opaque caller supplied parameter that will passed
 * to this predicate as the first argument.
 * Must return:
 *       >0   if true
 *       0    if false
 */
typedef int keyval_pred_f(const void * cookie, const keyval_pair * p);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PAIR_H_1137905286_65488__ */

/* EOF */
