#include "leanify.h"

#include <iostream>

#include "lib/miniz/miniz.h"
#include "lib/zopfli/zlib_container.h"

#include "formats/data_uri.h"
#include "formats/dwf.h"
#include "formats/format.h"
#include "formats/gft.h"
#include "formats/gz.h"
#include "formats/ico.h"
#include "formats/jpeg.h"
#include "formats/lua.h"
#include "formats/pe.h"
#include "formats/png.h"
#include "formats/swf.h"
#include "formats/tar.h"
#include "formats/rdb.h"
#include "formats/xml.h"
#include "formats/zip.h"

Format *GetType(void *file_pointer, size_t file_size, std::string &filename)
{
    if (depth > max_depth)
    {
        return new Format(file_pointer, file_size);
    }

    if (!filename.empty())
    {
        size_t dot = filename.find_last_of('.');
        if (dot != std::string::npos)
        {
            std::string ext = filename.substr(dot + 1);
            // toupper
            for (auto &c : ext)
                c &= ~0x20;

            if (ext == "HTML" ||
                ext == "HTM" ||
                ext == "JS" ||
                ext == "CSS")
            {
                if (is_verbose)
                {
                    std::cout << ext << " detected." << std::endl;
                }
                return new DataURI(file_pointer, file_size);
            }
        }

    }
    if (memcmp(file_pointer, Png::header_magic, sizeof(Png::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "PNG detected." << std::endl;
        }
        return new Png(file_pointer, file_size);
    }
    else if (memcmp(file_pointer, Jpeg::header_magic, sizeof(Jpeg::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "JPEG detected." << std::endl;
        }
        return new Jpeg(file_pointer, file_size);
    }
    else if (memcmp(file_pointer, Lua::header_magic, sizeof(Lua::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "Lua detected." << std::endl;
        }
        return new Lua(file_pointer, file_size);
    }
    else if (memcmp(file_pointer, Zip::header_magic, sizeof(Zip::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "ZIP detected." << std::endl;
        }
        return new Zip(file_pointer, file_size);
    }
    else if (memcmp(file_pointer, Pe::header_magic, sizeof(Pe::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "PE detected." << std::endl;
        }
        return new Pe(file_pointer, file_size);
    }
    else if (memcmp(file_pointer, Gz::header_magic, sizeof(Gz::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "GZ detected." << std::endl;
        }
        return new Gz(file_pointer, file_size);
    }
    else if (memcmp(file_pointer, Ico::header_magic, sizeof(Ico::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "ICO detected." << std::endl;
        }
        return new Ico(file_pointer, file_size);
    }
    else if (memcmp(file_pointer, Dwf::header_magic, sizeof(Dwf::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "DWF detected." << std::endl;
        }
        return new Dwf(file_pointer, file_size);
    }
    else if (memcmp(file_pointer, Gft::header_magic, sizeof(Gft::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "GFT detected." << std::endl;
        }
        return new Gft(file_pointer, file_size);
    }
    else if (memcmp(file_pointer, Rdb::header_magic, sizeof(Rdb::header_magic)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "RDB detected." << std::endl;
        }
        return new Rdb(file_pointer, file_size);
    }
    else if (memcmp(file_pointer, Swf::header_magic, sizeof(Swf::header_magic)) == 0 ||
             memcmp(file_pointer, Swf::header_magic_deflate, sizeof(Swf::header_magic_deflate)) == 0 ||
             memcmp(file_pointer, Swf::header_magic_lzma, sizeof(Swf::header_magic_lzma)) == 0)
    {
        if (is_verbose)
        {
            std::cout << "SWF detected." << std::endl;
        }
        return new Swf(file_pointer, file_size);
    }
    else
    {
        // tar file does not have header magic
        // ustar is optional
        {
            Tar *t = new Tar(file_pointer, file_size);
            // checking first record checksum
            if (t->IsValid())
            {
                if (is_verbose)
                {
                    std::cout << "tar detected." << std::endl;
                }
                return t;
            }
            delete t;
        }

        // XML file does not have header magic
        // have to parse and see if there are any errors.
        {
            Xml *x = new Xml(file_pointer, file_size);
            if (x->IsValid())
            {
                if (is_verbose)
                {
                    std::cout << "XML detected." << std::endl;
                }
                return x;
            }
            delete x;
        }
    }

    if (is_verbose)
    {
        std::cout << "Format not supported!" << std::endl;
    }
    // for unsupported format, just memmove it.
    return new Format(file_pointer, file_size);
}


// Leanify the file
// and move the file ahead size_leanified bytes
// the new location of the file will be file_pointer - size_leanified
// it's designed this way to avoid extra memmove or memcpy
// return new size
size_t LeanifyFile(void *file_pointer, size_t file_size, size_t size_leanified /*= 0*/, std::string filename /*= ""*/)
{
    Format *f = GetType(file_pointer, file_size, filename);
    size_t r = f->Leanify(size_leanified);
    delete f;
    return r;
}


size_t ZlibRecompress(void *src, size_t src_len, size_t size_leanified /*= 0*/)
{
    if (!is_fast)
    {
        size_t s = 0;
        uint8_t *buffer = static_cast<uint8_t *>(tinfl_decompress_mem_to_heap(src, src_len, &s, TINFL_FLAG_PARSE_ZLIB_HEADER));
        if (!buffer)
        {
            std::cerr << "Decompress Zlib data failed." << std::endl;
        }
        else
        {
            ZopfliOptions zopfli_options;
            ZopfliInitOptions(&zopfli_options);
            zopfli_options.numiterations = iterations;

            size_t new_size = 0;
            uint8_t *out_buffer = NULL;
            ZopfliZlibCompress(&zopfli_options, buffer, s, &out_buffer, &new_size);
            mz_free(buffer);
            if (new_size < src_len)
            {
                memcpy(static_cast<uint8_t *>(src) - size_leanified, out_buffer, new_size);
                delete[] out_buffer;
                return new_size;
            }
            delete[] out_buffer;
        }
    }

    memmove(static_cast<uint8_t *>(src) - size_leanified, src, src_len);
    return src_len;
}