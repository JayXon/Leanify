#include "jpeg.h"

#include <algorithm>
#include <csetjmp>  // for mozjpeg error handling
#include <iostream>

#include <mozjpeg/jpeglib.h>

const uint8_t Jpeg::header_magic[] = { 0xFF, 0xD8, 0xFF };
bool Jpeg::keep_exif_ = false;
bool Jpeg::keep_icc_profile_ = false;
bool Jpeg::keep_all_metadata_ = false;
bool Jpeg::force_arithmetic_coding_ = false;

namespace {

jmp_buf setjmp_buffer;

void mozjpeg_error_handler(j_common_ptr cinfo) {
  (*cinfo->err->output_message)(cinfo);

  if (cinfo->client_data != nullptr)
    jpeg_destroy(static_cast<j_common_ptr>(cinfo->client_data));

  longjmp(setjmp_buffer, 1);
}

void CompressJpeg(const j_decompress_ptr srcinfo, jvirt_barray_ptr* coef_arrays, bool baseline, bool arithmetic,
                  bool keep_exif, uint8_t** outbuffer, unsigned long* outsize) {
  jpeg_compress_struct dstinfo;
  dstinfo.client_data = &dstinfo;

  jpeg_error_mgr jdsterr;
  dstinfo.err = jpeg_std_error(&jdsterr);
  jdsterr.error_exit = mozjpeg_error_handler;

  jpeg_create_compress(&dstinfo);

  if (is_verbose) {
    dstinfo.err->trace_level++;
  }
  if (baseline) {
    jpeg_c_set_int_param(&dstinfo, JINT_COMPRESS_PROFILE, JCP_FASTEST);
  }

  /* Initialize destination compression parameters from source values */
  jpeg_copy_critical_parameters(srcinfo, &dstinfo);

  // use arithmetic coding if input file is arithmetic coded or if forced to
  if (arithmetic) {
    dstinfo.arith_code = true;
    dstinfo.optimize_coding = false;
  } else {
    dstinfo.optimize_coding = true;
  }

  /* Specify data destination for compression */
  jpeg_mem_dest(&dstinfo, outbuffer, outsize);

  /* Start compressor (note no image data is actually written here) */
  jpeg_write_coefficients(&dstinfo, coef_arrays);

  for (auto marker = srcinfo->marker_list; marker; marker = marker->next) {
    if (keep_exif || marker->marker != JPEG_APP0 + 1) {
      jpeg_write_marker(&dstinfo, marker->marker, marker->data, marker->data_length);
    }
  }

  /* Finish compression and release memory */
  jpeg_finish_compress(&dstinfo);
  jpeg_destroy_compress(&dstinfo);
}

}  // namespace

size_t Jpeg::Leanify(size_t size_leanified /*= 0*/) {
  jpeg_decompress_struct srcinfo;
  srcinfo.client_data = nullptr;

  jpeg_error_mgr jsrcerr;
  srcinfo.err = jpeg_std_error(&jsrcerr);
  jsrcerr.error_exit = mozjpeg_error_handler;

  if (setjmp(setjmp_buffer)) {
    jpeg_destroy_decompress(&srcinfo);

    return Format::Leanify(size_leanified);
  }

  jpeg_create_decompress(&srcinfo);

  /* Specify data source for decompression */
  jpeg_mem_src(&srcinfo, fp_, size_);

  // Always save exif to show warning if orientation might change.
  jpeg_save_markers(&srcinfo, JPEG_APP0 + 1, 0xFFFF);
  if (keep_icc_profile_ || keep_all_metadata_) {
    jpeg_save_markers(&srcinfo, JPEG_APP0 + 2, 0xFFFF);
  }
  if (keep_all_metadata_) {
    // Save the rest APPn markers.
    for (int i = 3; i < 16; i++)
      jpeg_save_markers(&srcinfo, JPEG_APP0 + i, 0xFFFF);
    // Save comments.
    jpeg_save_markers(&srcinfo, JPEG_COM, 0xFFFF);
  }

  jpeg_read_header(&srcinfo, true);

  if (!keep_exif_ && !keep_all_metadata_) {
    for (auto marker = srcinfo.marker_list; marker; marker = marker->next) {
      if (marker->marker == JPEG_APP0 + 1) {
        // Tag number: 0x0112, data format: unsigned short(3), number of components: 1
        const uint8_t kExifOrientation[] = { 0x12, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00 };
        const uint8_t kExifOrientationMotorola[] = { 0x01, 0x12, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01 };
        uint8_t* start = marker->data;
        uint8_t* end = start + marker->data_length;
        uint8_t* orientation_tag = std::search(start, end, kExifOrientation, std::end(kExifOrientation));
        bool big_endian = false;
        if (orientation_tag == end) {
          orientation_tag = std::search(start, end, kExifOrientationMotorola, std::end(kExifOrientationMotorola));
          big_endian = orientation_tag != end;
        }
        if (orientation_tag != end) {
          uint16_t orientation = *reinterpret_cast<uint16_t*>(orientation_tag + sizeof(kExifOrientation));
          if (big_endian)
            orientation = ((orientation >> 8) | (orientation << 8)) & 0xFFFF;
          // Only show warning if it's not the default upper left.
          if (orientation != 1) {
            std::cout << "Warning: The Exif being removed contains orientation data, result image might have wrong "
                         "orientation, use --keep-exif to keep Exif."
                      << std::endl;
          }
        }
        break;
      }
    }
  }

  /* Read source file as DCT coefficients */
  auto coef_arrays = jpeg_read_coefficients(&srcinfo);

  uint8_t* outbuffer = nullptr;
  unsigned long outsize = 0;

  // Try progressive unless fast mode.
  if (!is_fast) {
    CompressJpeg(&srcinfo, coef_arrays, false, srcinfo.arith_code || force_arithmetic_coding_,
                 keep_all_metadata_ || keep_exif_, &outbuffer, &outsize);
  }

  // Try baseline if fast mode or small file.
  if (is_fast || size_ < 32768) {
    uint8_t* baseline_buffer = nullptr;
    unsigned long baseline_size = 0;

    CompressJpeg(&srcinfo, coef_arrays, true, srcinfo.arith_code || force_arithmetic_coding_,
                 keep_all_metadata_ || keep_exif_, &baseline_buffer, &baseline_size);
    if (baseline_size < outsize) {
      free(outbuffer);
      outbuffer = baseline_buffer;
      outsize = baseline_size;
    } else {
      free(baseline_buffer);
    }
  }

  (void)jpeg_finish_decompress(&srcinfo);
  jpeg_destroy_decompress(&srcinfo);

  fp_ -= size_leanified;
  // use mozjpeg result if it's smaller than original
  if (outsize < size_) {
    memcpy(fp_, outbuffer, outsize);
    size_ = outsize;
  } else {
    memmove(fp_, fp_ + size_leanified, size_);
  }
  free(outbuffer);

  return size_;
}
