#ifndef FORMATS_SWF_H_
#define FORMATS_SWF_H_

#include "format.h"

extern bool is_fast;
extern int iterations;

class Swf : public Format {
 public:
  using Format::Format;

  size_t Leanify(size_t size_leanified = 0) override;

  static const uint8_t header_magic[3];
  static const uint8_t header_magic_deflate[3];
  static const uint8_t header_magic_lzma[3];
};

#endif  // FORMATS_SWF_H_
