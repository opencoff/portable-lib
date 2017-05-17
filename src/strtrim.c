/* :vim:ts=4:sw=4:
 *
 * strtrim.c -  Trim a string of leading & trailing white spaces.
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
 * Creation date: Sun Aug 22 19:18:19 1999
 */
#include "utils/utils.h"
#include <ctype.h>


/*
 * Remove all leading & trailing white-spaces from input string inplace.
 */
char *
strtrim(char *in_str)
{
    unsigned char  *str = (unsigned char *) in_str,
                   *ptr,
                   *str_begin,
                   *str_end;
    unsigned int c;

    if (!str)
        return 0 ;

    /* Remove leading spaces */
    for (str_begin = str; (c = *str); str++)
    {
        if ( isspace (c) )
            continue ;
        
        /* Copy from str_begin to end --skipping trailing white
           spaces */

        str_end = str_begin;
        for (ptr = str_begin; (c = *ptr = *str); str++, ptr++)
        {
            if ( ! isspace (c) )
                str_end = ptr;
        }
        *(str_end + 1) = 0;
        return (char *) str_begin;
    }


    *str_begin = 0;
    return (char *) str_begin;
}

#ifdef  TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define     fatal(msg)  do { \
        fprintf(stderr, "Test failed! --%s\n", (msg)) ;\
        exit(1) ; \
        } while (0)

int
main ()
{
    char a[32] = "abcdef";
    char b[32] = "  abcdef";
    char c[32] = "   abcdef   ";
    char d[32] = "";
    char e[32] = "    ";
    char f[32] = "  \"알림간격\"  ";

    if (strcmp ("abcdef", strtrim (a)) != 0)
        fatal ("Failed on ``a''");
    if (strcmp ("abcdef", strtrim (b)) != 0)
        fatal ("failed on ``b''");
    if (strcmp ("abcdef", strtrim (c)) != 0)
        fatal ("failed on ``c''");
    if (strcmp ("", strtrim (d)) != 0)
        fatal ("failed on ``d''");
    if (strcmp ("", strtrim (e)) != 0)
        fatal ("failed on ``e''");
    if (strcmp ("\"알림간격\"", strtrim (f)) != 0)
        fatal ("failed on ``f''");

    exit(0) ;
}

#endif
        /* EOF */
