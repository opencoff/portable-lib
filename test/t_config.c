/*
 * Test harness for configfile.c
 */
#include <stdio.h>

#include "utils/configfile.h"
#include "error.h"

extern void tconfig(const char*);

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


static void
repl(configfile* cf)
{
    gstr g;

    gstr_init(&g, 128);
    printf("(config) ");
    while (readline(&g, stdin)) {
        const char* e = config_get_entry(cf, gstr_str(&g));
        if (e) {
            printf("%s\n", e);
        } else {
            node* n = config_get_section(cf, gstr_str(&g));
            if (!n) {
                printf("!NOT FOUND\n(config) ");
                continue;
            }

            // now print the leaves
            keyval* kv;
            VECT_FOR_EACH(&n->values, kv) {
                printf("%s = %s\n", gstr_str(&kv->key), gstr_str(&kv->val));
            }
        }
        printf("(config) ");
    }
}


int
main(int argc, char* argv[])
{
    if (argc < 2)
        error(1, 0, "Usage: %s CONFIG-FILE", argv[0]);

    configfile cf;

    config_open(&cf, argv[1]);
    config_dump(&cf);

    repl(&cf);

    config_close(&cf);

    return 0;
}
