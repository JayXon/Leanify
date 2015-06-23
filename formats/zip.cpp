#include "zip.h"

#include <algorithm>    // std::search
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "../lib/miniz/miniz.h"

#include "../leanify.h"


const unsigned char Zip::header_magic[] = { 0x50, 0x4B, 0x03, 0x04 };

size_t Zip::Leanify(size_t size_leanified /*= 0*/)
{
    depth++;
    char *p_read = fp;
    fp -= size_leanified;
    char *p_write = fp;

    std::vector<uint32_t> vector_local_header_offset;
    // Local file header
    while (memcmp(p_read, header_magic, sizeof(header_magic)) == 0)
    {
        vector_local_header_offset.push_back(p_write - fp);

        uint16_t filename_length = *(uint16_t *)(p_read + 26);

        size_t header_size = 30 + filename_length;
        // move header
        if (p_read - p_write)
        {
            memmove(p_write, p_read, header_size);
        }

        // if Extra field length is not 0, then skip it and set it to 0
        if (*(uint16_t *)(p_write + 28))
        {
            p_read += *(uint16_t *)(p_write + 28);
            *(uint16_t *)(p_write + 28) = 0;
        }

        uint32_t *crc = (uint32_t *)(p_write + 14);
        uint32_t *compressed_size = crc + 1;
        uint32_t *uncompressed_size = compressed_size + 1;

        uint32_t orig_comp_size = *compressed_size;

        uint16_t flag = *(uint16_t *)(p_write + 6);
        uint16_t *compression_method = (uint16_t *)(p_write + 8);

        std::string filename(p_write + 30, filename_length);
        // do not output filename if it is a directory
        if ((orig_comp_size || *compression_method || flag & 8) && depth <= max_depth)
        {
            // output filename
            for (int i = 1; i < depth; i++)
            {
                std::cout << "-> ";
            }
            std::cout << filename << std::endl;
        }


        // From Wikipedia:
        // If bit 3 (0x08) of the general-purpose flags field is set,
        // then the CRC-32 and file sizes are not known when the header is written.
        // The fields in the local header are filled with zero,
        // and the CRC-32 and size are appended in a 12-byte structure
        // (optionally preceded by a 4-byte signature) immediately after the compressed data
        if (flag & 8)
        {
            // set this bit to 0
            *(uint16_t *)(p_write + 6) &= ~8;

            // data descriptor signature
            const unsigned char dd_sign[] = { 0x50, 0x4B, 0x07, 0x08 };
            // search for signature
            char *dd = p_read + header_size;
            while (*(uint32_t *)(dd + 8) != dd - p_read - header_size)
            {
                dd = std::search(dd + 1, fp + size + size_leanified, dd_sign, dd_sign + 4);
                if (dd == fp + size + size_leanified)
                {
                    std::cerr << "data descriptor signature not found!" << std::endl;
                    // abort
                    // zip does not have 4-byte signature preceded
                    return size;
                }
            }
            *crc = *(uint32_t *)(dd + 4);
            *compressed_size = orig_comp_size = *(uint32_t *)(dd + 8);
            *uncompressed_size = *(uint32_t *)(dd + 12);
        }

        // if compression method is not deflate or fast mode
        // then only Leanify embedded file if the method is store
        // otherwise just memmove the compressed part
        if (*compression_method != 8 || is_fast)
        {
            if (*compression_method == 0 && depth <= max_depth)
            {
                // method is store
                if (orig_comp_size)
                {
                    uint32_t new_size = LeanifyFile(p_read + header_size, orig_comp_size, p_read - p_write, filename);
                    p_read += header_size + orig_comp_size;
                    *compressed_size = *uncompressed_size = new_size;
                    *crc = mz_crc32(0, (unsigned char *)p_write + header_size, new_size);
                }
                else
                {
                    p_read += header_size;
                }
            }
            else
            {
                // unsupported compression method, move it
                memmove(p_write + header_size, p_read + header_size, orig_comp_size);
                p_read += header_size + orig_comp_size;

            }
            p_write += header_size + *compressed_size;

        }
        else
        {
            // the method is deflate, uncompress it and recompress with zopfli

            p_read += header_size;
            p_write += header_size;

            if (*uncompressed_size)
            {
                // uncompress
                size_t s = 0;
                unsigned char *buffer = (unsigned char *)tinfl_decompress_mem_to_heap(p_read, orig_comp_size, &s, 0);

                if (!buffer ||
                    s != *uncompressed_size ||
                    *crc != mz_crc32(0, buffer, *uncompressed_size))
                {
                    std::cerr << "ZIP file corrupted!" << std::endl;
                    mz_free(buffer);
                    memmove(p_write, p_read, orig_comp_size);
                    p_read += orig_comp_size;
                    p_write += orig_comp_size;
                    continue;
                }

                // Leanify uncompressed file
                uint32_t new_uncomp_size = s;
                // workaround of TinyXML2 not supporting xml:space="preserve"
                if (filename_length != 17 || filename != "word/document.xml")
                {
                    new_uncomp_size = LeanifyFile(buffer, s, 0, filename);
                }

                // recompress
                unsigned char bp = 0, *out = NULL;
                size_t new_comp_size = 0;
                ZopfliDeflate(&zopfli_options, 2, 1, buffer, new_uncomp_size, &bp, &out, &new_comp_size);

                // switch to store if deflate makes file larger
                if (new_uncomp_size <= new_comp_size && new_uncomp_size <= orig_comp_size)
                {
                    *compression_method = 0;
                    *crc = mz_crc32(0, buffer, new_uncomp_size);
                    *compressed_size = new_uncomp_size;
                    *uncompressed_size = new_uncomp_size;
                    memcpy(p_write, buffer, new_uncomp_size);
                    p_write += new_uncomp_size;
                }
                else if (new_comp_size < orig_comp_size)
                {
                    *crc = mz_crc32(0, buffer, new_uncomp_size);
                    *compressed_size = new_comp_size;
                    *uncompressed_size = new_uncomp_size;
                    memcpy(p_write, out, new_comp_size);
                    p_write += new_comp_size;
                }
                else
                {
                    memmove(p_write, p_read, orig_comp_size);
                    p_write += orig_comp_size;
                }
                p_read += orig_comp_size;

                mz_free(buffer);
                delete[] out;
            }
            else
            {
                *compression_method = 0;
                *compressed_size = 0;
                p_read += orig_comp_size;
            }
        }

        // we don't use data descriptor, so that can save more bytes (16 per file)
        if (flag & 8)
        {
            p_read += 16;
        }
    }

    char *central_directory = p_write;
    // Central directory file header
    const unsigned char cd_header_magic[] = { 0x50, 0x4B, 0x01, 0x02 };
    int i = 0;
    while (memcmp(p_read, cd_header_magic, sizeof(cd_header_magic)) == 0)
    {

        int header_size = 46 + *(uint16_t *)(p_read + 28);
        // move header
        if (p_read - p_write)
        {
            memmove(p_write, p_read, header_size);
        }

        // set bit 3 of General purpose bit flag to 0
        *(uint16_t *)(p_write + 8) &= ~8;

        // if Extra field length is not 0, then skip it and set it to 0
        if (*(uint16_t *)(p_write + 30))
        {
            p_read += *(uint16_t *)(p_write + 30);
            *(uint16_t *)(p_write + 30) = 0;
        }

        // if File comment length is not 0, then skip it and set it to 0
        if (*(uint16_t *)(p_write + 32))
        {
            p_read += *(uint16_t *)(p_write + 32);
            *(uint16_t *)(p_write + 32) = 0;
        }

        char *local_header = fp + vector_local_header_offset[i];

        // copy new CRC-32, Compressed size, Uncompressed size
        // from Local file header to Central directory file header
        memcpy(p_write + 16, local_header + 14, 12);

        // update compression method
        *(uint16_t *)(p_write + 10) = *(uint16_t *)(local_header + 8);

        // new Local file header offset
        *(uint32_t *)(p_write + 42) = vector_local_header_offset[i];

        i++;

        p_read += header_size;
        p_write += header_size;
    }

    // End of central directory record
    const unsigned char eocd_header_magic[] = { 0x50, 0x4B, 0x05, 0x06 };
    if (memcmp(p_read, eocd_header_magic, sizeof(eocd_header_magic)))
    {
        std::cerr << "EOCD not found!" << std::endl;

    }
    if (p_read - p_write)
    {
        memmove(p_write, p_read, 12);
    }
    // central directory size
    *(uint32_t *)(p_write + 12) = p_write - central_directory;
    // central directory offset
    *(uint32_t *)(p_write + 16) = central_directory - fp;
    // set comment length to 0
    *(uint16_t *)(p_write + 20) = 0;

    // 22 is the length of EOCD
    return p_write + 22 - fp;
}

