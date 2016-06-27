#ifndef FORMATS_GZ_H_
#define FORMATS_GZ_H_

#include "format.h"

extern bool is_fast;
extern int iterations;

class Gz : public Format {
 public:
  using Format::Format;

  size_t Leanify(size_t size_leanified = 0) override;

  static const uint8_t header_magic[3];
};

#endif  // FORMATS_GZ_H_
