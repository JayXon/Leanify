#ifndef JPEG_H
#define JPEG_H

#include <csetjmp>  // for mozjpeg error handling
#include <cstdio>

#ifdef _WIN32
#define XMD_H // prevents mozjpeg to redefine INT32
#endif
#include "mozjpeg/jpeglib.h"

#include "format.h"


extern bool is_fast;
extern bool is_verbose;

class Jpeg : Format
{
public:
    Jpeg(void *p, size_t s = 0) : Format(p, s) {}

    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[3];
    static bool keep_exif;

private:
    static jmp_buf setjmp_buffer;

    static void mozjpeg_error_handler(j_common_ptr cinfo);
};


#endif