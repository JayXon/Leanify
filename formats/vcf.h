#ifndef FORMATS_VCF_H_
#define FORMATS_VCF_H_

#include "format.h"

extern bool is_verbose;

class Vcf : public Format {
 public:
  using Format::Format;

  size_t Leanify(size_t size_leanified = 0) override;
};

#endif  // FORMATS_VCF_H_
