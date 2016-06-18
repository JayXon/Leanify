#include "gz.h"

#include <cstdint>
#include <cstring>
#include <iostream>

#include <miniz/miniz.h>
#include <zopfli/deflate.h>

#include "../leanify.h"
#include "../utils.h"

using std::cerr;
using std::endl;
using std::string;

// ID1 ID2 CM
// CM = 8 is deflate
const uint8_t Gz::header_magic[] = { 0x1F, 0x8B, 0x08 };


size_t Gz::Leanify(size_t size_leanified /*= 0*/)
{
    // written according to this specification
    // http://www.gzip.org/zlib/rfc-gzip.html

    if (size_ <= 18)
    {
        cerr << "Not a valid GZ file." << endl;
        return Format::Leanify(size_leanified);
    }

    depth++;
    uint8_t flags = *(fp_ + 3);

    if (size_leanified)
    {
        memmove(fp_ - size_leanified, fp_, 10);
    }

    // set the flags to 0, remove all unnecessary section
    *(fp_ + 3 - size_leanified) = 0;

    uint8_t *p_read = fp_ + 10;
    uint8_t *p_write = p_read - size_leanified;

    *(p_write - 2) = 2;     // XFL

    if (flags & (1 << 2))   // FEXTRA
    {
        p_read += *(uint16_t *)p_read + 2;
    }

    string filename;
    if (flags & (1 << 3))   // FNAME
    {
        filename.assign(reinterpret_cast<char *>(p_read));
        PrintFileName(filename);
        while (p_read < fp_ + size_ && *p_read++)
        {
            // skip string
        }
    }

    if (flags & (1 << 4))   // FCOMMENT
    {
        while (p_read < fp_ + size_ && *p_read++)
        {
            // skip string
        }
    }

    if (flags & (1 << 1))   // FHCRC
    {
        p_read += 2;
    }

    if (p_read >= fp_ + size_)
    {
        memmove(p_write, fp_ + 10, size_ - 10);
        return size_;
    }

    if (is_fast)
    {
        memmove(p_write, p_read, fp_ + size_ - p_read);
        return size_ - (p_read - p_write);
    }

    uint32_t uncompressed_size = *(uint32_t *)(fp_ + size_ - 4);
    uint32_t crc = *(uint32_t *)(fp_ + size_ - 8);
    size_t original_size = fp_ + size_ - 8 - p_read;

    size_t s = 0;
    uint8_t *buffer = static_cast<uint8_t *>(tinfl_decompress_mem_to_heap(p_read, original_size, &s, 0));

    if (!buffer ||
        s != uncompressed_size ||
        crc != mz_crc32(0, buffer, uncompressed_size))
    {
        cerr << "GZ corrupted!" << endl;
        mz_free(buffer);
        memmove(p_write, p_read, original_size + 8);
        return size_ - (p_read - p_write);
    }

    uncompressed_size = LeanifyFile(buffer, uncompressed_size, 0, filename);

    ZopfliOptions options;
    ZopfliInitOptions(&options);
    options.numiterations = iterations;

    uint8_t bp = 0, *out = nullptr;
    size_t outsize = 0;
    ZopfliDeflate(&options, 2, 1, buffer, uncompressed_size, &bp, &out, &outsize);


    if (outsize < original_size)
    {
        memcpy(p_write, out, outsize);
        p_write += outsize;
        *(uint32_t *)p_write = mz_crc32(0, buffer, uncompressed_size);
        *(uint32_t *)(p_write + 4) = uncompressed_size;
    }
    else
    {
        memmove(p_write, p_read, original_size + 8);
        p_write += original_size;
    }
    mz_free(buffer);
    delete[] out;
    depth--;
    fp_ -= size_leanified;
    return p_write + 8 - fp_;
}