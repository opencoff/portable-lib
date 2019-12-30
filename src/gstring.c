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
#include "bits.h"

/* Grow string `g' by `len' bytes -- if necessary. */
static int grow_if(gstr * g, int len);


/* Grow the gstr by additional 'len' bytes -- if necessary. */
static int
grow_if(gstr * g, int len)
{
    size_t want = g->len + len,
           have = g->cap;
    char * str;

    if (have > want) return 0;

    while (have < want) have *= 2;

    str = RENEWA(char, g->str, have);
    assert(str);

    g->str = str;
    g->cap = have;
    return 0;
}


static inline gstr *
__init_gstr(gstr *g, size_t n, char *src)
{
    if (n == 0) n = 127;

    g->cap = NEXTPOW2(n+1);
    g->str = NEWZA(char, g->cap);
    assert(g->str);

    if (src) {
        memcpy(g->str, src, n+1);
        g->len = n;
    } else {
        g->len = 0;
    }
    return g;
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

    return __init_gstr(g, size, 0);
}


gstr*
gstr_init2(gstr* g, const gstr* s)
{
    assert(g);
    assert(s);

    return __init_gstr(g, s->len, s->str);
}


/*
 * Initialize from nul terminated string 's'
 */
gstr*
gstr_init_from(gstr* g, const char* s)
{
    size_t n = s ? strlen(s) : 0;
    return __init_gstr(g, n, s);
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
        if (c == '\n') g->str[--g->len] = 0;
        if (g->len > 0) {
            int cr = g->str[g->len-1];
            if (cr == '\r') {
                c = cr;
                g->str[--g->len] = 0;
            }
        }
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
    assert(s);

    if (g->len == 0) return 0;

    int c = g->str[g->len-1];

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
    delim delim;
    char * str;
    size_t len;
    int c;

    assert(g);
    assert(fp);
    assert(tok);

    __init_delim(&delim);
    for (; (c = *tok); tok++)
        __add_delim(&delim, c);

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

        if (__is_delim(&delim, c))
            break;

        // else, store it and move on
        str[len++] = c;
    }
    str[len] = 0;

    return g->len = len;
}



/* EOF */
