#ifndef FORMATS_BASE64_H_
#define FORMATS_BASE64_H_

#include "format.h"

class Base64 : public Format {
 public:
  using Format::Format;

  size_t Leanify(size_t size_leanified = 0) override;
};

#endif  // FORMATS_BASE64_H_
