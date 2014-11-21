#ifndef PNG_H
#define PNG_H


#include "format.h"


extern bool is_fast;
extern bool is_verbose;
extern int iterations;


class Png : Format
{

public:

    Png(void *p, size_t s = 0) : Format(p, s) {}
    // using Format::Format;
    // VS2013 does not support C++11 inheriting constructors


    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[8];

};




#endif