#include "jpeg.h"


const unsigned char Jpeg::header_magic[] = { 0xFF, 0xD8 };

size_t Jpeg::Leanify(size_t size_leanified /*= 0*/)
{
    if (size_leanified)
    {
        memmove(fp - size_leanified, fp, size);
        fp -= size_leanified;
    }

    // TODO: use mozjpeg to leanify

    return 0;
}