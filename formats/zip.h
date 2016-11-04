#ifndef FORMATS_ZIP_H_
#define FORMATS_ZIP_H_

#include <zopfli/deflate.h>

#include "format.h"

extern bool is_fast;
extern int iterations;
extern int depth;

class Zip : public Format {
 public:
  explicit Zip(void* p, size_t s = 0, uint32_t o = 0) : Format(p, s), out_embed_off_(o) {
    ZopfliInitOptions(&zopfli_options_);
    zopfli_options_.numiterations = iterations;
  }
  ~Zip() {
    depth--;
  }

  size_t Leanify(size_t size_leanified = 0) override;

  static const uint8_t header_magic[4];

 private:
  ZopfliOptions zopfli_options_;
  uint32_t out_embed_off_;
};

#endif  // FORMATS_ZIP_H_
