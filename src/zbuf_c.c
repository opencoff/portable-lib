/* :vi:ts=4:sw=4:
 * 
 * zbuf_c.c - zlib buffer interface to compression.
 *
 * Copyright (c) 2002,2003 Sudhi Herle <sw at herle.net>
 *
 * Creation date: Sun Nov 10 12:59:09 2002
 * 
 * Redistribution permitted under the same terms as the original
 * zlib library.
 */

#include "zlib/zbuf.h"
#include <assert.h>
#include <string.h>


/*
 * Initialize compression at level 'lev' to use 'wbits' of
 * compression window size.
 *
 * Returns: Z_OK on success
 *          one of the Z_xxx_ERROR values on error.
 */
int
z_buf_compress_init (z_buf_context * zc, int lev, int wbits)
{
    z_stream * zs = &zc->z;
    int err;

    assert (zc);
    assert (zc->outbuf);
    assert (zc->process_output);

    if ( lev <= 0 || lev > 9 )
        lev = 5;

    if ( wbits <= 4 || wbits > 15 )
        wbits = 15;

    err = deflateInit2 (zs, lev, Z_DEFLATED, wbits, MAX_MEM_LEVEL,
            Z_DEFAULT_STRATEGY);


    zs->next_out  = zc->outbuf;
    zs->avail_out = zc->outbuf_size;

    return err;
}



/*
 * Compress  'len' bytes of data in 'buf'.
 * Calls the 'zc->process_output' member function to drain the
 * compression buffer.
 *
 * Returns:
 *      Z_OK on success
 *      Z_xxx_ERROR on error.
 */
int
z_buf_compress (z_buf_context * zc, void * data, int len)
{
    int err;
    z_stream * zs = &zc->z;

    if ( len > 0 )
    {
        zs->next_in   = (Bytef *) data;
        zs->avail_in  = len;
    }

    /*
     * Main loop to compress and output data.
     */
    while (zs->avail_in != 0)
    {
        if ( zs->avail_out == 0 )
        {
            int used    = (*zc->process_output) (zc->opaq, zc->outbuf, zc->outbuf_size),
                partial = zc->outbuf_size - used;

            /*
             * If the callback-func is unable to consume _any_
             * processed data, we can't proceed any further!
             */
            if ( used == 0 )
                return Z_MEM_ERROR;

            zs->next_out  = zc->outbuf;
            zs->avail_out = zc->outbuf_size;

            if ( partial > 0 )
            {
                /*
                 * Move the leftover fragment to the start of the
                 * output buffer. This will give the callback-func
                 * the impression of a contiguous output buffer from
                 * call-to-call.
                 */
                memmove (zc->outbuf, zc->outbuf+used, partial);
                zs->next_out  += partial;
                zs->avail_out -= partial;
            }
        }
        err = deflate (zs, Z_NO_FLUSH);
        if ( err != Z_OK )
            return err;
    }

    return Z_OK;
}



/*
 * Finalize compression by draining all pending output.
 * Returns:
 *      Z_OK on success
 *      Z_xxx_ERROR on error.
 */
int
z_buf_compress_end (z_buf_context * zc)
{
    int err = Z_OK,
        done = 0;
    z_stream * zs = &zc->z;

    /*
     * For sanity, call z_buf_compress_buf() and exhaust any pending
     * input.
     */
    if ( zs->avail_in > 0 )
    {
        err = z_buf_compress (zc, 0, 0);
        if ( err != Z_OK )
            return err;
    }


    /*
     * To close out the compression session, we have to call
     * deflate() with Z_FINISH. This will result in
     * completion of any pending operations and may result
     * in one or more I/O ops.
     */
    do
    {
        const uLong len = zc->outbuf_size - zs->avail_out;

        /*
         * If there is pending data to write to disk, do so.
         */
        if ( len > 0 )
        {
            /*
             * Flush remaining output.
             */
            unsigned char * buf = zc->outbuf;
            uLong rem  = len;

            while ( rem >  0 )
            {
                int used  = (*zc->process_output) (zc->opaq, buf, rem);

                if ( used <= 0 )
                    return Z_MEM_ERROR;

                buf  += used;
                rem  -= used;
            }

            zs->next_out  = zc->outbuf;
            zs->avail_out = zc->outbuf_size;
        }

        if ( done )
            break;

        err = deflate (zs, Z_FINISH);

        /*
         * Ignore second of two consecutive flushes.
         */
        if ( len == 0 && err == Z_BUF_ERROR )
            err = Z_OK;

        /*
         * deflate() has finished only when it hasn't used up any
         * available space in the output buf.
         */
        done = zs->avail_out != 0 || err == Z_STREAM_END;

        if ( err != Z_OK && err != Z_STREAM_END )
            return err;

    } while (1);


    deflateEnd (zs);

    return 0;
}

/* EOF */
