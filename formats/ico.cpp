#include "ico.h"


const unsigned char Ico::header_magic[] = { 0x00, 0x00, 0x01, 0x00 };


size_t Ico::Leanify(size_t size_leanified /*= 0*/)
{
    // number of images inside ico file
    uint16_t n = *(uint16_t *)(fp + 4);
    char *p_index = fp - size_leanified + 6;

    // invalid Icon file
    if (6 + n * 16U >= size || *(uint32_t *)(p_index + 8) + *(uint32_t *)(p_index + 12) > size)
    {
        return Format::Leanify(size_leanified);
    }

    // move header
    if (size_leanified)
    {
        memmove(fp - size_leanified, fp, 6 + n * 16);
    }

    size_t new_size_leanified = 0;
    while (n--)
    {
        uint32_t old_size = *(uint32_t *)(p_index + 8);
        uint32_t offset = *(uint32_t *)(p_index + 12);

        // write new offset
        if (new_size_leanified)
        {
            *(uint32_t *)(p_index + 12) -= new_size_leanified;
        }

        // only Leanify PNG
        if (!memcmp(fp + offset, Png::header_magic, sizeof(Png::header_magic)))
        {
            uint32_t new_size = Png(fp + offset).Leanify(size_leanified + new_size_leanified);
            if (new_size != old_size)
            {
                new_size_leanified += old_size - new_size;
                *(uint32_t *)(p_index + 8) = new_size;
            }
        }
        else if (size_leanified + new_size_leanified)
        {
            memmove(fp + offset - size_leanified - new_size_leanified, fp + offset, old_size);
        }
        p_index += 16;
    }

    fp -= size_leanified;
    // offset + size of last file
    return *(uint32_t *)(p_index - 4) + *(uint32_t *)(p_index - 8);
}


