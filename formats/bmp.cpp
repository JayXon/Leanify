#include "bmp.h"


const unsigned char Bmp::header_magic[] = { 0x42, 0x4D };

size_t Bmp::Leanify(size_t size_leanified /*= 0*/)
{
    // It seems higher version of BITMAPINFOHEADER is very rare.
    // Eg: BITMAPV4HEADER, BITMAPV5HEADER
    // So I'm not going to implement it for now.

    if (size_leanified)
    {
        memmove(fp - size_leanified, fp, *(int *)(fp + 2));
        fp -= size_leanified;
    }

    return size;
}