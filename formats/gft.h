#ifndef FORMATS_GFT_H_
#define FORMATS_GFT_H_

#include "format.h"

class Gft : public Format {
 public:
  using Format::Format;

  size_t Leanify(size_t size_leanified = 0) override;

  static const uint8_t header_magic[8];
};

#endif  // FORMATS_GFT_H_
