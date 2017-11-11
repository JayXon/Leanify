#include "base64.h"

#include <cstdint>
#include <iostream>
#include <vector>

#include "../leanify.h"

namespace {

int Base64Decode(const uint8_t* in, size_t in_len, uint8_t* out, size_t* out_len) {
  static const uint8_t d[] = {
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 64, 66, 66, 64, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 64, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 62, 66, 66, 66, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
    66, 66, 66, 65, 66, 66, 66, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    22, 23, 24, 25, 66, 66, 66, 66, 66, 66, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66
  };

  uint32_t buf = 1;
  size_t len = 0;

  for (size_t i = 0; i < in_len; i++) {
    uint8_t c = d[in[i]];

    switch (c) {
      case 66:
        return 2;  // invalid input, return error
      case 65:
        i = in_len;  // pad character, end of data
        // Fall through.
      case 64:
        continue;  // skip whitespace
      default:
        buf = buf << 6 | c;

        // If the buffer is full, split it into bytes
        if (buf & 0x1000000) {
          if ((len += 3) > *out_len)
            return 1;
          *out++ = buf >> 16;
          *out++ = buf >> 8;
          *out++ = buf;
          buf = 1;
        }
    }
  }

  if (buf & 0x40000) {
    if ((len += 2) > *out_len)
      return 1;
    *out++ = buf >> 10;
    *out = buf >> 2;
  } else if (buf & 0x1000) {
    if (++len > *out_len)
      return 1;
    *out = buf >> 4;
  }

  *out_len = len;
  return 0;
}

size_t Base64Encode(const uint8_t* in, size_t in_len, uint8_t* out) {
  static const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t resultIndex = 0;

  // increment over the length of the string, three characters at a time
  for (size_t x = 0; x < in_len; x += 3) {
    // these three 8-bit (ASCII) characters become one 24-bit number
    uint32_t n = in[x] << 16;

    if ((x + 2) < in_len) {
      n += (in[x + 1] << 8) + in[x + 2];
    } else if ((x + 1) < in_len) {
      n += in[x + 1] << 8;
    }

    // if we have one byte available, then its encoding is spread out over two characters
    out[resultIndex++] = base64chars[(n >> 18) & 63];
    out[resultIndex++] = base64chars[(n >> 12) & 63];

    if ((x + 2) < in_len) {
      out[resultIndex++] = base64chars[(n >> 6) & 63];
      out[resultIndex++] = base64chars[n & 63];
    } else {
      if ((x + 1) < in_len) {
        out[resultIndex++] = base64chars[(n >> 6) & 63];
      } else {
        out[resultIndex++] = '=';
      }
      out[resultIndex++] = '=';
    }
  }
  return resultIndex;
}

}  // namespace

size_t Base64::Leanify(size_t size_leanified /*= 0*/) {
  // 4 base64 character contains information of 3 bytes
  size_t binary_len = size_ * 3 / 4;
  std::vector<uint8_t> binary_data(binary_len);

  if (Base64Decode(fp_, size_, binary_data.data(), &binary_len)) {
    std::cerr << "Base64 decode error." << std::endl;
    return Format::Leanify(size_leanified);
  }

  // Leanify embedded file
  binary_len = LeanifyFile(binary_data.data(), binary_len);

  fp_ -= size_leanified;
  // encode back
  size_ = Base64Encode(binary_data.data(), binary_len, fp_);

  return size_;
}
