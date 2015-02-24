#ifndef GFT_H
#define GFT_H


#include "format.h"



class Gft : public Format
{
public:
    Gft(void *p, size_t s = 0) : Format(p, s) {}
    // using Format::Format;
    // VS2013 does not support C++11 inheriting constructors

    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[8];
};



#endif