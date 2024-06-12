/* :vi:ts=4:sw=4:
 *
 * File: hexdump.c - portable binary stream dump
 *
 * Copyright (c) 2024 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 *   Dump a buffer in 'hexdump -C' format.
 *   Code inspired by the golang encoding/hex::Dumper
 *   implementation.
 */
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "utils/hexdump.h"

static const char hexchars[]   = "0123456789abcdef";

// encode one byte into hex
static inline char *
__do_byte(char *buf, uint8_t v)
{
    *buf++ = hexchars[v >> 4];
    *buf++ = hexchars[v & 0x0f];
    return buf;
}

// encode an int of the right size; this is the 'offset' at the
// start of the line.
static inline char *
__encode_int(char *buf, size_t z)
{
    switch (sizeof(size_t)) {
        case 8:
            buf = __do_byte(buf, z >> 56);
            buf = __do_byte(buf, z >> 48);
            buf = __do_byte(buf, z >> 40);
            buf = __do_byte(buf, z >> 32);
            // fallthrough
        default:
            // fallthrough
        case 4:
            buf = __do_byte(buf, z >> 24);
            buf = __do_byte(buf, z >> 16);
            // fallthrough

        case 2:
            buf = __do_byte(buf, z >>  8);
            buf = __do_byte(buf, z & 0xff);
            break;
    }
    return buf;
}

static int
__fp_writer(void *ctx, const char *buf, size_t n)
{
    FILE *fp = ctx;
    size_t m = fwrite(buf, n, 1, fp);
    if (m != 1) {
        return feof(fp) ? -EOF : ferror(fp);
    }
    return 0;
}


// hexdump to a file stream 'fp'
int
fhexdump(FILE *fp, const void *buf, size_t n)
{
    hex_dumper d;
    int r = 0;

    hex_dump_init(&d, __fp_writer, fp, HEX_DUMP_OFFSET);

    if ((r = hex_dump_write(&d, buf, n)) < 0) return r;

    return hex_dump_close(&d);
}


// initialize a hexdump session with a custom output writer.
void
hex_dump_init(hex_dumper *d, int (*out)(void *, const char*, size_t), void *ctx, unsigned int flags) {
    memset(d, 0, sizeof *d);
    d->out   = out;
    d->ctx   = ctx;
    d->flags = flags;
}


// hexdump 'bufsiz' bytes from 'buf'.
int
hex_dump_write(hex_dumper *d, const void * buf, size_t bufsiz)
{
    const uint8_t *p  = (const uint8_t*)buf;
    size_t rem        = bufsiz;
    size_t off        = 0;
    char bb[128];

    if ((HEX_DUMP_PTR & d->flags) > 0) {
        off = (size_t)p;
    }

    while (rem > 0) {
        memset(bb, ' ', sizeof bb);

        size_t z    = 0;
        size_t n    = rem > 16 ? 16 : rem;
        char *ptr   = __encode_int(&bb[0], off);
        char *ascii = 0;

        *ptr++ = ' ';
        *ptr++ = ' ';
        ascii  = ptr + 1 + ((8 * 2 + 7) * 2) + 4;
        rem   -= n;
        off   += 16;

        *ascii++ = '|';
        for (z = 0; z < n; z++, p++) {
            unsigned char c = *p;

            ptr      = __do_byte(ptr, c);
            *ascii++ = isprint(c) ? c : '.';
            *ptr++   = ' ';

            if (z == 7 || z == 15) *ptr++ = ' ';
        }
        *ascii++ = '|';
        *ascii++ = '\n';
        *ascii   = 0;

        int r = (*d->out)(d->ctx, &bb[0], ascii-&bb[0]);
        if (r < 0) return r;
    }
    return 0;
}

// close the hexdump session
int
hex_dump_close(hex_dumper *d)
{
    memset(d, 0, sizeof *d);
    return 0;
}

// EOF
