#ifndef JPEG_H
#define JPEG_H

#include <cstdio>

#ifdef _WIN32
#define XMD_H // prevents mozjpeg to redefine INT32
#endif
#include "mozjpeg/jpeglib.h"

#include "format.h"

class Jpeg : Format
{
public:
    Jpeg(void *p, size_t s = 0) : Format(p, s) {}

    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[2];

};


#endif