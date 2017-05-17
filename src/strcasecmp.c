/* :vim:ts=4:sw=4:
 *
 * strcasecmp.c - portable implementation of strcasecmp() and
 *                strncasecmp(). Useful when some platforms don't
 *                have this.
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
#include <stdlib.h>


/* Assumes the ASCII collating sequence */

static const unsigned char _case_independent_map[256] =
{
     0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
    16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
    ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
    '@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '[', '\\', ']', '^', '_',
    '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', 127,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};



/*
 * Function    :  strcasecmp
 * Description :  Perform a case insensitive comparison
 * Parameters  :  (in)   register char *str1
 *                (in)   register char *str2
 * Return      :  -1 if str1 <  str2
 *                 1 if str1 >  str2
 *                 0 if str1 == str2
 */
int
strcasecmp(const char *str1, const char *str2)
{
    const unsigned char *map = _case_independent_map,
                        *l = (const unsigned char *) str1,
                        *r = (const unsigned char *) str2;
    int c;

    for (; map[*l] == (c = map[*r]); l++, r++)
    {
        if ( !c )
            return 0;
    }
    return map[*r] - map[*l];
}



/*
 *  Function    :  strncasecmp
 *  Description :  Perform a case insensitive comparison like
 *              :  strncmp, but case-insensitive!
 *  Parameters  :  (in) str1
 *                 (in) str2
 *                 (in) length
 *  Return      :  -1 if str1 <  str2
 *                  1 if str1 >  str2
 *                  0 if str1 == str2
 */

int
strncasecmp(const char *str1, const char *str2, size_t length)
{
    const unsigned char *map = _case_independent_map ;
    int c1 = 0, c2 = 0;
    int l, r;

    for (; length > 0;length--, str1++, str2++)
    {
        l = map[c1 = *str1];
        r = map[c2 = *str2];

        if ( !(c1 && c2) )
            goto _done;

        if (l != r)
            goto _done;
    }
    return 0;

_done:
    return l - r;
}



#ifdef TEST
int
main ()
{
    char *s = "abcDef*#123456789";
    char *t = "ABCdef#*234567890";

    if ( 0 == strcasecmp (s, t) )
        printf ("BROKEN!\n");
    return 0;
}
#endif /* TEST */

/* EOF */
