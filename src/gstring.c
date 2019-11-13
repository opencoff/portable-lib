/* :vim:ts=4:sw=4:
 *
 * gstring.c -  Growable/Generic string interface
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
 * Creation date: Mon Aug 30 09:31:50 1999
 */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include "utils/gstring.h"
#include "utils/utils.h"

/* Grow string `g' by `len' bytes -- if necessary. */
static int grow_if(gstr * g, int len);

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif /* !CHAR_BIT */


#define LONGBIT       (CHAR_BIT * sizeof(unsigned long))
#define BITVECTSIZE   (256 / LONGBIT)
#define _vect(c)      ((c) / LONGBIT)
#define _mask(c)      (1 << ((c) % LONGBIT))

#define ISDELIM(v,c)  (v[_vect(c)] & _mask(c))
#define ADDELIM(v,c)  (v[_vect(c)] |= _mask(c))


/* Grow the gstr by len -- if necessary. */
static int
grow_if(gstr * g, int len)
{
    size_t newcapacity = g->cap,
           reqd        = g->len + len;
    char * str;

    if (newcapacity > reqd)
        return 0;

    /* Align to nearest 1k boundary */
    newcapacity = _ALIGN_UP(reqd, 1024);

    str = RENEWA(char, g->str, newcapacity);

    /* XXX DIE? */
    if (! str)
        return -1;

    g->str = str;
    g->cap = newcapacity;
    return 0;
}


/* Initialize a gstr structure to hold string of atleast `size'
 * bytes.
 *
 * Returns:
 *  On Success: Pointer to input gstr.
 *  On failure: NULL (memory exhausted)
 */
gstr *
gstr_init(gstr * g, size_t size)
{
    assert(g);

    if ( size == 0 ) size = 128;

    g->len = 0;
    g->cap = size;
    g->str = NEWA(char, size);
    assert(g->str);
    g->str[0] = 0;
    return g;
}


gstr*
gstr_init2(gstr* g, const gstr* s)
{
    assert(g);
    assert(s);

    size_t n    = s->len+1;
    g->str      = NEWA(char, n);
    g->len      = s->len;
    g->cap = n;
    memcpy(g->str, s->str, n);
    return g;
}


/*
 * Initialize from nul terminated string 's'
 */
gstr*
gstr_init_from(gstr* g, const char* s)
{
    assert(s);

    size_t n = strlen(s) + 1;
    gstr_init(g, n);
    memcpy(g->str, s, n);

    return g;
}


/* Deallocate any resources associated with a gstr.
 * If you plan to use the string later on, DO NOT CALL THIS
 * FUNCTION!
 */
void
gstr_fini(gstr *g)
{
    assert(g);
    DEL(g->str);
    g->len = 0;
    g->cap = 0;
    g->str = 0;
}


static size_t
__append(gstr* g, const char* str, size_t len)
{
    len = strlen(str);
    if (grow_if(g, len+1) < 0) return 0;

    memcpy(g->str + g->len, str, len+1);

    return g->len += len;
}


/* Add atmost `len' bytes of the character string `str' into the
 * gstr.
 * Returns:
 *  On success: Number of bytes in the string.
 *  On failure: -1  if no memory
 */
size_t
gstr_append_str(gstr * g, const char * str)
{
    assert(g);

    return str ? __append(g, str, strlen(str)) : g->len;
}


/* Append 'newg' to 'g'.
 * Returns:
 *   On success: Number of bytes (after appending) in 'g'
 *   On failure: -1   if no memory
 */
int
gstr_append(gstr * g, const gstr * that)
{
    assert(g);

    return that ? __append(g, that->str, that->len) : g->len;
}


int
gstr_append_ch(gstr* g, int ch)
{
    assert(g);

    if (grow_if(g, 1) < 0) return -1;

    g->str[g->len++] = ch;
    g->str[g->len]   = 0;
    return g->len;
}

/* Truncate a string */
int
gstr_truncate(gstr * g, size_t n)
{
    assert(g);

    if (g->len > n)
    {
        g->len = n;
        g->str[n] = 0;
    }

    return g->len;
}


/*
 * "chop" the last character if it is CR or LF
 * Return
 *  True    if chopped
 *  False   otherwise
 */
