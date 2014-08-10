#include "gz.h"


// ID1 ID2 CM
// CM = 8 is deflate
const unsigned char Gz::header_magic[] = { 0x1F, 0x8B, 0x08 };


size_t Gz::Leanify(size_t size_leanified /*= 0*/)
{
    // written according to this specification
    // http://www.gzip.org/zlib/rfc-gzip.html

    if (size_leanified)
    {
        memmove(fp - size_leanified, fp, 10);
    }

    char flags = *(fp + 3);
    // set the flags to 0, remove all unnecessary section
    *(fp + 3 - size_leanified) = 0;

    char *p_read = fp + 10;
    char *p_write = p_read - size_leanified;

    *(p_write - 2) = 2;     // XFL

    if (flags & (1 << 2))   // FEXTRA
    {
        p_read += *(uint16_t *)p_read + 2;
    }

    if (flags & (1 << 3))   // FNAME
    {
        while (*p_read++)
        {
            // skip string
        }
    }

    if (flags & (1 << 4))   // FCOMMENT
    {
        while (*p_read++)
        {
            // skip string
        }
    }

    if (flags & (1 << 1))   // FHCRC
    {
        p_read += 2;
    }

    if (!is_recompress)
    {
        memmove(p_write, p_read, fp + size - p_read);
        return size - (p_read - p_write);
    }

    uint32_t uncompressed_size = *(uint32_t *)(fp + size - 4);
    uint32_t crc = *(uint32_t *)(fp + size - 8);
    size_t original_size = fp + size - 8 - p_read;

    size_t s = 0;
    unsigned char *buffer = (unsigned char *)tinfl_decompress_mem_to_heap(p_read, original_size, &s, 0);

    if (!buffer ||
        s != uncompressed_size ||
        crc != crc32(0, buffer, uncompressed_size))
    {
        std::cout << "GZ corrupted!" << std::endl;
        mz_free(buffer);
        memmove(p_write, p_read, original_size + 8);
        return size - (p_read - p_write);
    }

    uncompressed_size = LeanifyFile(buffer, uncompressed_size);
    
    ZopfliOptions options;
    ZopfliInitOptions(&options);
    options.numiterations = iterations;

    unsigned char bp = 0, *out = NULL;
    size_t outsize = 0;
    ZopfliDeflate(&options, 2, 1, buffer, uncompressed_size, &bp, &out, &outsize);


    if (outsize < original_size)
    {
        memcpy(p_write, out, outsize);
        p_write += outsize;
        *(uint32_t *)p_write = crc32(0, buffer, uncompressed_size);
        *(uint32_t *)(p_write + 4) = uncompressed_size;
    }
    else
    {
        memmove(p_write, p_read, original_size);
        p_write += original_size;
    }
    mz_free(buffer);
    delete[] out;

    fp -= size_leanified;
    return p_write + 8 - fp;
}