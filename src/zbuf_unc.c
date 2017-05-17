/* :vi:ts=4:sw=4:
 * 
 * zbuf_unc.c - zlib buffer interface to de-compression.
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


static int do_uncompress (z_stream * zs, z_buf_context * zc);


/*
 * initialize decompression.
 */
int
z_buf_uncompress_init (z_buf_context * zc, int wbits)
{
    z_stream * zs = &zc->z;
    int err;

    assert (zc);
    assert (zc->outbuf);
    assert (zc->process_output);

    if ( wbits <= 0 || wbits > 15 )
        wbits = 15;

    err = inflateInit2 (zs, wbits);


    zs->next_out  = zc->outbuf;
    zs->avail_out = zc->outbuf_size;
    zs->avail_in  = 0;
    zs->next_in   = 0;

    return err;
}



/*
 * Return number of chars consumed.
 */
int
z_buf_uncompress (z_buf_context * zc, void * data, int len)
{
    z_stream * zs = &zc->z;

    assert (zs->avail_in == 0);

    zs->next_in  = (Bytef *)data;
    zs->avail_in = len;

    return do_uncompress (zs, zc);
}



/*
 * Finalize decompression session.
 */
int
z_buf_uncompress_end (z_buf_context * zc)
{
    z_stream * zs = &zc->z;
    int err  = Z_OK;

    /*
     * Uncompress any pending data.
     */
    if ( zs->avail_in > 0 )
    {
        err = do_uncompress (zs, zc);
        if ( err != Z_STREAM_END || err != Z_OK )
            return err;
    }

    if ( zs->avail_out < zc->outbuf_size )
    {
        /*
         * Flush remaining output.
         */
        unsigned char * buf = zc->outbuf;
        int rem  = zc->outbuf_size - zs->avail_out;

        while ( rem >  0 )
        {
            int used  = (*zc->process_output) (zc->opaq, buf, rem);

            if ( used <= 0 )
                return Z_MEM_ERROR;

            buf  += used;
            rem  -= used;
        }
    }

    inflateEnd (zs);
    return err;
}


/*
 * Internal function to do the uncompress.
 */
static int
do_uncompress (z_stream * zs, z_buf_context * zc)
{
    int err = Z_OK;

    while ( zs->avail_in > 0 )
    {
        if ( zs->avail_out == 0 )
        {
            int used = (*zc->process_output) (zc->opaq, zc->outbuf, zc->outbuf_size),
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

        err = inflate (zs, Z_NO_FLUSH);
        if ( err == Z_STREAM_END )
            break;
        
        if ( err != Z_OK )
            return err;
    }

    return err;
}

/* EOF */
