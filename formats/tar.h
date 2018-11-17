#ifndef FORMATS_TAR_H_
#define FORMATS_TAR_H_

#include "format.h"

extern bool is_verbose;

class Tar : public Format {
 public:
  Tar(void* p, size_t s);

  size_t Leanify(size_t size_leanified = 0) override;

  bool IsValid() const {
    return is_valid_;
  }

 private:
  bool is_valid_;
};

#endif  // FORMATS_TAR_H_
