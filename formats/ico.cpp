#include "ico.h"

#include <cstdint>

#include "png.h"


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

    IconDirEntry *icon_dir_entry = reinterpret_cast<IconDirEntry *>(fp_ - size_leanified + 6);

    // invalid Icon file
    if (6 + num_of_img * 16U >= size_ ||
        icon_dir_entry->dwImageOffset + icon_dir_entry->dwBytesInRes > size_ ||
        icon_dir_entry[num_of_img - 1].dwImageOffset + icon_dir_entry[num_of_img - 1].dwBytesInRes > size_)
    {
        return Format::Leanify(size_leanified);
    }

    // move header
    if (size_leanified)
    {
        memmove(fp_ - size_leanified, fp_, 6 + num_of_img * 16);
    }

    size_t new_size_leanified = 0;
    while (num_of_img--)
    {
        uint32_t old_offset = icon_dir_entry->dwImageOffset;
        // write new offset
        if (new_size_leanified)
        {
            icon_dir_entry->dwImageOffset -= new_size_leanified;
        }

        // only Leanify PNG
        if (memcmp(fp_ + old_offset, Png::header_magic, sizeof(Png::header_magic)) == 0)
        {
            uint32_t new_size = Png(fp_ + old_offset, icon_dir_entry->dwBytesInRes).Leanify(size_leanified + new_size_leanified);
            if (new_size != icon_dir_entry->dwBytesInRes)
            {
                new_size_leanified += icon_dir_entry->dwBytesInRes - new_size;
                icon_dir_entry->dwBytesInRes = new_size;
            }
        }
        else if (size_leanified + new_size_leanified)
        {
            memmove(fp_ + icon_dir_entry->dwImageOffset - size_leanified, fp_ + old_offset, icon_dir_entry->dwBytesInRes);
        }
        icon_dir_entry++;
    }

    fp_ -= size_leanified;
    // offset + size of last file
    icon_dir_entry--;
    size_ = icon_dir_entry->dwImageOffset + icon_dir_entry->dwBytesInRes;
    return size_;
}


