/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/hexdump.h - portable hexdump ala 'hexdump -C'
 *
 * Copyright (c) 2024 Sudhi Herle <sw at herle.net>
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

#ifndef ___UTILS_HEXDUMP_H__HCS8ty7PficnFzXp___
#define ___UTILS_HEXDUMP_H__HCS8ty7PficnFzXp___ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/*
 * hex_dumper holds the state for a hexdump session.
 * Each session starts with hex_dumper_init() and ends
 * with hex_dumper_close().
 *
 * Callers can call hex_dumper_write() multiple times before calling
 * hex_dumper_close().
 */
struct hex_dumper {
    int (*out)(void *ctx, const char *buf, size_t n);
    void *ctx;
    unsigned int flags;
};
typedef struct hex_dumper hex_dumper;


// flags for hex_dumper

// Dump relative offsets and not the actual pointer value.
#define HEX_DUMP_PTR    (1 << 0)
#define HEX_DUMP_OFFSET (0)


/**
 * initialize a dumper with a custom write function 'writer'; the
 * 'ctx' is passed to 'writer' intact.
 *
 * Flags should be a bitwise-or of the flags above (HEX_DUMP_foo).
 *
 * The 'writer' should return 0 on success and negative number
 * (-errno) on error. Errors are passed back to the caller intact.
 */
void hex_dump_init(hex_dumper *d, int (*writer)(void *, const char*, size_t), void *ctx, unsigned int flags);

/**
 * Dump 'n' bytes of data from buffer 'buf'
 *
 * Returns 0 on success or the error from the writer.
 */
int hex_dump_write(hex_dumper *d, const void * buf, size_t n);

/**
 * flush any pending writes and close the dumper.
 *
 * Returns 0 on success or the error from the writer.
 */
int hex_dump_close(hex_dumper *d);


/**
 * Dump the contents of 'buf' to file 'fp' in the style of
 * 'hexdump -C'.  This functions implicitly only prints offsets.
 *
 * Returns:
 *      0       on success
 *      -errno  on error
 */
extern int fhexdump(FILE *fp, const void *buf, size_t bufsz);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___UTILS_HEXDUMP_H__HCS8ty7PficnFzXp___ */

/* EOF */
