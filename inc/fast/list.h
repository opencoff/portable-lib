/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * list.h -  Fast list macros
 *
 * Lists provided:
 *    Singly linked
 *    Doubly linked
 *
 * This trick is borrowed shamelessly from the OpenBSD sources.
 *
 * Copyright (c) 2000-2010 Sudhi Herle <sw at herle.net>
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
 * Creation date: Thu Mar  9 11:27:50 2000
 */

#ifndef __FAST_SL_H__
#define __FAST_SL_H__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Templates in C -- linked lists
 *
 * This file documents macros that provide typesafe "templates" in
 * C.  Much of the material is borrowed shamelessly from OpenBSD.
 */


#ifdef __cplusplus
#ifndef typeof
#define typeof(a)   __typeof__(a)
#endif
#endif


/**
 * Define a singly linked list head as a struct.
 *
 * This macro defines the head of a singly linked list as a
 * struct. The primary use of this macro is to embed a list head
 * within another struct. i.e., where an explicit typedef is not
 * required.
 *
 * e.g.,
 * @code
 *      struct some_struct
 *      {
 *          int some_member;
 *          // Head of linked list of 'struct other_struct'.
 *          SL_HEAD(list_head, other_struct) head;
 *      };
 * @endcode
 *
 * @param name      Name of the struct
 * @param type      Type of the struct
 */
#define SL_HEAD(name, type)                          \
struct name {                                           \
    struct type *slh_first;                             \
}



/** Define an explicit typedef for a list head.
 *
 * This macro essentially does the same thing as the
 * SL_HEAD(). The only difference is that this macro creates an
 * explicit typedef out of \a name.
 *
 * e.g.,
 * @code
 *      SL_HEAD_TYPEDEF(list_head, some_struct);
 * @endcode
 * This example defines a new type 'list_head' to consist of linked
 * list nodes -- where each node is of type 'some_struct'.
 *
 * @param   name    Name of the struct and its typedef
 * @param   type    Type of the struct
 *
 * @see SL_HEAD.
 */
#define SL_HEAD_TYPEDEF(name, type)                  \
    typedef SL_HEAD(name, type) name
 

 
/** Macro to initialize a singly linked list head.
 *
 * This macro is used as variable initializers for initializing the
 * list head.
 *
 * e.g.,
 * @code
 *      list_head Head = SL_HEAD_INITIALIZER(&Head);
 * @endcode
 *
 * @param   head    Pointer to list head.
 */
#define SL_HEAD_INITIALIZER(head)                    \
    { 0 }
 

/** Embed a pointer to a linked list node within a struct.
 *
 * This macro is used to embed a "next" pointer within a larger
 * struct. This macro actually defines an anonymous struct and thus,
 * a variable declaration must be accompanied with this.
 *
 * @param   type        Type of the link for the "next" pointer. 
 *                      This is also called as the "name of link 
 *                      field".
 *
 * e.g.,
 *
 * @code
 *      struct some_struct
 *      {
 *          // .. some members of struct
 *
 *          SL_ENTRY(some_struct)  link;
 *
 *          SL_ENTRY(other_struct) other_link;
 *      };
 * @endcode
 *
 * This example shows that 'some_struct' is a node that can belong
 * in two different linked list simultaneously. One of the lists is
 * chained via the "link" pointer and the other is chained via
 * "other_link" pointer. We will refer to 'link' and 'other_link'
 * elements as "name of the link field" or just "field".
 */
#define SL_ENTRY(type)                               \
struct {                                                \
    struct type *sle_next;  /* next element */          \
}

/** Synonym for SL_ENTRY().
 *
 * @see SL_ENTRY
 */
#define SL_LINK(type)        SL_ENTRY(type)
 



/*
 * Singly-linked List access methods.
 */


/** Return the first node of the linked list.
 * 
 * @param   head    Head of the singly linked list.
 *
 * @return  The first node of the linked list.
 * @return  NULL if the list is empty.
 */
#define SL_FIRST(head)   ((head)->slh_first)


/** predicate that determines list empty/full.
 *
 * @param head   Head of the singly linked list.
 *
 * @return true  if the list is non empty
 * @return false if the list is empty
 */
