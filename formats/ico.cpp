#include "ico.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include "png.h"

using std::cerr;
using std::endl;

const uint8_t Ico::header_magic[] = { 0x00, 0x00, 0x01, 0x00 };

namespace
{

struct IconDirEntry
{
    uint8_t        bWidth;          // Width, in pixels, of the image
    uint8_t        bHeight;         // Height, in pixels, of the image
    uint8_t        bColorCount;     // Number of colors in image (0 if >=8bpp)
    uint8_t        bReserved;       // Reserved ( must be 0)
    uint16_t       wPlanes;         // Color Planes
    uint16_t       wBitCount;       // Bits per pixel
    uint32_t       dwBytesInRes;    // How many bytes in this resource?
    uint32_t       dwImageOffset;   // Where in the file is this image?
};

} // namespace


size_t Ico::Leanify(size_t size_leanified /*= 0*/)
{
    // number of images inside ico file
    uint16_t num_of_img = *(uint16_t *)(fp_ + 4);

    // size too small
    if (6 + num_of_img * sizeof(IconDirEntry) >= size_)
    {
        return Format::Leanify(size_leanified);
    }

    // sort the entries by offset just in case it's not sorted before
    IconDirEntry *entry_addr = reinterpret_cast<IconDirEntry *>(fp_ + 6);
    std::vector<IconDirEntry> entries(entry_addr, entry_addr + num_of_img);
    std::sort(entries.begin(), entries.end(), [] (const IconDirEntry& a, const IconDirEntry& b)
    {
        return a.dwImageOffset < b.dwImageOffset;
    });

    // check overlaps
    for (size_t i = 1; i < entries.size(); i++)
    {
        if (entries[i - 1].dwImageOffset + entries[i - 1].dwBytesInRes > entries[i].dwImageOffset)
        {
            cerr << "Error: Found overlapping icon entries!" << endl;
            return Format::Leanify(size_leanified);
        }
    }

    // is file size enough?
    if (entries.back().dwImageOffset + entries.back().dwBytesInRes > size_)
    {
        return Format::Leanify(size_leanified);
    }

    for (size_t i = 0; i < entries.size(); i++)
    {
        uint32_t old_offset = entries[i].dwImageOffset;
        // write new offset
        if (i != 0)
        {
            entries[i].dwImageOffset = entries[i - 1].dwImageOffset + entries[i - 1].dwBytesInRes;
        }
        else
        {
            entries[i].dwImageOffset = 6 + num_of_img * sizeof(IconDirEntry);
        }

        // only Leanify PNG
        if (memcmp(fp_ + old_offset, Png::header_magic, sizeof(Png::header_magic)) == 0)
        {
            entries[i].dwBytesInRes = Png(fp_ + old_offset, entries[i].dwBytesInRes).Leanify(size_leanified + old_offset - entries[i].dwImageOffset);
        }
        else
        {
            memmove(fp_ + entries[i].dwImageOffset - size_leanified, fp_ + old_offset, entries[i].dwBytesInRes);
        }
    }

    fp_ -= size_leanified;

    // write new entries
    memcpy(fp_ + 6, entries.data(), entries.size() * sizeof(IconDirEntry));

    // offset + size of last img
    size_ = entries.back().dwImageOffset + entries.back().dwBytesInRes;
    return size_;
}


