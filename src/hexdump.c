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
 *   Dump a full/partial byte stream in raw format.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include "utils/utils.h"

/* Shorthand */
#define pU8(x)  ((uint8_t *)(x))





#ifdef TEST

#if 0
#define DIAG(a) printf a
#else
#define DIAG(a)
#endif


#else
#define DIAG(a)
#endif /* TEST */




typedef struct dump_info dump_info;
struct dump_info
{
    uint8_t  * buf,
             * end;

    char * outbuf,
         * lineptr,
         * ascii;

    /* Current offset */
    ptrdiff_t  block;


    void (*output) (void *, const char *, int);
    void * cookie;
};


/* Output to string buffer */
struct outbuf
{
    char* buf;
    char *end;
};




/* Handy macros to cvt a hex number to it's string
 * representation --a nibble at a time. */
static const char  _hexmap [] = "0123456789abcdef";
#define lo4(x)  (_hexmap[0xf & (x)])
#define hi4(x)  (_hexmap[(x) >> 4])


/* Dump format:
          10        20        30        40        50        60        70
012345678901234567890123456789012345678901234567890123456789012345678901234567890
off       00 01 02 03 04 05 06 07    08 09 0a 0b 0c 0d 0e 0f    ascii
00000000  00 01 02 03 04 05 06 07    08 09 0a 0b 0c 0d 0e 0f    12345678 90123456
00000010  00 01 02 03 04 05 06 07    08 09 0a 0b 0c 0d 0e 0f    12345678 90123456
 */


static int
__rem(dump_info * d)
{
    return d->end - d->buf;
}

static void
__init_ptr(dump_info * d)
{
    d->lineptr = d->outbuf;
    d->ascii   = d->lineptr + 61;
}


/**
 * Write the current output buffer via the callback function.
 * Reset the accumulator buffer and increment the block
 * number.
 */
static void
__output(dump_info * d, int n)
{
    (*d->output)(d->cookie, d->outbuf, n);

    d->block += 16;
    __init_ptr(d);
}


/**
 * Do at most 8 bytes of data.
 *
 * @param   skip    number of bytes to skip before inserting
 *                  padding.
 * @param   nbytes  number of bytes to actually write
 * @param   lastch  Last ASCII char to insert.
 *
 * Pre-condition: 0 < n <= 8 && 0 <= skip <= 15
 */
static void
__do_atmost_8 (dump_info * d, int skip, int nbytes, int lastch)
{
    int avail   = __rem(d),
        chunk   = avail > nbytes ? nbytes : avail;
    uint8_t * bufend = d->buf + chunk;

    assert(nbytes > 0 && nbytes <= 8);
    assert(skip  >= 0 && skip   <= 15);

    for (; d->buf < bufend; d->buf++, d->ascii++)
    {
        int c = *d->buf;

        *d->lineptr++ = hi4(c);
        *d->lineptr++ = lo4(c);
        *d->lineptr++ = ' ';
        if ( isprint(c) )
            *d->ascii = c;
        else
            *d->ascii = '.';
    }

    /* Add padding for bytes that don't exist. */
    if ( (skip+chunk) < 8 )
    {
        int n = 8 - chunk - skip;
        for (; n > 0; --n)
        {
            *d->lineptr++ = ' ';
            *d->lineptr++ = ' ';
            *d->lineptr++ = ' ';
            *d->ascii++   = ' ';
        }
    }

    /* Spacer bytes at the end of each 8-byte chunk. */
    *d->lineptr++ = ' ';
    *d->ascii++   = lastch;
}



/**
 * Output 16 bytes (or less) of data starting at an aligned address
 * in 'd->buf',
 */
static void
__do_one_line(dump_info * d)
{
    unsigned long o = (unsigned long)d->block;
    d->lineptr = d->outbuf;
    d->ascii   = d->lineptr + 61;    /* The ascii representation starts here */

    DIAG(("one-line: lineptr=%p ascii=%p ", d->lineptr, d->ascii));

    /* Write the offset first */
    d->lineptr += sprintf(d->lineptr, "%8.8lx  ", o);

    __do_atmost_8(d, 0, 8, ' ');
    __do_atmost_8(d, 0, 8, '\n');
    *d->ascii = 0;

    DIAG((";ascii=%p len=%d\n", d->ascii, d->ascii-d->outbuf));

    __output(d, d->ascii - d->outbuf);
}



/**
 * Add padding space for 'n' bytes of input data.
 * Each byte of input data consumes:
 *  2 hex chars
 *  1 space
 *  1 ascii char
 *
 * In addition, if the padding bytes > 8, we have to add 4 more
 * bytes of extra space (divider)
 */
static uint32_t
__do_padding(dump_info * d, uint32_t npad)
{
    unsigned long o           = (unsigned long)d->block;
    uint32_t white_space = npad * 3;
    uint32_t nascii      = npad;

    if ( npad >= 8 )
    {
        white_space += 1;
        ++nascii;
    }

    d->lineptr += sprintf(d->lineptr, "%8.8lx  ", o);
    memset(d->lineptr, ' ', white_space);
    memset(d->ascii, ' ', nascii);

    d->lineptr += white_space;
    d->ascii   += nascii;

    DIAG(("do-padding(%lu): whitespace=%lu, ascii-chars=%lu\n",
                npad, white_space, nascii));

    return 16 - npad;
}



