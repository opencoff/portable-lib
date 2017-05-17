/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * inc/utils/seahash.h - Rust's Seahash Hash function
 *
 * Copyright (c) 2016 Sudhi Herle <sw at herle.net>
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

#ifndef ___INC_UTILS_SEAHASH_H_6023646_1480447807__
#define ___INC_UTILS_SEAHASH_H_6023646_1480447807__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <sys/types.h>

struct seahash_state
{
    uint64_t v[4];
    uint64_t n;
    uint64_t i;
};
typedef struct seahash_state seahash_state;


/*
 * Initialize an instance of seahash. 'init' is the Random IV; it
 * must either be a nullptr or an array of at least 4 64-bit words.
 */
void seahash_init(seahash_state *st, const uint64_t *init);

/*
 * Hash 'n' bytes of data in 'vbuf' and update the hash state.
 */
void seahash_update(seahash_state *st, const void * vbuf, size_t n);

/*
 * Finish the hash and return the resulting 64-bit value
 */
uint64_t seahash_finish(seahash_state *st);


/* Quick - buffer at a time interface */
static inline uint64_t
seahash_buf(const void * buf, size_t n, const uint64_t *init)
{
    seahash_state st;
    seahash_init(&st, init);
    seahash_update(&st, buf, n);
    return seahash_finish(&st);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___INC_UTILS_SEAHASH_H_6023646_1480447807__ */

/* EOF */
