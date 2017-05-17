/* :vi:ts=4:sw=4:
 * 
 * zbuf_eg.c - Example program to demonstrate zlib buffer interface.
 *
 * Copyright (c) 2002,2003 Sudhi Herle <sudhi@herle.net>
 *
 * Creation date: Sun Nov 10 12:59:09 2002
 * 
 * Redistribution permitted under the same terms as the original
 * zlib library.
 */

#include "zlib/zbuf.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* 
 * Size of the input and output buffer.
 * We'll make the output buffer smaller than the input buffer to
 * force the draining to happen more frequently.
 */
#define INBUFSZ         1024
#define OUTBUFSZ        (INBUFSZ / 2)

/*
 * Compression window size
 */
#define Z_WINSZ     13


static void do_compress_test (char  * infile, char * outfile);
static void do_uncompress_test (char  * infile, char * outfile);

static void
error (int fatal, int errcode, const char * fmt, ...)
{
    va_list ap;

    if ( errcode < 0 )
        errcode = -errcode;

    va_start(ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end (ap);

    if ( errcode )
        fprintf (stderr, "(%d: %s)", errcode, strerror(errcode));
    fputc ('\n', stderr);

    if ( fatal )
        exit (fatal);
}


/* 
 * Common output function to write the processed data to a file.
 */
static int
process_data (void * opaq, void * buf, int len)
{
    FILE * fp = (FILE *) opaq;
    int n;
    
    /*
     * In this example, we will always consume all the data in
     * 'buf'.
     */

    n = fwrite (buf, 1, len, fp);
    if ( n != len )
        error (1, errno, "Partial write to output file; expected %d, saw %d",
                    len, n);

    return len;
}


int
main (int argc, char * argv [])
{
    char * program_name = argv[0];
    char * infilename = argv[1];
    char zfilename[256];
    char unzfilename[256];

    if ( argc < 2 )
        error (1, 0, "Usage: %s input-file-for-test", program_name);

    snprintf(zfilename, sizeof zfilename, "%s.z", infilename);
    snprintf(unzfilename, sizeof unzfilename, "%s.new", infilename);

    /*
     * As a demonstration, we take the input file and compress it
     * one chunk at a time and write to a new file with extension ".z".
     *
     * Next, we take the ".z" file and uncompress it one chunk at a
     * time and write to a new file with extension ".new".
     */

    do_compress_test   (infilename, zfilename);
    do_uncompress_test (zfilename, unzfilename);
    return 0;
}

static void
do_compress_test (char  * infile, char * outfile)
{
    FILE * infp,
         * outfp;
    unsigned char outbuf[OUTBUFSZ];
    unsigned char inbuf[INBUFSZ];
    int n;

    z_buf_context zc;

    infp = fopen (infile,   "rb");
    if ( !infp )
        error (1, errno, "Can't open input file '%s'", infile);

    outfp = fopen (outfile, "wb");
    if ( !outfp )
        error (1, errno, "Can't create output file '%s'", outfile);

    z_buf_context_init (&zc, outbuf, OUTBUFSZ, process_data, outfp);

    /* 
     * Initialize compression at MAX level (9) and window-size of
     * 13.
     */
    n = z_buf_compress_init (&zc, 9, Z_WINSZ);
    if ( n != Z_OK )
        error (1, 0, "Can't initialize zlib for compression: %s(%d)",
                zError(n), n);

    printf ("Compressing %s -> %s\n", infile, outfile);

    while ( !feof (infp) )
    {
        int nread = fread (inbuf, 1, INBUFSZ, infp);
        if ( ferror (infp) )
            error (1, errno, "Read error on %s", infile);

        n = z_buf_compress (&zc, inbuf, nread);
        if ( n != Z_OK )
            error (1, 0, "Can't compress: %s(%d)",
                    zError(n), n);
    }
    n = z_buf_compress_end (&zc);
    if ( n != Z_OK )
        error (1, 0, "Can't finish compress: %s(%d)",
                zError(n), n);

    fclose (outfp);
    fclose (infp);
    printf ("\t%lu total bytes in, %lu total bytes out\n",
            z_buf_context_total_in(&zc),
            z_buf_context_total_out(&zc));
}


static void
do_uncompress_test (char  * infile, char * outfile)
{
    FILE * infp,
         * outfp;
    unsigned char outbuf[OUTBUFSZ];
    unsigned char inbuf[INBUFSZ];
    int n;

    z_buf_context zc;

    infp = fopen (infile,   "rb");
    if ( !infp )
        error (1, errno, "Can't open input file '%s'", infile);

    outfp = fopen (outfile, "wb");
    if ( !outfp )
        error (1, errno, "Can't create output file '%s'", outfile);

    z_buf_context_init (&zc, outbuf, OUTBUFSZ, process_data, outfp);

    /* 
     * Initialize un-compression at MAX level (9) and window-size of
     * 13.
     */
    n = z_buf_uncompress_init (&zc, Z_WINSZ);
    if ( n != Z_OK )
        error (1, 0, "Can't initialize zlib for uncompression: %s(%d)",
                zError(n), n);

    printf ("Uncompressing %s -> %s\n", infile, outfile);

    while ( !feof (infp) )
    {
        int nread = fread (inbuf, 1, INBUFSZ, infp);
        if ( ferror (infp) )
            error (1, errno, "Read error on %s", infile);

        n = z_buf_uncompress (&zc, inbuf, nread);
        if ( n == Z_STREAM_END )
            break;
        else if ( n != Z_OK )
            error (1, 0, "Can't uncompress: %s(%d)",
                    zError(n), n);
    }
    n = z_buf_uncompress_end (&zc);
    if ( n != Z_OK )
        error (1, 0, "Can't finish uncompress: %s(%d)",
                zError(n), n);

    fclose (outfp);
    fclose (infp);
    printf ("\t%lu total bytes in, %lu total bytes out\n",
            z_buf_context_total_in(&zc),
            z_buf_context_total_out(&zc));
}

/* EOF */
