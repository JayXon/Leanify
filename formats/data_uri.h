#ifndef FORMATS_DATA_URI_H_
#define FORMATS_DATA_URI_H_

#include "format.h"

extern bool is_verbose;

class DataURI : public Format {
 public:
  using Format::Format;

  size_t Leanify(size_t size_leanified = 0) override;
};

#endif  // FORMATS_DATA_URI_H_
