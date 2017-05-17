/* :vim:ts=4:sw=4:
 *
 * gstring_var.c -  Variable expansion for gstring.
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
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include "utils/gstring.h"


/*
 * Find and collect a variable beginning at '*s' into 'v'. This
 * understands variables of the form:
 *     $VAR
 *     ${VAR}
 *
 * Return number of chars consumed.
 */
static int
getvar(gstr *v, char * s)
{
    int i = 0;
    int c = s[0];

    gstr_reset(v);
    
    // premature EOF - just a '$'. Eat it.
    if (c == 0) return 0;

    // We accept _everything_ within {}; i.e., we aren't particular
    // about spaces, special chars etc. Anything goes!
    if (c == '{') {    // we expect to see a closing brace.
        if (s[1] == 0) return 1;

        // We know that we have atleast two chars after the '$'
        for (i = 1; (c = s[i]); i++) {
            if (c == '}') return i+1;
            gstr_append_ch(v, c);
        }

        // didn't find a close brace. We skip the entire var
        gstr_reset(v);
        return i+1;
    }

    // Outside of {}, we expect vars to be specifically alphanumeric
    // ONLY. A non alphanumeric terminates the varname.
    for (i = 0; (c = s[i]); i++) {
        switch (c) {
            case '$': case '#': case '@': case '?': case '*': case '!':
            case '<': case '>': case '%': case '^': case '&': case '-':
            case '_': case '|': case ':': case '.':
                break;

            default:
                if (!isalnum(c)) goto out;
                break;
        }
        gstr_append_ch(v, c);
    }

out:
    return i;
}



// Expand variables in string 'g0' in-place. Use 'find()' to lookup
// key to value mappings.
// Returns:
//    0 on success
//    -ENOENT if variable can't be found
//    -EINVAL if string is malformed
int
gstr_varexp(gstr* g0, const char * (*find)(void *, const char* key), void* cookie)
{
    char * s = g0->str;
    int r    = 0;
    gstr g, v;

    gstr_init(&g, gstr_len(g0));
    gstr_init(&v, 64);

    int c = 0,
        prev;

    for (prev = c; (c = *s); prev = c, s++) {
        int n;
        const char* z;

        if (c != '$') {
            gstr_append_ch(&g, c);
            continue;
        }

        // Remove escaped '$' and insert literal
        if (prev == '\\') {
            g.str[g.len-1] = '$';
            continue;
        }

        // This could be a var. Expand it.
        n = getvar(&v, s+1);
        if (gstr_len(&v) == 0) { r = -EINVAL; goto done; }

        // var is in 'v.str'. Look it up.
        z = (*find)(cookie, v.str);
        if (!z) { r = -ENOENT; goto done; }

        gstr_append_str(&g, z);
        s += n;
    }

    // Finally, swap the two pointers
    char * gone = g0->str;

    g0->str = g.str;
    g0->len = g.len;
    g0->cap = g.cap;

    free(gone);

done:
    gstr_fini(&v);
    return r;
}

// EOF
