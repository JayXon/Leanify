#ifndef JPEG_H
#define JPEG_H

#include <csetjmp>  // for mozjpeg error handling
#include <cstdio>

#ifdef _WIN32
#define XMD_H // prevents mozjpeg to redefine INT32
#endif
#include "../lib/mozjpeg/jpeglib.h"

#include "format.h"


extern bool is_fast;
extern bool is_verbose;

class Jpeg : public Format
{
public:
    using Format::Format;

    size_t Leanify(size_t size_leanified = 0) override;

    static const uint8_t header_magic[3];
    static bool keep_exif_;

private:
    static jmp_buf setjmp_buffer_;

    static void mozjpeg_error_handler(j_common_ptr cinfo);
};


#endif