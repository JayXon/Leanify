#ifndef SWF_H
#define SWF_H



#include <iostream>
#include <cstring>
#include <cstdint>

#include "miniz/miniz.h"
#include "LZMA/LzmaLib.h"
#include "zopfli/zlib_container.h"

#include "format.h"
#include "../leanify.h"

extern bool is_verbose;
extern bool is_recompress;
extern int iterations;

class Swf : Format
{
public:
    Swf(void *p, size_t s = 0) : Format(p, s)
    {
        ZopfliInitOptions(&zopfli_options);
        zopfli_options.numiterations = iterations;
    }

    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[3];
    static const unsigned char header_magic_deflate[3];
    static const unsigned char header_magic_lzma[3];
private:
    ZopfliOptions zopfli_options;

    void Move(size_t size_leanified);
    size_t ZlibRecompress(unsigned char *src, size_t src_len, size_t size_leanified);
    void UpdateTagLength(unsigned char *tag_content, size_t header_length, size_t new_length);
};


#endif