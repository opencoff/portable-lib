/* :vi:ts=4:sw=4:
 *
 * strsplit_csv.c -  Inspired by
 *  "Practice of Programming" --  (Kernighan and Pike)
 *
 * Copyright (c) 2005 Sudhi Herle <sw at herle.net>
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


typedef uint64_t word;
#define WORDSIZE      (8 * sizeof(word))
#define BITVECTSIZE   (256 / WORDSIZE)


typedef struct {
    word b[BITVECTSIZE];
} DELIM_TYPE;

#define INIT_DELIM(v)  memset((v)->b, 0, sizeof (v)->b)

static void
__add_delim(DELIM_TYPE *v, uint32_t c)
{
    word w = c / WORDSIZE;
    word b = c % WORDSIZE;
    v->b[w] |= (1 << b);
}



static int
__is_delim(DELIM_TYPE *v, uint32_t c)
{
    word w = c / WORDSIZE;
    word b = c % WORDSIZE;
    return 0 != (v->b[w] & (1 << b));
}

#define ADD_DELIM(v,c)  __add_delim(v, ((word)c))
#define IS_DELIM(v,c)   __is_delim(v,  ((word)c))



static void
PRINT_DELIM(DELIM_TYPE *v)
{
    int i;
    int n = 0;
    fputc(' ', stdout);
    for (i = 0; i < 256; i++) {
        fputc(IS_DELIM(v, i) ? '1': '.', stdout);
        ++n;

        if ((n % 4) == 0) fputc(' ', stdout);
        if ((n % 8) == 0) fputc(' ', stdout);
        if ((n % 64)  == 0) {
            fputc('\n', stdout);
            fputc(' ', stdout);
        }
    }
    fputs("---\n", stdout);
}


int
main()
{
    DELIM_TYPE v;

    INIT_DELIM(&v);

    ADD_DELIM(&v, ',');
    PRINT_DELIM(&v);

    assert(IS_DELIM(&v, ','));
    return 0;
}
