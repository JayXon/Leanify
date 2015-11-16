#ifndef GFT_H
#define GFT_H


#include "format.h"



class Gft : public Format
{
public:
    using Format::Format;

    size_t Leanify(size_t size_leanified = 0) override;

    static const uint8_t header_magic[8];
};



#endif