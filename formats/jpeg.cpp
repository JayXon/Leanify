#include "jpeg.h"


const uint8_t Jpeg::header_magic[] = { 0xFF, 0xD8, 0xFF };
bool Jpeg::keep_exif = false;

jmp_buf Jpeg::setjmp_buffer;

void Jpeg::mozjpeg_error_handler(j_common_ptr cinfo)
{
    (*cinfo->err->output_message)(cinfo);

    longjmp(setjmp_buffer, 1);
}



size_t Jpeg::Leanify(size_t size_leanified /*= 0*/)
{

    struct jpeg_decompress_struct   srcinfo;
    struct jpeg_compress_struct     dstinfo;
    struct jpeg_error_mgr           jsrcerr, jdsterr;

    srcinfo.err = jpeg_std_error(&jsrcerr);
    jsrcerr.error_exit = mozjpeg_error_handler;
    if (setjmp(setjmp_buffer))
    {
        jpeg_destroy_compress(&dstinfo);
        jpeg_destroy_decompress(&srcinfo);

        return Format::Leanify(size_leanified);
    }

    jpeg_create_decompress(&srcinfo);

    dstinfo.err = jpeg_std_error(&jdsterr);
    jdsterr.error_exit = mozjpeg_error_handler;

    jpeg_create_compress(&dstinfo);

    if (is_verbose)
    {
        dstinfo.err->trace_level++;
    }
    if (is_fast)
    {
        jpeg_c_set_int_param(&dstinfo, JINT_COMPRESS_PROFILE, JCP_FASTEST);
    }

    /* Specify data source for decompression */
    jpeg_mem_src(&srcinfo, fp, size);

    if (keep_exif)
    {
        jpeg_save_markers(&srcinfo, JPEG_APP0 + 1, 0xFFFF);
    }

    (void)jpeg_read_header(&srcinfo, true);

    /* Read source file as DCT coefficients */
    auto coef_arrays = jpeg_read_coefficients(&srcinfo);

    /* Initialize destination compression parameters from source values */
    jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

    // use arithmetic coding if input file is arithmetic coded
    if (srcinfo.arith_code)
    {
        dstinfo.arith_code = true;
        dstinfo.optimize_coding = false;
    }
    else
    {
        dstinfo.optimize_coding = true;
    }

    uint8_t *outbuffer = nullptr;
    unsigned long outsize = 0;
    /* Specify data destination for compression */
    jpeg_mem_dest(&dstinfo, &outbuffer, &outsize);

    /* Start compressor (note no image data is actually written here) */
    jpeg_write_coefficients(&dstinfo, coef_arrays);

    if (keep_exif)
    {
        for (auto marker = srcinfo.marker_list; marker; marker = marker->next)
        {
            if (marker->marker == JPEG_APP0 + 1)
            {
                jpeg_write_marker(&dstinfo, marker->marker, marker->data, marker->data_length);
            }
        }
    }

    /* Finish compression and release memory */
    jpeg_finish_compress(&dstinfo);

    (void)jpeg_finish_decompress(&srcinfo);
    jpeg_destroy_decompress(&srcinfo);

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

    return size;
}