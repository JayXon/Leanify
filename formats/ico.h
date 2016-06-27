#ifndef FORMATS_ICO_H_
#define FORMATS_ICO_H_

#include "format.h"

class Ico : public Format {
 public:
  using Format::Format;

  size_t Leanify(size_t size_leanified = 0) override;

  static const uint8_t header_magic[4];
};

#endif  // FORMATS_ICO_H_
