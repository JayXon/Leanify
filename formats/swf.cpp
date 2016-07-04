#include "swf.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>

#include <LZMA/LzmaLib.h>
#include <miniz/miniz.h>

#include "../leanify.h"
#include "../utils.h"

using std::cerr;
using std::endl;

const uint8_t Swf::header_magic[] = { 'F', 'W', 'S' };
const uint8_t Swf::header_magic_deflate[] = { 'C', 'W', 'S' };
const uint8_t Swf::header_magic_lzma[] = { 'Z', 'W', 'S' };

namespace {

void UpdateTagLength(uint8_t* tag_content, size_t header_length, size_t new_length) {
  if (header_length == 6)
    *(uint32_t*)(tag_content - 4) = new_length;
  else
    *(tag_content - 2) += (new_length & 0x3F) - (*(tag_content - 2) & 0x3F);
}

size_t GetRECTSize(uint8_t* rect) {
  // The first 5 bits.
  uint8_t nbits = *rect >> 3;
  // Xmin, Xmax, Ymin, Ymax takes nbits each, round the sum to bytes.
  return (5 + nbits * 4 + 7) / 8;
}

}  // namespace

size_t Swf::Leanify(size_t size_leanified /*= 0*/) {
  if (is_fast && *fp_ != 'F')
    return Format::Leanify(size_leanified);

  uint8_t* in_buffer = fp_ + 8;
  uint32_t in_len = *(uint32_t*)(fp_ + 4) - 8;

  // if SWF is compressed, decompress it first
  if (*fp_ == 'C') {
    // deflate
    VerbosePrint("SWF is compressed with deflate.");
    size_t s = 0;
    in_buffer =
        static_cast<uint8_t*>(tinfl_decompress_mem_to_heap(in_buffer, size_ - 8, &s, TINFL_FLAG_PARSE_ZLIB_HEADER));
    if (!in_buffer || s != in_len) {
      cerr << "SWF file corrupted!" << endl;
      mz_free(in_buffer);
      return Format::Leanify(size_leanified);
    }
  } else if (*fp_ == 'Z') {
    // LZMA
    VerbosePrint("SWF is compressed with LZMA.");
    // | 4 bytes         | 4 bytes   | 4 bytes       | 5 bytes    | n bytes   | 6 bytes         |
    // | 'ZWS' + version | scriptLen | compressedLen | LZMA props | LZMA data | LZMA end marker |
    uint8_t* dst_buffer = new uint8_t[in_len];
    size_t s = in_len, len = size_ - 12 - LZMA_PROPS_SIZE;
    // check compressed length
    if (*(uint32_t*)in_buffer != len ||
        LzmaUncompress(dst_buffer, &s, in_buffer + 4 + LZMA_PROPS_SIZE, &len, in_buffer + 4, LZMA_PROPS_SIZE) ||
        s != in_len) {
      cerr << "SWF file corrupted!" << endl;
      delete[] dst_buffer;
      return Format::Leanify(size_leanified);
    }
    in_buffer = dst_buffer;
  } else {
    VerbosePrint("SWF is not compressed.");
  }

  // parsing SWF tags
  uint8_t* p = in_buffer + GetRECTSize(in_buffer);  // skip FrameSize which is a RECT
  p += 4;                                           // skip FrameRate(2 Byte) + FrameCount(2 Byte) = 4 Byte
  size_t tag_size_leanified = 0;
  do {
    uint16_t tag_type = *(uint16_t*)p >> 6;
    uint32_t tag_length = *p & 0x3F;
    size_t tag_header_length = 2;
    if (tag_length == 0x3F) {
      tag_length = *(uint32_t*)(p + 2);
      tag_header_length += 4;
    }

    memmove(p - tag_size_leanified, p, tag_header_length);
    p += tag_header_length;

    switch (tag_type) {
      // DefineBitsLossless
      case 20:
      // DefineBitsLossless2
      case 36: {
        size_t header_size = 7 + (p[3] == 3);
        VerbosePrint("DefineBitsLossless tag found.");
        memmove(p - tag_size_leanified, p, header_size);

        // recompress Zlib bitmap data
        size_t new_data_size = ZlibRecompress(p + header_size, tag_length - header_size, tag_size_leanified);

        UpdateTagLength(p - tag_size_leanified, tag_header_length, header_size + new_data_size);
        tag_size_leanified += tag_length - header_size - new_data_size;
        break;
      }
      // DefineBitsJPEG2
      case 21: {
        VerbosePrint("DefineBitsJPEG2 tag found.");
        // copy id
        *(uint16_t*)(p - tag_size_leanified) = *(uint16_t*)p;

        // Leanify embedded image
        size_t new_size = LeanifyFile(p + 2, tag_length - 2, tag_size_leanified);

        UpdateTagLength(p - tag_size_leanified, tag_header_length, 2 + new_size);
        tag_size_leanified += tag_length - 2 - new_size;
        break;
      }
      // DefineBitsJPEG3
      case 35:
      // DefineBitsJPEG4
      case 90: {
        // copy id
        *(uint16_t*)(p - tag_size_leanified) = *(uint16_t*)p;

        uint32_t img_size = *(uint32_t*)(p + 2);
        size_t header_size = tag_type == 90 ? 8 : 6;

        VerbosePrint("DefineBitsJPEG", header_size / 2, " tag found.");
        // Leanify embedded image
        size_t new_img_size = LeanifyFile(p + header_size, img_size, tag_size_leanified);
        *(uint32_t*)(p + 2 - tag_size_leanified) = new_img_size;

        // recompress alpha data
        size_t new_alpha_data_size = ZlibRecompress(p + header_size + img_size, tag_length - img_size - header_size,
                                                    tag_size_leanified + img_size - new_img_size);

        size_t new_tag_size = new_img_size + new_alpha_data_size + header_size;
        UpdateTagLength(p - tag_size_leanified, tag_header_length, new_tag_size);
        tag_size_leanified += tag_length - new_tag_size;
        break;
      }
      // Metadata
      case 77:
        VerbosePrint("Metadata removed.");
        tag_size_leanified += tag_length + tag_header_length;
        break;
      // FileAttributes
      case 69:
        *p &= ~(1 << 4);  // set HasMetadata bit to 0
      default:
        memmove(p - tag_size_leanified, p, tag_length);
    }
    p += tag_length;
  } while (p < in_buffer + in_len);

  in_len -= tag_size_leanified;

  if (is_fast) {
    // write header
    fp_ -= size_leanified;
    memmove(fp_, fp_ + size_leanified, 4);

    // decompressed size (including header)
    *(uint32_t*)(fp_ + 4) = size_ = in_len + 8;

    memmove(fp_ + 8, fp_ + 8 + size_leanified, in_len);
    return size_;
  }

  // compress with LZMA
  size_t s = in_len + in_len / 4, props = LZMA_PROPS_SIZE;
  uint8_t* dst = new uint8_t[s + LZMA_PROPS_SIZE];
  // have to set writeEndMark to true
  if (LzmaCompress(dst + LZMA_PROPS_SIZE, &s, in_buffer, in_len, dst, &props, std::min(9, iterations), 1 << 24, -1, -1,
                   -1, 128, -1)) {
    cerr << "LZMA compression failed." << endl;
    s = size_;
  }

  // free decompressed data
  if (*fp_ == 'C')
    mz_free(in_buffer);
  else if (*fp_ == 'Z')
    delete[] in_buffer;

  fp_ -= size_leanified;

  s += LZMA_PROPS_SIZE;
  if (s + 12 < size_) {
    size_ = s + 12;

    // write header
    memcpy(fp_, header_magic_lzma, sizeof(header_magic_lzma));

    // write SWF version, at least 13 to support LZMA
    fp_[3] = std::max(static_cast<uint8_t>(13), fp_[3 + size_leanified]);

    // decompressed size (including header)
    *(uint32_t*)(fp_ + 4) = in_len + 8;

    // compressed size: LZMA data + end mark
    *(uint32_t*)(fp_ + 8) = s - LZMA_PROPS_SIZE;

    memcpy(fp_ + 12, dst, s);
  } else {
    memmove(fp_, fp_ + size_leanified, size_);
  }

  delete[] dst;

  return size_;
}