int
gstr_chop(gstr * g)
{
    assert(g);

    int c = 0;
    if ( g->len > 0 )
    {
        c = g->str[g->len-1];
        if (c == '\r' || c == '\n')
            g->str[--g->len] = 0;
        else
            c = 0;
    }

    return c;
}



/*
 * "chop" last character if it is one of the characters in 's'
 * Return 0 if EOS.
 */
int
gstr_chop_if(gstr * g, const char * s)
{
    assert(g);

    int c;

    if ( !s )
        return 0;

    if (g->len == 0)
        return 0;

    c = g->str[g->len-1];

    /* Remove the char if it is present in 's'
     * Optimization: Usually, we only have one character in 's'.
     * So, to speed it up, see if the char we chopped is the first
     * char. If not, we go the lengthy route.
     */
    if ( c  && (c == *s || strchr(s, c)) )
        g->str[--g->len] = 0;

    return c;
}


/* Copy 'src' to 'dest'.
 * Returns:
 *   On success: Number of bytes copied
 *   On failure: -1 if no memory
 */
gstr*
gstr_copy(gstr * dest, const gstr* src)
{
    size_t len;

    assert(dest);
    assert(src);

    len = src->len;
    dest->len = 0;

    if (grow_if(dest, len+1) < 0) return 0;

    memcpy(dest->str, src->str, len+1);
    return dest;
}


/* Return the contiguous character string corresponding to the
 * gstr `g'.
 * Returns:
 *  On success: Pointer to the NULL terminated string grown so far.
 *  On failure: 0.
 */
char *
(gstr_str)(gstr * g)
{
    return g ? g->str : 0;
}


/* Return the length of the null terminated string within the
 * gstr `g'.
 * Returns:
 *  On success: Number of bytes in the string.
 */
size_t
(gstr_len)(gstr * g)
{
    return g ? g->len : 0;
}


/* Remove leading and trailing white spaces from a string. */
gstr*
gstr_trim(gstr * g)
{
    assert(g);

    char * ptr,
         * str,
         * str_end;
    int c;

    /* Remove leading spaces */
    for (str = g->str; (c = *str); str++) {
        if (!isspace(c))
            goto _nonws;
    }

    /* The entire string is white spaces, return now. */
    *g->str = 0;
    g->len  = 0;
    return g;

_nonws:
        
    /*
     * Copy from str to end -- remembering the last non-white
     * space position in 'str_end'.
     */
    for (ptr = str_end = g->str; (c = *ptr = *str);
                 str++, ptr++)
    {
        if (!isspace(c))
            str_end = ptr;
    }

    /*
     * '*str_end' is the last non-white-space character in the string.
     * So, we end the string _after_ it.
     */
    *++str_end = 0;
    g->len = str_end - g->str;

    return g;
}



/*
 * Remove any enclosing quote chars; i.e., leading and trailing
 * quote chars.
 *
 * Return:
 *    0     if no quote found
 *    > 0   the quote char we just removed
 *    < 0   if matching end-quote is not found
 */
int
gstr_unquote(gstr* g)
{
    int q = g->str[0];
    if (q != '"' && q != '\'')  return 0;

    if (g->str[g->len-1] != q) return -1;

    g->str[--g->len] = 0;
    memmove(g->str, g->str+1, g->len);
    return q;
}


/* Get a null-terminated line of text delimited by any char in 'tok' from
 * file 'fp' into gstr 'g'.
 *
 * THE DELIMITER IS NOT PART OF THE RETURNED STRING!
 *
 * Returns:
 *  On success: Number of chars in line
 *  On Failure: -1.
 *  On EOF: 0
 */
int
gstr_readline(gstr * g, FILE * fp, const char * tok)
{
    unsigned long delim[BITVECTSIZE];
    char * str;
    size_t len;
    int c;

    assert(g);
    assert(fp);
    assert(tok);

    memset(delim, 0, sizeof delim);
    for (; (c = *tok); tok++)
        ADDELIM(delim, c);

    str = g->str;
    len = g->len = 0;
    while ((c = fgetc(fp)) != EOF) {
        if (len >= g->cap) {
            size_t newcap = g->cap * 2;
            str = (char *) realloc(g->str, newcap);
            if (!str)
                return -ENOMEM;

            g->str = str;
            g->cap = newcap;
        }

        if (ISDELIM(delim, c))
            break;

        // else, store it and move on
        str[len++] = c;
    }
    str[len] = 0;

    return g->len = len;
}



/* EOF */
