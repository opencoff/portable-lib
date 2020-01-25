/* vim expandtab:tw=68:ts=4:sw=4:
 *
 * gstr.h - Growable/Generic strings for C
 *
 * Copyright (c) 1999-2010 Sudhi Herle <sw at herle.net>
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
 * Creation date: Mon Aug 30 09:31:50 1999
 */

#ifndef __GSTRING_H_3901300761__
#define __GSTRING_H_3901300761__ 1

#include <stdio.h>

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <string.h>

struct gstr
{
    char* str;
    size_t   len;
    size_t   cap;
};
typedef struct gstr gstr;

/**
 * Initialize a new gstr object to initially hold a string of
 * size 'size'. 
 *
 * Return the initialized gstr ptr (same as the object passed to the
 * function).
 */
extern gstr* gstr_init(gstr *, size_t size);


/**
 * Initialize a new gstr from a C-string. Return a pointer to the
 * initialized gstr object.
 */
extern gstr* gstr_init_from(gstr*, const char*);


/**
 * Duplicate a gstring into another one
 */
gstr* gstr_dup(gstr* d, const gstr* src);


/**
 * Deallocate storage from a gstr object.
 */ 
extern void gstr_fini(gstr*);


/**
 * Unbind the underlying string from the gstr structure and
 * return it. The caller is responsible for freeing the returned
 * pointer.
 */
extern char* gstr_finalize(gstr*);


/* All of below return new string length */

/**
 * Append a gstr 'src' to the gstr 'dest'. 
 *
 * Return the new length of the gstr.
 */
extern int gstr_append(gstr* dest, const gstr* src);


/**
 * Append a C-string 'src' to the gstr 'dest'. 
 *
 * Return the new length of the gstr.
 */
extern size_t gstr_append_str(gstr* dest, const char* src);


static inline int
gstr_eq(gstr* a, gstr* b)
{
    if (a->len != b->len)   return 0;
    return 0 == memcmp(a->str, b->str, a->len);
}

static inline int
gstr_eq_str(gstr* a, const char* s)
{
    return 0 == strcmp(a->str, s);
}


/**
 * Append a character 'ch' to the gstr 'dest'. 
 *
 * Return the new length of the gstr.
 */
extern int gstr_append_ch(gstr* dest, int ch);


/**
 * Truncate iff string is bigger than new_len, else return existing
 * length.
 */
extern int gstr_truncate(gstr*, size_t new_len);
#define gstr_reset(g)  do { gstr* _x = (g);          \
                        _x->len = 0; _x->str[0] = 0; \
                       } while(0)
                    


/**
 * Chop the last character of the string and return it.
 */
extern int gstr_chop(gstr*);


/**
 * Chop the last character iff it is in the character  set 's'
 *
 * Return the chopped character.
 */
extern int gstr_chop_if(gstr*, const char* s);


/**
 * copy 'src' into 'dest' and return 'dest'.
 * i.e., strcpy(3) on gstrs.
 */
extern gstr* gstr_copy(gstr* dest, const gstr* src);


/**
 * Accessor functions - returns the underlying C-string and length
 * respectively.
 */
extern char*  (gstr_str)(gstr*);
extern size_t (gstr_len)(gstr*);


/**
 * Trim leading & trailing whitespace from the gstr.
 * Return the input gstr.
 */
extern gstr* gstr_trim(gstr*);


/**
 * Remove leading & trailing quote chars from the string.
 * If 'p_ret' is non-Null, return the resulting substring in
 * '*p_ret'.
 *
 * Return:
 *    0     No quote found
 *    > 0   Quote char that was removed (either '"' or '\'')
 *    < 0   No ending quote found
 *
 * NB: This function expects the caller to have already trimmed
 *     leading/trailing white spaces.
 */
extern int gstr_unquote(gstr* g);

/*
 * If 'src' contains any chars in
 * 'escset', escape it with 'esc_char'.
 *
 * Note: 'esc_char' can't be in 'escset'.
 */
extern gstr *gstr_escape(gstr *g, const char *escset, int esc_char);

/*
 * Unescape a string 'src' where 'esc_char' is used to "escape" the
 * special chars.
 */
extern gstr* gstr_unescape(gstr *g, int esc_char);

/**
 * Get a null-terminated line of text delimited by any char in 'tok' from
 * file 'fp' into gstr 'g'.
 *
 * THE DELIMITER IS REMOVED FROM 'g' before returning.
 *
 * Returns:
 *  On success: Number of chars in line
 *  On Failure: -1.
 *  On EOF: 0
 */
extern int gstr_readline(gstr*, FILE* fp, const char* tok);

/**
 * Expand variables of the form $VAR or ${VAR} in the string 'g'.
 * Use lookup function '(*find)()' to map variables to their values.
 * Cookie is passed transparently to find().
 *
 * Returns:
 *    0   on success
 *    -ENOENT if var is not found (or mapped)
 *    -EINVAL if variable is malformed
 */
extern int gstr_varexp(gstr *g, const char* (*find)(void*, const char* key), void * cookie);

#define gstr_lastch(g)  ({gstr* _x = (g); \
                          int _c   = _x->len > 0 ? _x->str[_x->len-1] : 0;\
                          _c;})
#define gstr_firstch(g) ((g)->str[0])
#define gstr_len(g)     (g)->len
#define gstr_str(g)     (g)->str
#define gstr_ch(g, i)   (g)->str[(i)]

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __GSTRING_H_3901300761__ */

/* EOF */
