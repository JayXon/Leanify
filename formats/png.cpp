#include "png.h"

#include <cstdint>
#include <cstring>
#include <iostream>

#include "../lib/miniz/miniz.h"
#include "../lib/zopflipng/zopflipng_lib.h"

#include "../leanify.h"

#ifdef _MSC_VER
#   define bswap32(x) _byteswap_ulong(x)
#elif defined __GNUC__
#   define bswap32(x) __builtin_bswap32(x)
#else
#   define bswap32(x) _bswap(x)
#endif

using std::cout;
using std::endl;
using std::vector;


const uint8_t Png::header_magic[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };



size_t Png::Leanify(size_t size_leanified /*= 0*/)
{
    // header
    uint8_t *p_read = fp;
    uint8_t *p_write = p_read - size_leanified;

    if (size_leanified)
    {
        memmove(p_write, p_read, sizeof(header_magic));
    }

    p_read += sizeof(header_magic);
    p_write += sizeof(header_magic);

    // chunk
    uint32_t chunk_type;

    uint8_t *idat_addr = nullptr;

    do
    {

        // read chunk length
        // use bswap to convert Big-Endian to Little-Endian
        // 12 = length: 4 + type: 4 + crc: 4
        uint32_t chunk_length = bswap32(*(uint32_t *)p_read) + 12;

        // detect truncated file
        if (p_read + chunk_length > fp + size)
        {
            memmove(p_write, p_read, fp + size - p_read);
            p_write += fp + size - p_read;
            p_read = fp + size;
            break;
        }

        // read chunk type
        chunk_type = *(uint32_t *)(p_read + 4);

        // judge the case of first letter
        // remove all ancillary chunks except tRNS and APNG chunks and npTc
        // tRNS has transparency information
        if (chunk_type & 0x20)
        {

            switch (chunk_type)
            {
            case 0x534E5274:    // tRNS     transparent
            case 0x4C546361:    // acTL     APNG
            case 0x4C546366:    // fcTL     APNG
            case 0x54416466:    // fdAT     APNG    TODO: use zopfli to recompress fdAT
            case 0x6354706E:    // npTc     Android 9Patch images (*.9.png)
                break;

            default:
                if (is_verbose)
                {
                    // chunk name
                    for (int i = 4; i < 8; i++)
                    {
                        cout << static_cast<char>(p_read[i]);
                    }
                    cout << " chunk removed." << endl;
                }
                // remove this chunk
                p_read += chunk_length;
                continue;
            }

        }

        // move this chunk
        if (p_write != p_read)
        {
            memmove(p_write, p_read, chunk_length);
        }

        // save IDAT chunk address
        if (chunk_type == 0x54414449)
        {
            idat_addr = p_write;
        }

        // skip whole chunk
        p_write += chunk_length;
        p_read += chunk_length;


    }
    while (chunk_type != 0x444E4549);       // IEND

    fp -= size_leanified;
    uint32_t png_size = p_write - fp;


    ZopfliPNGOptions zopflipng_options;
    zopflipng_options.use_zopfli = !is_fast;
    zopflipng_options.lossy_transparent = true;
    // see the switch above for information about these chunks
    zopflipng_options.keepchunks = { "acTL", "fcTL", "fdAT", "npTc" };
    zopflipng_options.num_iterations = iterations;
    zopflipng_options.num_iterations_large = iterations / 3 + 1;
    // trying both methods does not worth the time it spend
    // it's better to use the time for more iterations.
    // zopflipng_options.block_split_strategy = 3;


    const vector<uint8_t> origpng(fp, fp + png_size);
    vector<uint8_t> resultpng;

    if (!ZopfliPNGOptimize(origpng, zopflipng_options, is_verbose, &resultpng))
    {
        // only use the result PNG if it is smaller
        // sometimes the original PNG is already highly optimized
        // then maybe ZopfliPNG will produce bigger file
        if (resultpng.size() < png_size)
        {
            memcpy(fp, resultpng.data(), resultpng.size());
            return resultpng.size();
        }
    }

    if (idat_addr == nullptr)
    {
        return png_size;
    }

    // sometimes the strategy chosen by ZopfliPNG is worse than original
    // then try to recompress IDAT chunk using only zopfli
    uint32_t idat_length = bswap32(*(uint32_t *)idat_addr);
    uint32_t new_idat_length = ZlibRecompress(idat_addr + 8, idat_length);
    if (idat_length != new_idat_length)
    {
        *(uint32_t *)idat_addr = bswap32(new_idat_length);
        *(uint32_t *)(idat_addr + new_idat_length + 8) = bswap32(mz_crc32(0, idat_addr + 4, new_idat_length + 4));
        uint8_t *idat_end = idat_addr + idat_length + 12;
        memmove(idat_addr + new_idat_length + 12, idat_end, fp + png_size - idat_end);
        png_size -= idat_length - new_idat_length;
    }

    return png_size;
}
