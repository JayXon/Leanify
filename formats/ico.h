#ifndef ICO_H
#define ICO_H

#include "format.h"

class Ico : public Format
{
public:
    using Format::Format;

    size_t Leanify(size_t size_leanified = 0);

    static const uint8_t header_magic[4];
};


#endif