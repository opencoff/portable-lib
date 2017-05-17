/* :vi:ts=4:sw=4:
 * 
 * zbufz.h - zlib buffer interface.
 *
 * Copyright (c) 2002-2003 Sudhi Herle <sw at herle.net>
 *
 * Creation date: Sun Nov 10 12:59:09 2002
 * 
 * Redistribution permitted under the same terms as the original
 * zlib library.
 */

#ifndef __ZBUFZ_H__
#define __ZBUFZ_H__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "zlib.h"


/*
 * Using the zlib buffer interface:
 *
 * 1. initialize an instance of z_buf_context by calling:
 *
 *     z_buf_context_init (&zc, output_buffer, outbuf_size,
 *                          output_processor, cookie);
 *
 *      You have to supply an output buffer of fixed size to hold
 *      the processed output (compressed data -- if doing compress,
 *      un-compressed -- otherwise).
 *      The 'output_processor' is a function pointer you supply.
 *      This will be called to drain the output (compressed data if
 *      doing compression, uncompressed data otherwise). You can
 *      associate an opaque "cookie" with the output processor. This
 *      will be passed as the first parameter to the callback
 *      function.
 *
 * 2. Compressing:
 *      Call: z_buf_compress (&zc, buf, len)
 *      to compress 'len' bytes of data in 'buf'. This call can
 *      result in zero or more calls to the output-processor callback
 *      function.
 *
 *    Uncompressing:
 *      Call: z_buf_uncompress (&zc, buf, len)
 *      to uncompress 'len' bytes of compressed data in 'buf'.  This
 *      call can result in zero or more calls to the
 *      output-processor callback function.
 *
 *
 * 3. Finishing up:
 *     If compressing, call z_buf_compress_end(&zc).
 *     If un-compressing, call z_buf_uncompress_end(&zc).
 *     In both cases, the output-processor callback function will be
 *     called zero or more times -- until all the data has been
 *     "drained".
 *
 *
 * The output processor must return the number of bytes that it
 * processed successfully. e.g., with record oriented protocols, the
 * underlying data may not have an integral number of un-compressed
 * records; in such cases, the callback function should return the
 * actual number of bytes processed.
 *
 * See the file zbuf_eg.c to see an example usage.
 */



/*
 * Context for compress or uncompress. Don't manipulate this struct
 * directly. Use the functions/macros supplied below.
 */
typedef struct z_buf_context z_buf_context;
struct z_buf_context
{
    /*
     * Output buffer to hold either compressed or un-compressed data
     * (depending on the usage).
     */
    Byte * outbuf;

    /* Size of the output buffer. */
    uInt outbuf_size;


    /*
     * Function to flush the output.
     *
     * Return number of  bytes actually processed.
     */
    int  (*process_output) (void * opaq, void * buf, int len);
    void * opaq;


    /* zlib stream context. */
    z_stream z;

};





/*
 * Initialize the z-buf-context structure with an output buffer and
 * a output-processor callback function.
 */
#define z_buf_context_init(zc,buf,sz,proc,op)  do { \
                memset ((zc), 0, sizeof(*(zc))); \
                (zc)->outbuf         = (buf);\
                (zc)->outbuf_size    = (sz);\
                (zc)->process_output = (int (*) (void *,void *,int))(proc);\
                (zc)->opaq           = (op); \
            } while (0)



/*
 * Initialize compression at level 'lev' to use 'wbits' of
 * compression window size.
 *
 * Returns: Z_OK on success
 *          one of the Z_xxx_ERROR values on error.
 */
int z_buf_compress_init (z_buf_context * zc, int lev, int wbits);


/*
 * Compress  'len' bytes of data in 'buf'.
 * Calls the 'zc->process_output' member function to drain the
 * compression buffer.
 *
 * Returns:
 *      Z_OK on success
 *      Z_xxx_ERROR on error.
 */
int z_buf_compress (z_buf_context * zc, void * buf, int len);


/*
 * Finalize compression by draining all pending output.
 * Returns:
 *      Z_OK on success
 *      Z_xxx_ERROR on error.
 */
int z_buf_compress_end (z_buf_context * zc);


/*
 * Initialize un-compression using a window size of 'wbits'.
 * Returns:
 *      Z_OK on success
 *      Z_xxx_ERROR on error.
 */
int z_buf_uncompress_init (z_buf_context * zc, int wbits);

/*
 * Uncompress  'len' bytes of data in 'buf'. Will call the output
 * processor zero or more times to drain the output.
 * Returns:
 *      Z_OK on success
 *      Z_STREAM_END on EOF
 *      Z_xxx_ERROR on error.
 */
int z_buf_uncompress (z_buf_context * zc, void * buf, int len);


/*
 * Finalize uncompression by draining all pending output.
 * Returns:
 *      Z_OK on success
 *      Z_xxx_ERROR on error.
 */
int z_buf_uncompress_end (z_buf_context * zc);



/*
 * Handy macros to query statistics.
 */


/*
 * Return the total number of bytes in.
 */
#define z_buf_context_total_in(zc)  ((zc)->z.total_in)

/*
 * Reutn the total number of bytes out.
 */
#define z_buf_context_total_out(zc) ((zc)->z.total_out)

/*
 * Return the Adler-32 checksum of the output.
 */
#define z_buf_context_adler32(zc)   ((zc)->z.adler)




/*
 * Define custom memory allocator to be associated with z_buf_context
 * 'zc'.  'a' is the alloc function, 'f' is the free function, and
 * 'o' is the opaque value to be passed to both of these.
 */
#define z_buf_context_set_allocator(zc,a,f,o) do { \
            z_stream *zs_ = &(zc)->z;\
            zs_->zalloc = a;\
            zs_->zfree  = f;\
            zs_->opaque = o;\
        } while (0)



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __ZBUFZ_H__ */

/* EOF */
