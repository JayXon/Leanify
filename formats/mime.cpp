#include "mime.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

#include "base64.h"

using std::cout;
using std::endl;
using std::string;

size_t Mime::Leanify(size_t size_leanified /*= 0*/) {
  uint8_t *p_read = fp_, *p_write = fp_ - size_leanified;

  while (p_read < fp_ + size_) {
    const string magic = "Content-Transfer-Encoding: base64";
    // Case insensitive search
    uint8_t* encoding_magic = std::search(p_read, fp_ + size_, magic.begin(), magic.end(),
                                          [](char a, char b) { return toupper(a) == toupper(b); });

    if (encoding_magic >= fp_ + size_) {
      memmove(p_write, p_read, fp_ + size_ - p_read);
      p_write += fp_ + size_ - p_read;
      break;
    }

    const string double_crlf = "\r\n\r\n";
    uint8_t* start = std::search(encoding_magic + magic.size(), fp_ + size_, double_crlf.begin(), double_crlf.end()) +
                     double_crlf.size();

    start = std::min(start, fp_ + size_);

    memmove(p_write, p_read, start - p_read);
    p_write += start - p_read;
    p_read = start;

    if (p_read >= fp_ + size_) {
      break;
    }

    const string dash_boundary = "\r\n--";
    uint8_t* end = std::search(start, fp_ + size_, dash_boundary.begin(), dash_boundary.end());
    if (end >= fp_ + size_) {
      memmove(p_write, p_read, fp_ + size_ - p_read);
      p_write += fp_ + size_ - p_read;
      break;
    }

    if (is_verbose) {
      cout << string(reinterpret_cast<char*>(start), 8) << "... found at offset 0x" << std::hex << start - fp_
           << std::dec << endl;
    }
    size_t new_size = Base64(p_read, end - p_read).Leanify(p_read - p_write);
    p_write += new_size;
    p_read = end;
  }
  fp_ -= size_leanified;
  size_ = p_write - fp_;
  return size_;
}
