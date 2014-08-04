#ifndef ICO_H
#define ICO_H

#include <cstdint>

#include "format.h"
#include "png.h"


class Ico : Format
{
public:
    Ico(void *p, size_t s = 0) : Format(p, s) {}

    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[4];
};


#endif