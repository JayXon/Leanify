#include "gft.h"

#include <cstdint>
#include <iostream>

#include "../leanify.h"


const unsigned char Gft::header_magic[] = { 0x54, 0x47, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00 };


size_t Gft::Leanify(size_t size_leanified /*= 0*/)
{
    // header
    uint32_t header_size;
    if (size < 0x14 || size <= (header_size = *(uint32_t *)(fp + 0x10)))
    {
        std::cerr << "Not a valid GFT file." << std::endl;
        return Format::Leanify(size_leanified);
    }


    // move header
    if (size_leanified)
    {
        memmove(fp - size_leanified, fp, header_size);
        fp -= size_leanified;
    }


    return header_size + LeanifyFile(fp + size_leanified + header_size, size - header_size, size_leanified);
}