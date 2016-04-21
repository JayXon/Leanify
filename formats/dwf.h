#ifndef DWF_H
#define DWF_H

#include "format.h"

class Dwf : public Format
{
public:
    using Format::Format;

    size_t Leanify(size_t size_leanified = 0) override;

    static const uint8_t header_magic[9];
};

#endif