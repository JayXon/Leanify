#ifndef ZIP_H
#define ZIP_H


#include "zopfli/deflate.h"

#include "format.h"


extern bool is_fast;
extern int iterations;
extern int depth;


class Zip : Format
{
public:
    Zip(void *p, size_t s = 0) : Format(p, s)
    {
        ZopfliInitOptions(&zopfli_options);
        zopfli_options.numiterations = iterations;
    }
    ~Zip() { depth--; }

    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[4];

private:
    ZopfliOptions zopfli_options;
};


#endif