#define SL_EMPTY(head)   (SL_FIRST(head) == 0)


/** Given a node, traverse to the next node.
 *
 * This macro takes a pointer to a node and the "name"
 * of the node to obtain the next node in the singly linked chain.
 *
 * @param   elm     Pointer to node
 * @param   field   The name of the link field.
 */
#define SL_NEXT(elm, field)  ((elm)->field.sle_next)


/** Iterate over each element of the linked list.
 *
 * This macro is useful for iterating over all elements in a linked
 * list. 
 *
 * @warning
 * Do NOT delete the list node ('ptr') while the list is being
 * traversed.
 *
 * @param   var     Variable that will be set to each node of the
 *                  list.
 * @param   head    Pointer to head of the linked list.
 * @param   field   The name of the link field.
 *
 * e.g.,
 * @code
 *      SL_FOREACH(ptr, &Head, link)
 *      {
 *          do_something (ptr);
 *      }
 * @endcode
 *
 */
#define SL_FOREACH(var, head, field)                 \
    for((var) = SL_FIRST(head);                      \
        (var) != 0;                                  \
        (var) = SL_NEXT(var, field))


#define SL_FOREACH_SAFE(var, head, field, tmp)      \
    for((var) = SL_FIRST(head);                     \
        (var) && (((tmp)=SL_NEXT((var), field)), 1); \
        (var) = tmp)

/*
 * Singly-linked List functions.
 */
#define SL_INIT(head) do {                           \
    SL_FIRST(head) = 0;                              \
} while (0)

#define SL_INSERT_AFTER(head,inlist,elm,field) do {  \
    (elm)->field.sle_next = (inlist)->field.sle_next;   \
    (inlist)->field.sle_next = (elm);                   \
} while (0)

#define SL_INSERT_HEAD(head, elm, field) do {        \
    (elm)->field.sle_next = (head)->slh_first;          \
    (head)->slh_first = (elm);                          \
} while (0)

#define SL_REMOVE_HEAD(head, field) do {             \
    (head)->slh_first = (head)->slh_first->field.sle_next;\
} while (0)



/*
 * Doubly linked list
 */
#define DL_HEAD(name,type)                           \
    struct name {                                       \
        struct type * first;                            \
        struct type * last;                             \
    }

#define DL_HEAD_TYPEDEF(name,type)                   \
    DL_HEAD(name,type);                              \
    typedef struct name name                            \

#define DL_HEAD_INITIALIZER(head)                    \
    { 0, 0 }

#define DL_ENTRY(type)                               \
    struct {                                            \
        struct type * next;                             \
        struct type * prev;                             \
    }

#define DL_LINK_INIT(elm,field)    do {              \
        elm->field.next = elm->field.prev = 0;          \
    } while (0)
    
#define DL_LINK(type)         DL_ENTRY(type)
#define DL_FIRST(head)        ((head)->first)
#define DL_LAST(head)         ((head)->last)
#define DL_END(head)          0
#define DL_NEXT(elm,field)    ((elm)->field.next)
#define DL_PREV(elm,field)    ((elm)->field.prev)

#define DL_EMPTY(head)        (DL_FIRST(head) == DL_END(head))

#define DL_empty_p(head)     DL_EMPTY(head)

#define DL_FOREACH(head,var,field)                   \
    for ((var) = DL_FIRST(head);                     \
         (var) != DL_END(head);                      \
         (var) = DL_NEXT(var,field))


#define DL_FOREACH_REVERSE(head,var,field)           \
    for ((var) = DL_LAST(head);                      \
         (var) != DL_END(head);                      \
         (var) = DL_PREV(var,field))


#define DL_INIT(head)     do {                       \
    (head)->first = (head)->last = 0;                \
} while (0)



/*
 * Prepend at the head of the list.
 */
#define DL_INSERT_HEAD(head,elm,field)    do {       \
    if ( ((elm)->field.next = (head)->first) )       \
        (head)->first->field.prev = (elm);           \
    else                                             \
        (head)->last = (elm);                        \
    (head)->first = (elm);                           \
    (elm)->field.prev = 0;                           \
} while (0)



