#ifndef FORMATS_RDB_H_
#define FORMATS_RDB_H_

#include "format.h"

class Rdb : public Format {
 public:
  using Format::Format;

  size_t Leanify(size_t size_leanified = 0) override;

  static const uint8_t header_magic[16];
};

#endif  // FORMATS_RDB_H_
