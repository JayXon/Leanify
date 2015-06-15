#include "rdb.h"

#include <cstdint>
#include <cstring>
#include <iostream>

#include "../leanify.h"
#include "../utils.h"


// 531E98204F8542F0
const unsigned char Rdb::header_magic[] = { 0x35, 0x33, 0x31, 0x45, 0x39, 0x38, 0x32, 0x30, 0x34, 0x46, 0x38, 0x35, 0x34, 0x32, 0x46, 0x30 };


size_t Rdb::Leanify(size_t size_leanified /*= 0*/)
{
    if (size <= 0x20)
    {
        std::cerr << "Not a valid RDB file." << std::endl;
        return Format::Leanify(size_leanified);
    }

    depth++;
    char *p_read;
    size_t rdb_size_leanified = 0;

    // header
    p_read = fp;
    fp -= size_leanified;

    // total number of files including directory
    uint32_t file_num = *(uint32_t *)(p_read + 0x10);

    uint64_t index_offset = *(uint64_t *)(p_read + 0x14);

    uint64_t content_offset = index_offset + *(uint64_t *)(p_read + 0x1C);

    // move header and indexes
    if (size_leanified)
    {
        memmove(fp, p_read, (size_t)content_offset);
    }

    char *p_index = fp + index_offset;
    p_read += content_offset;


    for (uint32_t i = 0; i < file_num; i++)
    {
        // index
        wchar_t *file_name = (wchar_t *)p_index;

        // note that on Linux wchar_t is 4 bytes instead of 2
        // so I can't use wcslen
        // p_index += (wcslen(file_name) + 1) * 2;
        while (*(uint16_t *)p_index)
        {
            p_index += 2;
        }
        p_index += 2;

        uint64_t file_size = *(uint64_t *)(p_index + 8);

        *(uint64_t *)p_index -= rdb_size_leanified;

        // skip directories
        if (!file_size)
        {
            p_index += 16;
            continue;
        }

        if (depth <= max_depth)
        {
            // output filename
            for (int i = 1; i < depth; i++)
            {
                std::cout << "-> ";
            }

            char mbs[256] = { 0 };
            UTF16toMBS(file_name, p_index - (char *)file_name, mbs, sizeof(mbs));
            std::cout << mbs << std::endl;

            // Leanify inner file
            size_t new_size = LeanifyFile(p_read, (size_t)file_size, rdb_size_leanified + size_leanified, std::string(mbs));
            if (new_size != file_size)
            {
                // update the size in index
                *(uint64_t *)(p_index + 8) = new_size;
                rdb_size_leanified += (size_t)file_size - new_size;
            }
        }
        else
        {
            memmove(p_read - rdb_size_leanified - size_leanified, p_read, (size_t)file_size);
        }

        p_read += file_size;

        p_index += 16;
    }
    size = p_read - fp - size_leanified - rdb_size_leanified;
    depth--;
    return size;
}