/*
 * Append at the end of the list.
 */
#define DL_INSERT_TAIL(head,elm,field)    do {       \
    if ( ((elm)->field.prev = (head)->last) )           \
        (head)->last->field.next = (elm);               \
    else                                                \
        (head)->first = (elm);                          \
    (head)->last = (elm);                               \
    (elm)->field.next = 0;                              \
} while (0)

#define DL_APPEND(head,elm,field)    DL_INSERT_TAIL(head,elm,field)


/*
 * Insert 'elm' after 'inlist'
 */
#define DL_INSERT_AFTER(head,inlist,elm,field) do {  \
    if ( ((elm)->field.next = (inlist)->field.next) )   \
        (inlist)->field.next->field.prev = (elm);       \
    else                                                \
        (head)->last = (elm);                           \
    (elm)->field.prev = (inlist);                       \
    (inlist)->field.next = (elm);                       \
} while (0)



/*
 * Insert 'elm' before 'inlist'
 */
#define DL_INSERT_BEFORE(head,inlist,elm,field) do { \
    if ( ((elm)->field.prev = (inlist)->field.prev) )   \
        (inlist)->field.prev->field.next = (elm);       \
    else                                                \
        (head)->first = (elm);                          \
    (elm)->field.next = (inlist);                       \
    (inlist)->field.prev = (elm);                       \
} while (0)



/*
 * Remove an arbitrary node in the list.
 */
#define DL_REMOVE(head,elm,field)               do { \
    if ( (elm)->field.next )                            \
        (elm)->field.next->field.prev = (elm)->field.prev; \
    else                                                \
        (head)->last = (elm)->field.prev;               \
    if ( (elm)->field.prev )                            \
        (elm)->field.prev->field.next = (elm)->field.next; \
    else                                                \
        (head)->first = (elm)->field.next;              \
} while (0)



/*
 * Remove an element from the head
 */
#define DL_REMOVE_HEAD(head,field)       ({ \
            typeof((head)->first)  zz=0;    \
            if ( ((zz) = (head)->first) ) { \
                if  ( ((head)->first = (zz)->field.next) ) \
                     (head)->first->field.prev = 0; \
                else \
                    (head)->last = 0; \
            }; \
            zz;})


/*
 * Remove an element from the tail.
 */
#define DL_REMOVE_TAIL(head,field)   ({                     \
            typeof((head)->first)  zz=0;                    \
            if ( ((zz) = (head)->last) ) {                  \
                if ( ((head)->last = (zz)->field.prev) )    \
                    (head)->last->field.next = 0;           \
                else                                        \
                    (head)->first = 0;                      \
                                                            \
            };                                              \
            zz;})


/*
 * Append list 'newhead' to 'oldhead' (at the tail)
 */
#define DL_APPEND_LIST(oldhead, newhead,field)   do {            \
        if ( (oldhead)->last )                                      \
        {                                                           \
            if ( ((oldhead)->last->field.next = (newhead)->first) ) \
                (newhead)->first->field.prev = (oldhead)->last;     \
        }                                                           \
        else                                                        \
            (oldhead)->first = (newhead)->first;                    \
        if ( (newhead)->last )                                      \
            (oldhead)->last = (newhead)->last;                      \
        (newhead)->first = (newhead)->last = 0;                     \
    } while (0)



/*
 * Prepend list 'newhead' to 'oldhead' (at the head)
 */
#define DL_PREPEND_LIST(oldhead, newhead,field)   do {           \
        if ( (oldhead)->first )                                     \
        {                                                           \
            if ( ((oldhead)->first->field.prev = (newhead)->last) ) \
                (newhead)->last->field.next = (oldhead)->first;     \
        }                                                           \
        else                                                        \
            (oldhead)->last = (newhead)->last;                      \
        if ( (newhead)->first )                                     \
            (oldhead)->first = (newhead)->first;                    \
        (newhead)->first = (newhead)->last = 0;                     \
    } while (0)




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __FAST_SL_H__ */

/* EOF */
