#ifndef JPEG_H
#define JPEG_H


#include "format.h"

// The support for this format is not yet implemented!

class Jpeg : Format
{
public:
    Jpeg(void *p, size_t s = 0) : Format(p, s) {}

    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[2];

};


#endif