#ifndef ZIP_H
#define ZIP_H



#include <iostream>
#include <cstring>
#include <cstdint>
#include <vector>
#include <algorithm>    // std::search

#include "miniz/miniz.h"
#include "zopfli/deflate.h"

#include "format.h"
#include "../leanify.h"


extern bool is_fast;
extern int iterations;
extern int level;

class Zip : Format
{
public:
    Zip(void *p, size_t s = 0) : Format(p, s)
    {
        ZopfliInitOptions(&zopfli_options);
        zopfli_options.numiterations = iterations;
    }
    ~Zip() { level--; }

    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[4];

private:
    ZopfliOptions zopfli_options;
};


#endif