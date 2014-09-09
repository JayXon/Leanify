#include "swf.h"

const unsigned char Swf::header_magic[] = { 'F', 'W', 'S' };
const unsigned char Swf::header_magic_deflate[] = { 'C', 'W', 'S' };
const unsigned char Swf::header_magic_lzma[] = { 'Z', 'W', 'S' };

size_t Swf::Leanify(size_t size_leanified /*= 0*/)
{
    if (is_fast && *fp != 'F')
    {
        return Format::Leanify(size_leanified);
    }

    unsigned char *in_buffer = (unsigned char *)fp + 8;
    uint32_t in_len = *(uint32_t *)(fp + 4) - 8;

    // if SWF is compressed, decompress it first
    if (*fp == 'C')
    {
        // deflate
        if (is_verbose)
        {
            std::cout << "SWF is compressed with deflate." << std::endl;
        }
        size_t s = 0;
        in_buffer = (unsigned char *)tinfl_decompress_mem_to_heap(in_buffer, size - 8, &s, TINFL_FLAG_PARSE_ZLIB_HEADER);
        if (!in_buffer || s != in_len)
        {
            std::cerr << "SWF file corrupted!" << std::endl;
            mz_free(in_buffer);
            return Format::Leanify(size_leanified);
        }
    }
    else if (*fp == 'Z')
    {
        // LZMA
        if (is_verbose)
        {
            std::cout << "SWF is compressed with LZMA." << std::endl;
        }
        // | 4 bytes         | 4 bytes   | 4 bytes       | 5 bytes    | n bytes   | 6 bytes         |
        // | 'ZWS' + version | scriptLen | compressedLen | LZMA props | LZMA data | LZMA end marker |
        unsigned char *dst_buffer = new unsigned char[in_len];
        size_t s = in_len, len = size - 12 - LZMA_PROPS_SIZE;
        in_buffer += 4;
        if (LzmaUncompress(dst_buffer, &s, in_buffer + LZMA_PROPS_SIZE, &len, in_buffer, LZMA_PROPS_SIZE) ||
            s != in_len || len != size - 18 - LZMA_PROPS_SIZE)
        {
            std::cerr << "SWF file corrupted!" << std::endl;
            delete[] dst_buffer;
            return Format::Leanify(size_leanified);
        }
        in_buffer = dst_buffer;
    }
    else if (is_verbose)
    {
        std::cout << "SWF is not compressed." << std::endl;
    }

    // parsing SWF tags
    unsigned char *p = in_buffer + 13;      // skip FrameSize(9B) + FrameRate(2B) + FrameCount(2B) = 13B
    size_t tag_size_leanified = 0;
    do
    {
        uint16_t tag_type = *(uint16_t *)p >> 6;
        uint32_t tag_length = *p & 0x3F;
        size_t tag_header_length = 2;
        if (tag_length == 0x3F)
        {
            tag_length = *(uint32_t *)(p + 2);
            tag_header_length += 4;
        }

        if (tag_size_leanified)
        {
            memmove(p - tag_size_leanified, p, tag_header_length);
        }

        p += tag_header_length;

        switch (tag_type)
        {
        case 20:    // DefineBitsLossless
        case 36:    // DefineBitsLossless2
        {
            // move header
            size_t header_size = 7 + (p[3] == 3);
            if (is_verbose)
            {
                std::cout << "DefineBitsLossless tag found." << std::endl;
            }
            if (tag_size_leanified)
            {
                memmove(p - tag_size_leanified, p, header_size);
            }
            // recompress Zlib bitmap data
            size_t new_data_size = ZlibRecompress(p + header_size, tag_length - header_size, tag_size_leanified);
            
            UpdateTagLength(p - tag_size_leanified, tag_header_length, header_size + new_data_size);
            tag_size_leanified += tag_length - header_size - new_data_size;
            p += tag_length;
            continue;
        }
        case 21:    // DefineBitsJPEG2
        {
            if (is_verbose)
            {
                std::cout << "DefineBitsJPEG2 tag found." << std::endl;
            }
            // copy id
            *(uint16_t *)(p - tag_size_leanified) = *(uint16_t *)p;

            // Leanify embedded image
            size_t new_size = LeanifyFile(p + 2, tag_length - 2, tag_size_leanified);

            UpdateTagLength(p - tag_size_leanified, tag_header_length, 2 + new_size);
            tag_size_leanified += tag_length - 2 - new_size;
            p += tag_length;
            continue;
        }
        case 35:    // DefineBitsJPEG3
        case 90:    // DefineBitsJPEG4
        {
            // copy id
            *(uint16_t *)(p - tag_size_leanified) = *(uint16_t *)p;

            uint32_t img_size = *(uint32_t *)(p + 2);
            size_t header_size = tag_type == 90 ? 8 : 6;

            if (is_verbose)
            {
                std::cout << "DefineBitsJPEG" << header_size / 2 << " tag found." << std::endl;
            }
            // Leanify embedded image
            size_t new_img_size = LeanifyFile(p + header_size, img_size, tag_size_leanified);
            *(uint32_t *)(p + 2 - tag_size_leanified) = new_img_size;

            // recompress alpha data
            size_t new_alpha_data_size = ZlibRecompress(p + header_size + img_size, tag_length - img_size - header_size, tag_size_leanified + img_size - new_img_size);

            size_t new_tag_size = new_img_size + new_alpha_data_size + header_size;
            UpdateTagLength(p - tag_size_leanified, tag_header_length, new_tag_size);
            tag_size_leanified += tag_length - new_tag_size;
            p += tag_length;
            continue;
        }
        case 69:    // FileAttributes
            *p &= ~(1 << 4);    // set HasMetadata bit to 0
            break;
        case 77:    // Metadata
            if (is_verbose)
            {
                std::cout << "Metadata removed." << std::endl;
            }
            tag_size_leanified += tag_length + tag_header_length;
            p += tag_length;
            continue;
        }
        if (tag_size_leanified)
        {
            memmove(p - tag_size_leanified, p, tag_length);
        }
        p += tag_length;
    } while (p < in_buffer + in_len);

    in_len -= tag_size_leanified;

    if (is_fast)
    {
        // write header
        fp -= size_leanified;

        // decompressed size (including header)
        *(uint32_t *)(fp + 4) = size = in_len + 8;

        if (size_leanified)
        {
            memmove(fp, fp + size_leanified, 4);
            memmove(fp + 8, fp + 8 + size_leanified, in_len);
        }
        return size;
    }

    // compress with LZMA
    size_t s = in_len, props = LZMA_PROPS_SIZE;
    unsigned char *dst = new unsigned char[in_len + LZMA_PROPS_SIZE];
    // have to set writeEndMark to true
    if (LzmaCompress(dst + LZMA_PROPS_SIZE, &s, in_buffer, in_len, dst, &props, iterations < 9 ? iterations : 9, 1 << 24, -1, -1, -1, 128, -1))
    {
        std::cerr << "LZMA compression failed." << std::endl;
        s = SIZE_MAX;
    }

    // free decompressed data
    if (*fp == 'C')
    {
        mz_free(in_buffer);
    }
    else if (*fp == 'Z')
    {
        delete[] in_buffer;
    }

    // write header
    fp -= size_leanified;
    memcpy(fp, header_magic_lzma, sizeof(header_magic_lzma));

    // write SWF version, at least 13 to support LZMA
    if (fp[3 + size_leanified] < 13)
    {
        fp[3] = 13;
    }
    else
    {
        fp[3] = fp[3 + size_leanified];
    }

    // decompressed size (including header)
    *(uint32_t *)(fp + 4) = in_len + 8;

    s += LZMA_PROPS_SIZE;
    if (s + 12 < size)
    {
        size = s + 12;
        *(uint32_t *)(fp + 8) = s - LZMA_PROPS_SIZE;
        memcpy(fp + 12, dst, s);
    }
    else if (size_leanified)
    {
        memmove(fp + 8, fp + 8 + size_leanified, size - 8);
    }

    delete[] dst;

    return size;
}



void Swf::UpdateTagLength(unsigned char *tag_content, size_t header_length, size_t new_length)
{
    if (header_length == 6)
    {
        *(uint32_t *)(tag_content - 4) = new_length;
    }
    else
    {
        *(tag_content - 2) += (new_length & 0x3F) - (*(tag_content - 2) & 0x3F);
    }
}
