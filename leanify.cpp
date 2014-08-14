#include "leanify.h"


// return new size
size_t LeanifyFile(void *file_pointer, size_t file_size, size_t size_leanified /*= 0*/)
{
    if (!memcmp(file_pointer, Png::header_magic, sizeof(Png::header_magic)))
    {
        if (is_verbose)
        {
            std::cout << "PNG detected." << std::endl;
        }
        return Png(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (!memcmp(file_pointer, Jpeg::header_magic, sizeof(Jpeg::header_magic)))
    {
        if (is_verbose)
        {
            std::cout << "JPEG detected." << std::endl;
        }
        return Jpeg(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (!memcmp(file_pointer, Lua::header_magic, sizeof(Lua::header_magic)))
    {
        if (is_verbose)
        {
            std::cout << "Lua detected." << std::endl;
        }
        return Lua(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (!memcmp(file_pointer, Zip::header_magic, sizeof(Zip::header_magic)))
    {
        if (is_verbose)
        {
            std::cout << "ZIP detected." << std::endl;
        }
        return Zip(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (!memcmp(file_pointer, Gz::header_magic, sizeof(Gz::header_magic)))
    {
        if (is_verbose)
        {
            std::cout << "GZ detected." << std::endl;
        }
        return Gz(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (!memcmp(file_pointer, Ico::header_magic, sizeof(Ico::header_magic)))
    {
        if (is_verbose)
        {
            std::cout << "ICO detected." << std::endl;
        }
        return Ico(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (!memcmp(file_pointer, Gft::header_magic, sizeof(Gft::header_magic)))
    {
        if (is_verbose)
        {
            std::cout << "GFT detected." << std::endl;
        }
        return Gft(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (!memcmp(file_pointer, Rdb::header_magic, sizeof(Rdb::header_magic)))
    {
        if (is_verbose)
        {
            std::cout << "RDB detected." << std::endl;
        }
        return Rdb(file_pointer, file_size).Leanify(size_leanified);
    }
    else if (!memcmp(file_pointer, Swf::header_magic, sizeof(Swf::header_magic)) ||
             !memcmp(file_pointer, Swf::header_magic_deflate, sizeof(Swf::header_magic_deflate)) ||
             !memcmp(file_pointer, Swf::header_magic_lzma, sizeof(Swf::header_magic_lzma)))
    {
        if (is_verbose)
        {
            std::cout << "SWF detected." << std::endl;
        }
        return Swf(file_pointer, file_size).Leanify(size_leanified);
    }
    else
    {
        // tar file does not have header magic
        // ustar is optional
        // check file size first
        if (file_size > 512 && file_size % 512 == 0)
        {
            Tar t(file_pointer, file_size);
            // checking first record checksum
            if (t.IsValid())
            {
                if (is_verbose)
                {
                    std::cout << "tar detected." << std::endl;
                }
                return t.Leanify(size_leanified);
            }
        }

        // xml file does not have header magic to tell if it is a xml file.
        // have to try to parse and see if there is any errors.
        Xml x(file_pointer, file_size);
        if (x.IsValid())
        {
            if (is_verbose)
            {
                std::cout << "XML detected." << std::endl;
            }
            return x.Leanify(size_leanified);
        }
    }

    if (is_verbose)
    {
        std::cout << "Format not supported!" << std::endl;
    }
    // for unsupported format, just memmove it.
    return Format(file_pointer, file_size).Leanify(size_leanified);
}


size_t ZlibRecompress(unsigned char *src, size_t src_len, size_t size_leanified /*= 0*/)
{
    if (!is_fast)
    {
        size_t s = 0;
        unsigned char *buffer = (unsigned char *)tinfl_decompress_mem_to_heap(src, src_len, &s, TINFL_FLAG_PARSE_ZLIB_HEADER);
        if (!buffer)
        {
            std::cout << "Decompress Zlib failed." << std::endl;
        }
        else
        {
            ZopfliOptions zopfli_options;
            ZopfliInitOptions(&zopfli_options);
            zopfli_options.numiterations = iterations;

            size_t new_size = 0;
            unsigned char *out_buffer = NULL;
            ZopfliZlibCompress(&zopfli_options, buffer, s, &out_buffer, &new_size);
            mz_free(buffer);
            if (new_size < src_len)
            {
                memcpy(src - size_leanified, out_buffer, new_size);
                delete[] out_buffer;
                return new_size;
            }
            delete[] out_buffer;
        }
    }

    memmove(src - size_leanified, src, src_len);
    return src_len;
}