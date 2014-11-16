#ifndef PNG_H
#define PNG_H



#include <cstdint>
#include <cstring>
#include <iostream>

#include "zopflipng/zopflipng_lib.h"

#include "format.h"



#ifdef _MSC_VER
#   define bswap32(x) _byteswap_ulong(x)
#elif defined __GNUC__
#   define bswap32(x) __builtin_bswap32(x)
#else
#   define bswap32(x) _bswap(x)
#endif


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