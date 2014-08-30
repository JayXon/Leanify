#ifndef SWF_H
#define SWF_H



#include <iostream>
#include <cstring>
#include <cstdint>

#include "miniz/miniz.h"
#include "LZMA/LzmaLib.h"

#include "format.h"
#include "../leanify.h"

extern bool is_verbose;
extern bool is_fast;
extern int iterations;

class Swf : Format
{
public:
    Swf(void *p, size_t s = 0) : Format(p, s) {}

    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[3];
    static const unsigned char header_magic_deflate[3];
    static const unsigned char header_magic_lzma[3];

private:

    void UpdateTagLength(unsigned char *tag_content, size_t header_length, size_t new_length);
};


#endif