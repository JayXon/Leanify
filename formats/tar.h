#ifndef FORMATS_TAR_H_
#define FORMATS_TAR_H_

#include <cstdlib>

#include "format.h"

extern bool is_verbose;

class Tar : public Format
{
public:
    explicit Tar(void *p, size_t s = 0) : Format(p, s)
    {
        // check file size first
        is_valid_ = s > 512 && s % 512 == 0 &&
                   CalcChecksum(fp_) == strtol(static_cast<char *>(p) + 148, nullptr, 8);
    }

    size_t Leanify(size_t size_leanified = 0) override;

    bool IsValid() const
    {
        return is_valid_;
    }

private:
    int CalcChecksum(uint8_t *header) const;

    bool is_valid_;
};

#endif  // FORMATS_TAR_H_
