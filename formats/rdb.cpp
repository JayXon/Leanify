#include "rdb.h"


// 531E98204F8542F0
const unsigned char Rdb::header_magic[] = { 0x35, 0x33, 0x31, 0x45, 0x39, 0x38, 0x32, 0x30, 0x34, 0x46, 0x38, 0x35, 0x34, 0x32, 0x46, 0x30 };


size_t Rdb::Leanify(size_t size_leanified /*= 0*/)
{
    level++;
    char *p_read;
    size_t rdb_size_leanified = 0;

    // header
    p_read = fp;
    fp -= size_leanified;


    uint32_t file_num = *(uint32_t *)(p_read + 0x10);

    uint64_t index_offset = *(uint64_t *)(p_read + 0x14);
    // content offset = index offset + index size
    uint64_t content_offset = index_offset + *(uint64_t *)(p_read + 0x1C);

    if (size_leanified)
    {
        memmove(fp, p_read, (size_t)content_offset);
    }

    char *p_index = fp + index_offset;
    p_read += content_offset;


    for (uint32_t i = 0; i < file_num; i++)
    {
        // index
        // note that on linux wchar_t is 4 bytes instead of 2
        // so this won't work correctly
        // will change to char16_t later
        wchar_t *file_name = (wchar_t *)p_index;


        p_index += (wcslen(file_name) + 1) * 2;


        uint64_t file_size = *(uint64_t *)(p_index + 8);

        *(uint64_t *)p_index -= rdb_size_leanified;

        // skip directories
        if (!file_size)
        {
            p_index += 16;
            continue;
        }

        // output filename
        for (int i = 0; i < level; i++)
        {
            std::cout << "-> ";
        }
        std::wcout << file_name << std::endl;

        // Leanify inner file
        size_t new_size = LeanifyFile(p_read, (size_t)file_size, rdb_size_leanified + size_leanified);
        if (new_size != file_size)
        {
            *(uint64_t *)(p_index + 8) = new_size;
            rdb_size_leanified += (size_t)file_size - new_size;
        }

        p_read += file_size;

        p_index += 16;
    }
    size = p_read - fp - size_leanified - rdb_size_leanified;
    return size;
}