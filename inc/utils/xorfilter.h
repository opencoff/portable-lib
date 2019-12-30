/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * xorfilter.h - Better than Bloom & Cuckoo Filters
 *
 * An independent implementation of:
 *  "Xor Filters: Faster and Smaller Than Bloom and Cuckoo Filters"
 *  https://arxiv.org/abs/1912.08258
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
 *
 */

#ifndef ___XORFILTER_H__4dSwjy8YTI73DQsx___
#define ___XORFILTER_H__4dSwjy8YTI73DQsx___ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// Forward Decl: Opaque struct for callers
struct Xorfilter;
typedef struct Xorfilter Xorfilter;

/*
 * Create and return a new 8-bit Xorfilter using 'n' keys in 'elem'.
 */
extern Xorfilter* Xorfilter_new8(uint64_t *elem, size_t n);

/*
 * Create and return a new 16-bit Xorfilter using 'n' keys in 'elem'.
 */
extern Xorfilter* Xorfilter_new16(uint64_t *elem, size_t n);

/* Delete and free memory associated with Xorfilter */
extern void Xorfilter_delete(Xorfilter *);

/*
 * Return true if the Xorfilter contains element 'x' and false
 * otherwise.
 */
extern int Xorfilter_contains(Xorfilter *, uint64_t x);

/* Return number of bits per element in this XOrfilter */
extern double Xorfilter_bpe(Xorfilter *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___XORFILTER_H__4dSwjy8YTI73DQsx___ */

/* EOF */