void
hex_dump_bytes(const void * inbuf, int len,
               void (*output) (void *, const char * line, int nbytes),
               void * cookie, const char* msg, int offsetonly)
{
    static const char _first_line[] =
        "off       00 01 02 03 04 05 06 07"
        "  08 09 0a 0b 0c 0d 0e 0f   ascii\n"
        "---------------------------------------"
        "---------------------------------------\n";
    char outbuf[96];
    dump_info d;
    int n;

    memset(&d, 0, sizeof(d));

    if (msg)
        (*output)(cookie, msg, strlen(msg));

    if (offsetonly)
    {
        n        = snprintf(outbuf, sizeof outbuf, "length %d bytes\n", len);
        d.block  = 0;
    }
    else
    {
        n = snprintf(outbuf, sizeof outbuf, "buffer  %p, length %d bytes\n", inbuf, len);
        d.block  = _U64(inbuf) & ~15;
    }

    (*output)(cookie, outbuf, n);

    memset(outbuf, ' ', sizeof(outbuf));

    d.buf    = pU8(inbuf);
    d.end    = d.buf + len;
    d.outbuf = outbuf;
    d.output = output;
    d.cookie = cookie;

    DIAG(("dump %p..+%d => %p..%p\n",
                d.buf, len, d.buf, d.end));

    if (msg)
        (*output)(cookie, msg, strlen(msg));

    /* Write the header line in raw form. */
    (*output)(cookie, _first_line, (sizeof _first_line)-1);

    /*
     * Output the first 'n' unaligned bytes.
     */
    if ( _U64(inbuf) & 15 )
    {
        uint32_t skip = _U64(inbuf) & 15;
        uint32_t rem;

        DIAG(("** ptr=%p, padding=%lu, nbytes=%lu\n",
                inbuf, skip, 16 - skip));


        __init_ptr(&d);

        rem   = __do_padding(&d, skip);
        if ( rem > 8 )
        {
            DIAG(("** rem %lu; doing first %lu\n",
                        rem, rem - 8));
            __do_atmost_8(&d, skip, rem - 8, ' ');
            __do_atmost_8(&d, 0, 8, '\n');
        }
        else
            __do_atmost_8(&d, 0, rem, '\n');

        *d.ascii = 0;
        __output(&d, d.ascii - d.outbuf);
    }


    for (d.lineptr = outbuf; d.buf < d.end; d.lineptr = outbuf)
    {
        __do_one_line(&d);
    }
}

/* output function callback that writes to file 'f' */
static void
out_fp(void * f, const char * buf, int nbytes)
{
    USEARG(nbytes);
    fputs(buf, (FILE*)f);
}



/* dump hex bytes to file 'fp' */
void
hex_dump_bytes_file(const void* inbuf, int buflen, FILE* fp, const char* msg, int offsetonly)
{
    hex_dump_bytes(inbuf, buflen, out_fp, fp, msg, offsetonly);
}


/* Output function to append to buffer. */
static void
__output_buf(void* cookie, const char* str, int string_len)
{
    struct outbuf* b = (struct outbuf*)cookie;
    char* end = b->buf + string_len;

    if (end <=  b->end)
    {
        memcpy(b->buf, str, string_len);
        b->buf  = end;
        *b->buf = 0;
    }
}


/* Dump to buffer 'buf' */
void
hex_dump_buf(char *buf, int bufsize, const void* inbuf, int inlen, const char* msg, int offsetonly)
{
    struct outbuf b;

    b.buf = buf;
    b.end = buf + bufsize;
    hex_dump_bytes(inbuf, inlen, __output_buf, &b, msg, offsetonly);
}



#ifdef  TEST
#include <stdio.h>
#include <stdlib.h>

#define BUFSIZE 256


unsigned long Aligned_buf[BUFSIZE/sizeof(unsigned long)];

static void *
__align (void * buf)
{
    unsigned long p = (unsigned long) buf;

    return (void *) ((p + 15) & ~15);
}


int
main (int argc, char * argv [])
{
    char outbuf[2048];
    unsigned char * buf =  (unsigned char *)&Aligned_buf[0],
                  * aligned = (unsigned char *) __align (buf);
    int i,
        n,
        excess  = aligned - buf,
        bufsize = BUFSIZE - excess;


    /* Create pattern buffer. */
    for (i = 0, n = 255; n >= 0; ++i, --n)
    {
        buf[i] = n;
    }

    excess = aligned - buf;
    printf("--ALIGNED BUF TEST--\n");

    hex_dump_buf(outbuf, sizeof outbuf, aligned+13, bufsize-13, 0, 1);
    printf ("\nptr=%p len=%d off=%d\n", aligned+13, bufsize-13, 13);
    puts(outbuf);
    return 0;
    for (i = 0; i < 16; ++i)
    {
        hex_dump_buf(outbuf, sizeof outbuf, aligned+i, bufsize-i, 0, 0);
        printf ("\nptr=%p len=%d off=%d\n", aligned+i, bufsize-i, i);
        puts(outbuf);
    }


    printf("--SMALL BUF TESTS--\n");
    for (i = 0; i <= 16; ++i)
    {
        int j;

        printf("\nptr=%p len=%d off=%d\n", aligned, i, 0);
        hex_dump_bytes_file(aligned, i, stdout, 0, 1);

        for (j = 1; j < 16; ++j)
        {
            printf("\nptr=%p len=%d off=%d\n", aligned+j, i, j);
            hex_dump_bytes_file(aligned+j, i, stdout, 0, 1);
        }
    }

    return 0;
}

#endif /* TEST */

/* EOF */
