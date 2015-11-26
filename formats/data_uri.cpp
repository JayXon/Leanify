#include "data_uri.h"

#include <algorithm>
#include <iostream>
#include <string>

#include "base64.h"

using std::cout;
using std::endl;
using std::string;

size_t DataURI::Leanify(size_t size_leanified /*= 0*/)
{
    uint8_t *p_read = fp_, *p_write = fp_ - size_leanified;

    while (p_read < fp_ + size_)
    {
        const string magic = "data:image/";
        uint8_t *new_p_read = std::search(p_read, fp_ + size_, magic.begin(), magic.end());
        // move anything in between
        memmove(p_write, p_read, new_p_read - p_read);
        p_write += new_p_read - p_read;
        p_read = new_p_read;

        if (p_read >= fp_ + size_)
        {
            break;
        }

        const string base64_sign = ";base64,";
        // data:image/png;base64,   data:image/jpeg;base64,
        // maximum gap is 4
        uint8_t *search_end = p_read + magic.size() + 4 + base64_sign.size();
        uint8_t *start = std::search(p_read + magic.size(), search_end, base64_sign.begin(), base64_sign.end()) + base64_sign.size();

        memmove(p_write, p_read, start - p_read);
        p_write += start - p_read;
        p_read = start;

        if (p_read > search_end)
        {
            continue;
        }

        const char quote[] = { '\'', '"', ')' };
        uint8_t *end = std::find_first_of(p_read, fp_ + size_, quote, quote + sizeof(quote));
        if (end < fp_ + size_)
        {
            if (is_verbose)
            {
                cout << string(reinterpret_cast<char *>(new_p_read), start + 8 - new_p_read) << "... found at offset 0x" << std::hex << new_p_read - fp_ << endl;
            }
            size_t new_size = Base64(p_read, end - p_read).Leanify(p_read - p_write);
            p_write += new_size;
            p_read = end;
        }
        else
        {
            memmove(p_write, p_read, fp_ + size_ - p_read);
            p_write += fp_ + size_ - p_read;
            p_read = fp_ + size_;
        }
    }
    fp_ -= size_leanified;
    size_ = p_write - fp_;
    return size_;
}
