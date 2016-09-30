#include "data_uri.h"

#include <algorithm>
#include <iostream>
#include <string>

#include "base64.h"

using std::cout;
using std::endl;
using std::string;

size_t DataURI::Leanify(size_t size_leanified /*= 0*/) {
  uint8_t *p_read = fp_, *p_write = fp_ - size_leanified;

  while (p_read < fp_ + size_) {
    const string magic = "data:image/";
    uint8_t* data_magic = std::search(p_read, fp_ + size_, magic.begin(), magic.end());

    if (data_magic >= fp_ + size_) {
      memmove(p_write, p_read, fp_ + size_ - p_read);
      p_write += fp_ + size_ - p_read;
      break;
    }

    const string base64_sign = ";base64,";
    const size_t kMaxSearchGap = 64;
    uint8_t* search_end = data_magic + magic.size() + kMaxSearchGap + base64_sign.size();
    search_end = std::min(search_end, fp_ + size_);
    uint8_t* start =
        std::search(data_magic + magic.size(), search_end, base64_sign.begin(), base64_sign.end()) + base64_sign.size();

    memmove(p_write, p_read, start - p_read);
    p_write += start - p_read;
    p_read = start;

    if (p_read > search_end)
      continue;

    const string quote = "'\")";
    uint8_t* end = fp_ + size_;
    if (!single_mode_) {
      end = std::find_first_of(p_read, fp_ + size_, quote.begin(), quote.end());
      if (end >= fp_ + size_) {
        memmove(p_write, p_read, fp_ + size_ - p_read);
        p_write += fp_ + size_ - p_read;
        break;
      }
    }

    if (is_verbose) {
      cout << string(reinterpret_cast<char*>(data_magic), start + 8 - data_magic) << "... found";
      if (!single_mode_) {
        cout << " at offset 0x" << std::hex << data_magic - fp_ << std::dec;
      }
      cout << endl;
    }
    size_t new_size = Base64(p_read, end - p_read).Leanify(p_read - p_write);
    p_write += new_size;
    p_read = end;
  }
  fp_ -= size_leanified;
  size_ = p_write - fp_;
  return size_;
}
