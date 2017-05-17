/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/siphash.h - Siphash24 implementation.
 *
 * Original code Copyright (c) 2013  Marek Majkowski <marek@popcount.org>
 * Streaming interface Copyright (c) 2013 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 or MIT
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#ifndef ___UTILS_SIPHASH_H_6161138_1385833276__
#define ___UTILS_SIPHASH_H_6161138_1385833276__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h>
#include <stdint.h>

struct siphash24_state
{
    uint64_t v0, v1, v2, v3;
};
typedef struct siphash24_state siphash24_state;


/*
 * Streaming Interface
 *
 * Key is a 2 x 64-bit words (16-bytes) array.
 */
extern void siphash24_init(siphash24_state* st, const uint64_t *key);
extern void siphash24_update(siphash24_state* st, const void* data, size_t len);
extern uint64_t siphash24_finish(siphash24_state* st);


/* At-once interface
 *
 * Key is a 16-byte (2 x 64-bit words) array.
 */
extern uint64_t siphash24(const void* data, size_t len, const uint64_t* key);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_SIPHASH_H_6161138_1385833276__ */

/* EOF */
