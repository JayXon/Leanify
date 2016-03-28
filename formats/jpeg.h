#ifndef JPEG_H
#define JPEG_H

#include "format.h"


extern bool is_fast;
extern bool is_verbose;

class Jpeg : public Format
{
public:
    using Format::Format;

    size_t Leanify(size_t size_leanified = 0) override;

    static const uint8_t header_magic[3];
    static bool keep_exif_;

};


#endif