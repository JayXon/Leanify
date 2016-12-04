#ifndef FORMATS_PNG_H_
#define FORMATS_PNG_H_

#include "format.h"

extern bool is_fast;
extern bool is_verbose;
extern int iterations;

class Png : public Format {
 public:
  using Format::Format;

  size_t Leanify(size_t size_leanified = 0) override;

  static const uint8_t header_magic[8];
  static bool keep_icc_profile_;
};

#endif  // FORMATS_PNG_H_
