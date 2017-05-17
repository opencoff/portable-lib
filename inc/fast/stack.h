/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * stack.h - Fast stack services.
 *
 * Copyright (c) 2005-2010 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#ifndef __STACK_H__
#define __STACK_H__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "utils/utils.h"


/* This struct type is for internal use of the macros defined herein. */
typedef struct  __fast_stack __fast_stack;
struct __fast_stack
{
    int sp;     /* Current stack pointer */
    int top;    /* Top of stack. */
};


/*
 * This macro makes a typedef (and introduces it into the current
 * scope).  The typedef is specific to holding objects of type
 * 'ty_'. Further, the size of the stack is fixed at 'sz_' objects.
 *
 *  typnm_ is the type name given to the stack
 *  ty_ is the type of the things to be stacked
 *  sz_ is the number of things to be held in the stack
 */
#define FSTACK_TYPEDEF(typnm_,ty_,sz_)  typedef struct typnm_  \
                                     {                      \
                                         __fast_stack s;    \
                                         ty_ stack[sz_];    \
                                     } typnm_


/*
 * Compatibility alias. The old name is deprecated and will go away
 * soon.
 */
#define FSTACK_TYPE(nm_,ty_,sz_) FSTACK_TYPEDEF(nm_,ty_,sz_)




/*
 * Declare a "dynamic" fast-stack where the size is not fixed at
 * compile time.
 *
 *  typnm_ is the type name given to the stack
 *  ty_ is the type of the things to be stacked
 */
#define FSTACK_DYN_TYPEDEF(typnm_,ty_) \
                                     typedef struct typnm_  \
                                     {                      \
                                         __fast_stack s;    \
                                         ty_ * stack;       \
                                     } typnm_



/*
 * This macro initializes a stack of a certain
 * stack type as declared by the FSTACK_TYPE() macro.
 *
 * An instance of the stack must first be created (i.e. space is
 * allocated for it).
 *
 *  t_  is the name of the instance of the stack to be initialized
 *  sz_ is the number of things to be held in the stack
 */
#define FSTACK_INIT(t_,sz_)                                 \
                                     do {                   \
                                         __fast_stack * _t = &(t_)->s;\
                                        _t->sp = 0;         \
                                        _t->top = sz_;      \
                                     } while (0)



/*
 * This macro initializes a dynamic stack of a certain
 * stack type as declared by the FSTACK_DYN_TYPEDEF() macro.
 *
 * An instance of the stack must first be created (i.e. space is
 * allocated for it).
 *
 *  t_  is the name of the instance of the stack to be initialized
 *  ty_ is the type of the stack elements.
 *  sz_ is the number of things to be held in the stack
 */
#define FSTACK_DYN_INIT(t_,ty_,sz_)                        \
                                     do {                   \
                                         __fast_stack * _t = &(t_)->s;\
                                        _t->sp    = 0;         \
                                        _t->top   = sz_;      \
                                        (t_)->stack = NEWA(ty_,sz_); \
                                     } while (0)


/*
 * Finalize/cleanup a dynamic stack initialized with
 * FSTACK_DYN_INIT().
 */
#define FSTACK_DYN_FINI(t_)  do { \
                                __fast_stack * _t = &(t_)->s;\
                                DEL ((t_)->stack);\
                                (t_)->stack = 0; \
                                _t->top = _t->sp = 0; \
                            } while (0)




/*
 * Reset the stack pointers; this is similar to calling INIT() a
 * second time without any associated memory management.
 */
#define FSTACK_RESET(t_)    do { (t_)->s.sp = 0; } while (0)



/*
 * This macro pushes an element onto a previously created and
 * initialized stack.
 *
 *  t_  is the name of the instance of the stack onto which an
 *      element is being pushed.
 *  e_  is the element to be pushed onto the stack
 *  s_  is a variable to hold the error flag.  If the item cannot be
 *      pushed, this flag will be set to 1.  Otherwise, it is 0.
 */
#define FSTACK_PUSH(t_,e_,s_)                               \
                                     do {                   \
                                         __fast_stack * _t = &(t_)->s;\
                                        int _s = 0;         \
                                        int _sp = _t->sp;   \
                                        if ( _sp == _t->top )\
                                            _s = 1;         \
                                        else {              \
                                            (t_)->stack[_sp] = e_;\
                                            _t->sp = _sp + 1;\
                                        }                   \
                                        s_ = _s;            \
                                     } while (0)



/*
 * This macro pops an element off a previously created and
 * initialized stack.
 *
 *  t_  is the name of the instance of the stack from which an
 *      element is being popped.
 *  e_  is the variable of correct type to hold the element 
 *      to be popped from the stack
 *  s_  is a variable to hold the error flag.  If the item cannot be
 *      popped, this flag will be set to 1.  Otherwise, it is 0.
 */
#define FSTACK_POP(t_,e_,s_)                                \
                                     do {                   \
                                         __fast_stack * _t = &(t_)->s;\
                                        int _s = 0;         \
                                        int _sp = _t->sp-1; \
                                        if ( _sp < 0)       \
                                            _s = 1;         \
                                        else                \
                                            e_ = (t_)->stack[_t->sp = _sp]; \
                                        s_ = _s;            \
                                     } while (0)




/*
 * This macro peeks at the top element in a previously created and
 * initialized stack.  This essentially makes a copy of the element
 * in the variable e_.
 *
 *  t_  is the name of the instance of the stack containing the
 *      element to be peeked at.
 *  e_  is the variable of correct type to hold the copy of the element 
 *      to be used for peeking purposes.
 *  s_  is a variable to hold the error flag.  If the item cannot be
 *      popped, this flag will be set to 1.  Otherwise, it is 0.
 */
#define FSTACK_PEEK(t_,e_,s_)                               \
                                     do {                   \
                                         __fast_stack * _t = &(t_)->s;\
                                        int _s = 0;         \
                                        int _sp = _t->sp-1; \
                                        if ( _sp < 0)       \
                                            _s = 1;         \
                                        else                \
                                            e_ = (t_)->stack[_sp]; \
                                        s_ = _s;            \
                                     } while (0)



/*
 * This macro returns the current stack depth -- i.e., number of
 * elements on the stack.
 *  t_  is the name of the instance of the stack containing the
 *      element to be peeked at.
 */
#define FSTACK_DEPTH(t_)            ((t_)->s.sp)
#define FSTACK_SP(t_)               ((t_)->s.sp)


/*
 * Return the element at stack depth 'i'
 * Note: No error check is done to verify if 'i' is within the
 * stack.
 */
#define FSTACK_ITEM(t_, i)         ((t_)->stack[i])



/*
 * These macros are predicates to test whether a previously created
 * and initialized stack is full or empty.
 *
 *  t_  is the name of the instance of the stack containing the
 *      element to be peeked at.
 */
#define FSTACK_FULL_P(t_)           ((t_)->s.sp == (t_)->s.top)
#define FSTACK_EMPTY_P(t_)          ((t_)->s.sp == 0)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __STACK_H__ */

/* EOF */
