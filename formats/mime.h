#ifndef FORMATS_MIME_H_
#define FORMATS_MIME_H_

#include "format.h"

extern bool is_verbose;

// Multipurpose Internet Mail Extensions
// https://en.wikipedia.org/wiki/MIME

class Mime : public Format {
 public:
  using Format::Format;

  size_t Leanify(size_t size_leanified = 0) override;
};

#endif  // FORMATS_MIME_H_
