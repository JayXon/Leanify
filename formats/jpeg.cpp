#include "jpeg.h"


const unsigned char Jpeg::header_magic[] = { 0xFF, 0xD8 };

size_t Jpeg::Leanify(size_t size_leanified /*= 0*/)
{

    struct jpeg_decompress_struct srcinfo;
    struct jpeg_compress_struct dstinfo;
    struct jpeg_error_mgr jsrcerr, jdsterr;

    /* Initialize the JPEG decompression object with default error handling. */
    srcinfo.err = jpeg_std_error(&jsrcerr);
    jpeg_create_decompress(&srcinfo);
    /* Initialize the JPEG compression object with default error handling. */
    dstinfo.err = jpeg_std_error(&jdsterr);
    jpeg_create_compress(&dstinfo);

    dstinfo.use_moz_defaults = is_recompress;

    if (is_verbose)
    {
        dstinfo.err->trace_level++;
    }

    /* Specify data source for decompression */
    jpeg_mem_src(&srcinfo, (unsigned char *)fp, size);

    /* Read file header */
    (void)jpeg_read_header(&srcinfo, TRUE);

    /* Read source file as DCT coefficients */
    jvirt_barray_ptr *coef_arrays = jpeg_read_coefficients(&srcinfo);

    /* Initialize destination compression parameters from source values */
    jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

    dstinfo.optimize_coding = TRUE;

    unsigned char *outbuffer = NULL;
    unsigned long outsize = 0;
    /* Specify data destination for compression */
    jpeg_mem_dest(&dstinfo, &outbuffer, &outsize);

    /* Start compressor (note no image data is actually written here) */
    jpeg_write_coefficients(&dstinfo, coef_arrays);

    /* Finish compression and release memory */
    jpeg_finish_compress(&dstinfo);


    fp -= size_leanified;
    // use mozjpeg result if it's smaller than original
    if (outsize < size)
    {
        memcpy(fp, outbuffer, outsize);
        size = outsize;
    }
    else if (size_leanified)
    {
        memmove(fp, fp + size_leanified, size);
    }

    jpeg_destroy_compress(&dstinfo);
    (void)jpeg_finish_decompress(&srcinfo);
    jpeg_destroy_decompress(&srcinfo);

    return size;
}