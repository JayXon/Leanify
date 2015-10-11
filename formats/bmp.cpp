#include "bmp.h"

#include <iostream>

const uint8_t Bmp::header_magic[] = { 0x42, 0x4D };

size_t Bmp::Leanify(size_t size_leanified /*= 0*/)
{
    // It seems higher version of BITMAPINFOHEADER is very rare.
    // Eg: BITMAPV4HEADER, BITMAPV5HEADER
    // So I'm not going to implement it for now.
    uint32_t bmp_size = *(uint32_t *)(fp + 2);
    if (bmp_size > size)
    {
        std::cerr << "BMP corrupted." << std::endl;
        return Format::Leanify(size_leanified);
    }
    size = bmp_size;

    if (size_leanified)
    {
        memmove(fp - size_leanified, fp, size);
        fp -= size_leanified;
    }

    return size;
}