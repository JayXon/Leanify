#ifndef TAR_H
#define TAR_H


#include <cstdlib>

#include "format.h"


extern bool is_verbose;

class Tar : public Format
{
public:
    Tar(void *p, size_t s = 0) : Format(p, s)
    {
        // check file size first
        is_valid = s > 512 && s % 512 == 0 &&
                   CalcChecksum((unsigned char *)p) == strtol(fp + 148, nullptr, 8);
    }

    size_t Leanify(size_t size_leanified = 0);

    bool IsValid() const
    {
        return is_valid;
    }

private:
    int CalcChecksum(unsigned char *header) const;

    bool is_valid;

};


#endif