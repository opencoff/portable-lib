/* :vi:ts=4:sw=4:
 *
 * File: hexdump.c - portable binary stream dump
 *
 * Copyright (c) 2001-2024 golang authors
 * 'C' reimplementation  Copyright (c) 2024- Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: Same as golang sources.
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
#include "utils/utils.h"


static const char hexchars[] = "0123456789abcdef";

static inline char
__tochar(uint8_t c)
{
    if (c < 32 || c > 126) {
        return '.';
    }
    return (char)c;
}

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
__encode_int(char *out, int n)
{
    uint64_t z = (uint64_t)n;
    char * buf = out;

    switch (sizeof(int)) {
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
__fp_writer(void *ctx, void *buf, size_t n)
{
    FILE *fp = ctx;
    size_t m = fwrite(buf, n, 1, fp);
    if (m != 1) {
        return feof(fp) ? -EOF : ferror(fp);
    }
    return 0;
}


// Initialize a dumper instance using the output function 'writer'
// and it's context 'ctx'.
void
hex_dumper_init(hex_dumper *d, int (*writer)(void *ctx, void *buf, size_t n), void *ctx)
{
    memset(d, 0, sizeof *d);
    d->writer = writer;
    d->ctx    = ctx;
}

#define __out(d, buf, n) do {                        \
            hex_dumper *_d = d;                      \
            int _r = (*_d->writer)(_d->ctx, buf, n); \
            if (_r < 0) return _r;                   \
        } while (0)


// Dump 'nbytes' of data from 'vbuf' into the dumper instance 'd'
int
hex_dumper_write(hex_dumper *d, void *vbuf, size_t nbytes)
{
    size_t i     = 0;
    char *p      = d->buf;
    uint8_t *buf = vbuf;

    for (i = 0; i < nbytes; i++) {
        if (d->used == 0) {
            p = __encode_int(d->buf, d->n);
            *p++ = ' ';
            *p++ = ' ';

            __out(d, d->buf, p - d->buf);
        }

        p = __do_byte(d->buf, buf[i]);
        *p++ = ' ';
        size_t n  = 3;

        if (d->used == 7) {
            *p++ = ' ';
            n = 4;
        } else if (d->used == 15) {
            *p++ = ' ';
            *p++ = '|';
            n = 5;
        }

        __out(d, d->buf, n);
        d->right[d->used] = __tochar(buf[i]);
        d->used++;
        d->n++;
        if (d->used == 16) {
            d->right[16] = '|';
            d->right[17] = '\n';

            __out(d, d->right, sizeof d->right);
            d->used = 0;
        }
    }
    return 0;
}

// flush pending data and complete the dump session.
int
hex_dumper_close(hex_dumper *d)
{
    if (d->used == 0) return 0;

    d->buf[0] = ' ';
    d->buf[1] = ' ';
    d->buf[2] = ' ';
    d->buf[3] = ' ';
    d->buf[4] = '|';

    int m = d->used;
    while (d->used < 16) {
        size_t n = 3;
        if (d->used == 7) {
            n = 4;
        } else if (d->used == 15) {
            n = 5;
        }

        __out(d, d->buf, n);
        d->used++;
    }
    d->right[m++] = '|';
    d->right[m++] = '\n';
    __out(d, d->right, m);
    return 0;
}



// convenience wrapper to dump 'nbytes' from 'buf' into FILE 'fp'.
int
fhexdump(FILE *fp, void *buf, size_t nbytes)
{
    hex_dumper d;
    int r = 0;

    hex_dumper_init(&d, __fp_writer, fp);

    if ((r = hex_dumper_write(&d, pU8(buf), nbytes)) < 0) return r;
    if ((r = hex_dumper_close(&d)) < 0) return r;
    return 0;
}

// EOF
