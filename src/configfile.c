/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * configfile.h - Pythonic config file processing.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "utils/gstring.h"
#include "fast/list.h"
#include "fast/vect.h"
#include "error.h"
#include "utils/configfile.h"
#include "utils/parse-ip.h"
#include "utils/utils.h"


// Stack 
VECT_TYPEDEF(node_vect, struct node*);


#if 0
#define DBG(a)  printf a
#else
#define DBG(a)
#endif


// take a string of white spaces and figure out its logical
// "col number". We use this calculate the indentation level.
static int
cols(gstr* g, int* non_ws)
{
    int i = 0;
    int c = 0;

#define TABSZ       4

    for (i = 0; i < gstr_len(g); ++i) {
        int x = gstr_ch(g, i);
        if (x == ' ')       c++;
        else if (x == '\t') c = (c / TABSZ + 1) * TABSZ;
        else break;
    }

    *non_ws = i;
    return c;
}


// Return False if length is zero _and_ EOF
// Return True oterhwise.
static int
readline(gstr* g, FILE* fp)
{
    int c;
    gstr_reset(g);

    while (1) {
        c = fgetc(fp);
        if (c == EOF)
            break;

        if (c == '\r') {
            int next = fgetc(fp);
            if (next != EOF && next != '\n')
                ungetc(next, fp);

            break;
        } else if (c == '\n')    break;
        else   gstr_append_ch(g, c);
    }

    return gstr_len(g) > 0 || c != EOF;
}



// Create a new node and add it to 'parent'.
static node*
newnode(node* parent, gstr* g, int col)
{
    node* n = NEWZ(node);

    assert(n);
    assert(parent);

    gstr_trim(g);

    n->col    = col;
    n->master = ':' == gstr_chop_if(g, ":");

    if (n->master) {
        gstr_init2(&n->name, g);
        VECT_INIT(&n->values, 4);
        DL_INIT(&n->subtree);

        DL_APPEND(&parent->subtree, n, link);
    } else {
        char* v = "1";
        char* s = gstr_str(g);
        char* e = strchr(s, '=');

        if (!e)
            e = strchr(s, ' ');

        if (e) {
            *e = 0;

            v = e+1;
            strtrim(v);
        }
        strtrim(s);

        keyval kv;

        gstr_init_from(&kv.key, s);
        gstr_init_from(&kv.val, v);

        VECT_APPEND(&parent->values, kv);
    }

    return n;
}


// Parse the file of config info
static void
parse(configfile* cf, FILE* fp)
{
    gstr _g;
    gstr* g = &_g;
    gstr  l;
    int lineno = 0;

    // Intermediate parsing stuff
    node*     top;      // top of stack;
    node*     prev;     // previous node
    int       prevcol;  // previous column
    node_vect st;       // stack

    gstr_init(g,  128);
    gstr_init(&l, 128);

    VECT_INIT(&st, 16);

    top = prev = NEWZ(node);

    gstr_init_from(&top->name, "<ROOT>");
    top->master = 1;
    top->col    = -1;
    prevcol     = 0;

    VECT_INIT(&top->values, 8);
    DL_INIT(&top->subtree);

    cf->root = top;

    while (readline(g, fp)) {
        lineno++;

        if (gstr_len(g) == 0)
            continue;

        if (gstr_chop_if(g, "\\") == '\\') {
            // gstr_trim(g);
            gstr_append(&l, g);
            continue;
        }

        gstr_append(&l, g);

        int i = 0;
        int c = cols(&l, &i);

        if (gstr_ch(&l, i) == '#') {
            gstr_reset(&l);
            continue;
        }

        if (c > prevcol) {
            if (!prev->master)
                error(1, 0, "%s:%d: Indented node '%12.12s..' has no parent?", cf->name, lineno, gstr_str(&l));

            VECT_PUSH_BACK(&st, top);
            top = prev;

        } else if (c < prevcol) {
            node* t = 0;
            while (VECT_SIZE(&st) > 0) {
                t = VECT_POP_BACK(&st);
                if (t->col < c) goto ok;
            }
            error(1, 0, "%s:%d: Indent stack corrupted while scanning '%12.12s..'", cf->name, lineno, gstr_str(&l));

        ok:
            top = t;
        }

        node* n = newnode(top, &l, c);
        prev    = n;
        prevcol = c;
        gstr_reset(&l);
    }

    // XXX Any post processing?

    VECT_FINI(&st);
    gstr_fini(&l);
    gstr_fini(g);
}



static void
_dumpx(node* n, int lev)
{
    size_t ws = 4 * lev;
    char* z   = alloca(ws + 1);
    keyval* kv;

    if (ws > 0) memset(z, ' ', ws);
    z[ws] = 0;

    assert(n->master);

    printf("%s %s:\n", z, gstr_str(&n->name));

    VECT_FOR_EACH(&n->values, kv) {
        printf("%s     %s=%s\n", z, gstr_str(&kv->key), gstr_str(&kv->val));
    }

    node* p;
    DL_FOREACH(&n->subtree, p, link) {
        _dumpx(p, lev+1);
    }
}

// Recursively free every node
static void
xfree(node* n)
{
    keyval* kv;
    node*   p;

    VECT_FOR_EACH(&n->values, kv) {
        gstr_fini(&kv->key);
        gstr_fini(&kv->val);
    }

    p = DL_FIRST(&n->subtree);
    while (p) {
        node* x = DL_NEXT(p, link);
        DL_REMOVE(&n->subtree, p, link);

        xfree(p);
        DEL(p);

        p = x;
    }

    VECT_FINI(&n->values);
    gstr_fini(&n->name);
}


