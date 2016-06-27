#include "vcf.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "base64.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

size_t Vcf::Leanify(size_t size_leanified /*= 0*/) {
  uint8_t *p_read = fp_, *p_write = fp_ - size_leanified;

  while (p_read < fp_ + size_) {
    const string magic = "\nPHOTO;";
    uint8_t* photo_magic = std::search(p_read, fp_ + size_, magic.begin(), magic.end()) + 1;

    if (photo_magic >= fp_ + size_) {
      memmove(p_write, p_read, fp_ + size_ - p_read);
      p_write += fp_ + size_ - p_read;
      break;
    }

    uint8_t* line_end = std::find(photo_magic + magic.size(), fp_ + size_, '\n');
    const vector<string> base64_sign = { "ENCODING=BASE64", "ENCODING=b" };

    // Check if current line contains any of the |base64_sign|.
    if (!std::any_of(base64_sign.begin(), base64_sign.end(), [&](const string& s) {
          return std::search(photo_magic + magic.size() - 1, line_end, s.begin(), s.end()) < line_end;
        })) {
      // Not base64 encoded, skipping
      memmove(p_write, p_read, line_end - p_read);
      p_write += line_end - p_read;
      p_read = line_end;
      continue;
    }

    uint8_t* start = std::find(photo_magic + magic.size(), line_end, ':') + 1;

    memmove(p_write, p_read, start - p_read);
    p_write += start - p_read;
    p_read = start;

    if (p_read > line_end)
      continue;

    // Find the first line that doesn't start with a space or tab.
    // https://tools.ietf.org/html/rfc6350#section-3.2
    uint8_t* end = std::find(start, fp_ + size_, '\n');
    while (end + 1 < fp_ + size_ && (end[1] == ' ' || end[1] == '\t')) {
      end = std::find(end + 1, fp_ + size_, '\n');
    }
    if (end >= fp_ + size_) {
      memmove(p_write, p_read, fp_ + size_ - p_read);
      p_write += fp_ + size_ - p_read;
      break;
    }

    // Don't remove this \r
    if (*(end - 1) == '\r')
      end--;

    if (is_verbose) {
      cout << string(reinterpret_cast<char*>(photo_magic), start + 12 - photo_magic) << "... found at offset 0x"
           << std::hex << photo_magic - fp_ << std::dec << endl;
    }
    size_t new_size = Base64(p_read, end - p_read).Leanify(p_read - p_write);
    p_write += new_size;
    p_read = end;
  }
  fp_ -= size_leanified;
  size_ = p_write - fp_;
  return size_;
}
