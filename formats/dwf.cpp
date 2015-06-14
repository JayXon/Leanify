#include "dwf.h"

#include "zip.h"

// (DWF V06.00)PK
const unsigned char Dwf::header_magic[] = { 0x28, 0x44, 0x57, 0x46, 0x20, 0x56, 0x30, 0x36, 0x2E, 0x30, 0x30, 0x29, 0x50, 0x4B, 0x03, 0x04 };

size_t Dwf::Leanify(size_t size_leanified /*= 0*/)
{
    const size_t dwf_magic_len = 12;
    if (size_leanified)
    {
        memmove(fp - size_leanified, fp, dwf_magic_len);
    }
    Zip zip(fp + dwf_magic_len, size - dwf_magic_len);
    size = zip.Leanify(size_leanified) + dwf_magic_len;
    fp -= size_leanified;
    return size;
}

