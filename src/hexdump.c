/* :vi:ts=4:sw=4:
 *
 * File: hexdump.c - portable binary stream dump
 *
 * Copyright (c) 2005-2007 Sudhi Herle <sw at herle.net>
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
 *   Dump a buffer in 'hexdump -C' format.
 *   Code inspired by the golang encoding/hex::Dumper
 *   implementation.
 */
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include "utils/utils.h"

struct dumper {
    FILE *fp;

    int used;   // # of used bytes
    int n;      // # of total bytes
    char right[18];
    char buf[32];
};
typedef struct dumper dumper;

static const char hexchars[] = "0123456789abcdef";

static void
dumper_init(dumper *d, FILE *fp)
{
    memset(d, 0, sizeof *d);
    d->fp = fp;
}

static inline char
__tochar(uint8_t c)
{
    if (c < 32 || c > 126) {
        return '.';
    }
    return (char)c;
}

static char *
__do_byte(char *buf, uint8_t v)
{
    *buf++ = hexchars[v >> 4];
    *buf++ = hexchars[v & 0x0f];
    return buf;
}



// Write a maximum of 16 bytes
static char *
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

        default:
        case 4:
            buf = __do_byte(buf, z >> 24);
            buf = __do_byte(buf, z >> 16);

        case 2:
            buf = __do_byte(buf, z >>  8);
            buf = __do_byte(buf, z & 0xff);
    }
    return buf;
}

static int
__safewrite(FILE *fp, void *buf, size_t n)
{
    size_t m = fwrite(buf, n, 1, fp);
    if (m != 1) {
        return feof(fp) ? -EOF : ferror(fp);
    }
    return 0;
}

static int
dumper_write(dumper *d, uint8_t *buf, size_t bufsz)
{
    size_t i;
    char *p = d->buf;
    for (i = 0; i < bufsz; i++) {
        if (d->used == 0) {
            p = __encode_int(d->buf, d->n);
            *p++ = ' ';
            *p++ = ' ';
            *p = 0;

            int r = __safewrite(d->fp, d->buf, p - d->buf+1);
            if (r < 0) return r;
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

        int r = __safewrite(d->fp, d->buf, n);
        if (r < 0) return r;

        d->right[d->used] = __tochar(buf[i]);
        d->used++;
        d->n++;
        if (d->used == 16) {
            d->right[16] = '|';
            d->right[17] = '\n';

            int r = __safewrite(d->fp, d->right, sizeof d->right);
            if (r < 0) return r;
            d->used = 0;
        }
    }
    return 0;
}

static int
dumper_fini(dumper *d)
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

        int r = __safewrite(d->fp, d->buf, n);
        if (r < 0) return r;
        d->used++;
    }
    d->right[m] = '|';
    d->right[m+1] = '\n';
    int r = __safewrite(d->fp, d->right, m+2);
    if (r < 0) return r;
    return 0;
}



int
fhexdump(FILE *fp, void *buf, size_t bufsz)
{
    dumper d;
    int r = 0;

    dumper_init(&d, fp);
    if ((r = dumper_write(&d, pU8(buf), bufsz)) < 0) return r;
    if ((r = dumper_fini(&d)) < 0) return r;
    return 0;
}

// EOF
