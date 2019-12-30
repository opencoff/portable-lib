#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include "utils/gstring.h"

struct kv
{
    char * key;
    char * val;
};
typedef struct kv kv;


static const kv Vars[] = {
    {"abc", "foo"},
    {"ab",  "bar"},
    {"a",   "pqrf"},
    {"a-b c", "ABC"},
    {0, 0}
};

struct test
{
    int pass;
    const char *in;
    const char *exp;
};
typedef struct test test;

#define _P(a,b) {1, a, b}
#define _F(a,b) {0, a, b}

static const test Tests[] = {
    _P("$abc",   "foo"),
    _P("${abc}", "foo"),
    _P("$ab",    "bar"),
    _P("${ab}",  "bar"),
    _P("$a",     "pqrf"),
    _P("${a}",   "pqrf"),
    _P("${a-b c}", "ABC"),
    _P("$a ${a-b c}", "pqrf ABC"),
    _P("$abc def", "foo def"),
    _P("${abc} def", "foo def"),
    _P("${a} def",   "pqrf def"),
    _P("$a ${ab}def",   "pqrf bardef"),
    _P("$a\t${ab}def",   "pqrf\tbardef"),
    _P("\\$",        "$"),
    _P("\\$abc",     "$abc"),

    _F("$",    0),
    _F("${",   0),
    _F("${ab", 0),
    _F("${ab ",0),
    _F("$xyz", 0),
    _F("${z}", 0),
    _F("$   ", 0),
    _F("${  ", 0),
    _F("${}",  0),
    _F("${ }", 0),
    _F("$$",   0),

    {0, 0, 0}       // always keep last
};

static const char*
mapvar(void * vart, const char* key)
{
    const kv * v;

    (void)vart;

    for (v = &Vars[0]; v->key; v++) {
        if (0 == strcmp(v->key, key)) return v->val;
    }
    return 0;
}

int
main()
{
    const test* t = &Tests[0];

    for (; t->in; t++) {
        gstr g;
        int r;

        gstr_init_from(&g, t->in);
        r = gstr_varexp(&g, mapvar, 0);

        if (t->pass) {
            if (r != 0) {
                printf("%s: Failed. Expected pass <%s>\n", t->in, t->exp);
            } else if (gstr_len(&g) != strlen(t->exp)) {
                printf("%s: Failed. length mismatch exp <%s> vs. <%s>\n", t->in, t->exp, gstr_str(&g));
            } else if (0 != strcmp(gstr_str(&g), t->exp)) {
                printf("%s: Failed. value mismatch exp <%s> vs. <%s>\n", t->in, t->exp, gstr_str(&g));
            }
        } else {
            if (r == 0) {
                printf("%s: Passed. Expected to fail\n", t->in);
            } else if (gstr_len(&g) != 0) {
                printf("%s: Failed. length expected to be 0, saw %zd\n", t->in, gstr_len(&g));
            }
        }

        gstr_fini(&g);
    }
    return 0;
}
