#ifndef FORMATS_DATA_URI_H_
#define FORMATS_DATA_URI_H_

#include "format.h"

extern bool is_verbose;

class DataURI : public Format {
 public:
  using Format::Format;

  size_t Leanify(size_t size_leanified = 0) override;

  void SetSingleMode(bool single_mode) {
    single_mode_ = single_mode;
  }

 private:
  // Single mode means DataURI is the whole file.
  bool single_mode_ = false;
};

#endif  // FORMATS_DATA_URI_H_