void
config_dump(configfile* cf)
{
    _dumpx(cf->root, 0);
}


int
config_open(configfile* cf, const char* name)
{
    memset(cf, 0, sizeof *cf);

    cf->name = name;

    FILE* fp = fopen(name, "rb");
    if (!fp)
        error(1, errno, "Can't open config file %s", name);

    parse(cf, fp);

    fclose(fp);
    return 0;
}




// Free the memory
int
config_close(configfile* cf)
{
    assert(cf->root);
    xfree(cf->root);
    DEL(cf->root);

    memset(cf, 0, sizeof *cf);
    return 0;
}


// -- Search interface --

// Find 'p' in the subtree rooted at 'h'
// Return pointer to that subtree.
static inline node*
find_subtree(node_head* h, const char* p) {
    node* n;
    DL_FOREACH(h, n, link) {
        if (gstr_eq_str(&n->name, p))   return n;
    }

    return 0;
}


// Walk path array 'p' starting at 'start' and make sure every
// component of 'p' can be matched to an entry in the subtree rooted
// at 'start'
//
// Return the last matching node.
static inline node*
find(node* start, char** p, int r)
{
    node* n = start;
    int i;

    for (i = 0; i < r; ++i) {
        n = find_subtree(&n->subtree, p[i]);
        if (!n) return 0;
    }

    return n;
}


static node*
walk(node* start, const char* path)
{
    size_t m = strlen(path) + 1;
    char*  s = alloca(m);
    memcpy(s, path, m);

    char * p[32];
    int r = strsplit_quick(p, 32, s, "/", 1);
    if (r <= 0)     return 0;

    return find(start, p, r);
}

// Return the master node for this search string.
// Paths are separated by "/"
node*
config_get_section(configfile* cf, const char* path)
{
    if (0 == strcmp("/", path)) return cf->root;
    if ('/' == *path)           path++;

    return walk(cf->root, path);
}


// Return master node beginning at 'start'
node*
config_get_subsection(node* start, const char* path)
{
    if ('/' == *path) path++;

    return walk(start, path);
}


// Find a leaf node starting with 'path'
static const char*
find_leaf(node* start, const char* path)
{
    size_t m = strlen(path) + 1;
    char*  s = alloca(m);
    memcpy(s, path, m);


    char * p[32];
    int r = strsplit_quick(p, 32, s, "/", 1);
    if (r <= 0)     return 0;

    char* leaf = p[--r];    // penultimate node
    node* n    = r == 0 ? start : find(start, p, r);
    if (n) {
        // we have reached the dir entry we need. Now, we look in the
        // values.
        keyval* kv;
        VECT_FOR_EACH(&n->values, kv) {
            if (gstr_eq_str(&kv->key, leaf))    return gstr_str(&kv->val);
        }
    }

    return 0;
}


// Get a leaf entry following a path beginning at 'start'
// Any absolute paths are ignored and treated as if they are
// "rooted" at 'start'.
const char*
config_section_get_entry(node* start, const char* path)
{
    if ('/' == *path) path++;
    if (!*path)       return 0;

    return find_leaf(start, path);
}


// Get a leaf node by traversing a path that looks like "/a/b/c"
const char*
config_get_entry(configfile* cf, const char* path)
{
    return find_leaf(cf->root, path);
}



// -- Parsing individual values --
int
config_parse_int(const char* s, long* val)
{
    char* e = 0;

    errno  = 0;
    long z = strtol(s, &e, 0);
    if (errno != 0)
        return -errno;

    *val = z;
    return 0;
}


int
config_parse_uint(const char* s, unsigned long* val)
{
    char * endptr = 0;
    uint64_t mult = 1;
    uint64_t v;

#ifdef _MSC_VER
// MS is weird. They deliberately choose NOT to use function names that
// the rest of the world uses.
#define strtoul(a,b,c)  _strtoui64(a,b,c)
#define _ULLCONST(n) n##ui64
#else

#define _ULLCONST(n) n##ULL

#endif
#define KB_  _ULLCONST(1024)
#define MB_  (KB_ * 1024)
#define GB_  (MB_ * 1024)
#define TB_  (GB_ * 1024)
#define PB_  (TB_ * 1024)
#define EB_  (PB_ * 1024)


    errno = 0;
    v     = strtoul(s, &endptr, 0);
    if (errno != 0)
        return -errno;

    if ( endptr && *endptr )
    {
        switch (*endptr)
        {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
                break;

            case 'k': case 'K':
                mult = KB_;
                break;

            case 'M':
                mult = MB_;
                break;

            case 'G':
                mult = GB_;
                break;

            case 'T':
                mult = TB_;
                break;

            case 'P':
                mult = PB_;
                break;

            case 'E':
                mult = EB_;
                break;

            default:
                return -EINVAL;
        }
    }

    uint64_t z = v * mult;
    if (z < v)
        return -EOVERFLOW;

    *val = z;
    return 0;
}


int
config_parse_bool(const char* s, int* val)
{
    *val =  (0 == strcasecmp(s, "on")   ||
             0 == strcasecmp(s, "true") ||
             0 == strcasecmp(s, "1")) ? 1 : 0;
    return 0;
}



/* EOF */
