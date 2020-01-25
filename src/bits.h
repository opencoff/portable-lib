/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * bits.h - token delimiters implemented as bits
 *
 * Copyright (c) 2019 Sudhi Herle <sw at herle.net>
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

#ifndef ___BITS_H__1alvw90M2xekUKib___
#define ___BITS_H__1alvw90M2xekUKib___ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
/*
 * We store token delimiters as bits in a 256-wide bitarray. We
 * compose this large bitarray from an array of uint32_t.
 *
 * Checking to see if a character is a delimiter then boils down to
 * checking if the appropriate bit within the long-word is set.
 */
#define xword          uint32_t
#define WORDBITS      (8 * sizeof(xword))
#define BITVECTSIZE   (256 / WORDBITS)
#define _word(c)      (((xword)(c)) / WORDBITS)
#define _bitp(c)      (1 << (((xword)(c)) % WORDBITS))

struct delim {
    xword    v[BITVECTSIZE];
};
typedef struct delim delim;

#define __init_delim(d)   memset((d)->v, 0, sizeof (d)->v)
#define __is_delim(d,c)   ((d)->v[_word(c)] &  _bitp(c))
#define __add_delim(d,c)  ((d)->v[_word(c)] |= _bitp(c))


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___BITS_H__1alvw90M2xekUKib___ */

/* EOF */
