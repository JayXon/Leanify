#ifndef FORMATS_JPEG_H_
#define FORMATS_JPEG_H_

#include "format.h"

extern bool is_fast;
extern bool is_verbose;

class Jpeg : public Format {
 public:
  using Format::Format;

  size_t Leanify(size_t size_leanified = 0) override;

  static const uint8_t header_magic[3];
  static bool keep_exif_;
  static bool keep_icc_profile_;
  static bool keep_all_metadata_;
  static bool force_arithmetic_coding_;
};

#endif  // FORMATS_JPEG_H_
