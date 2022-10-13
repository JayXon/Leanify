#include "rdb.h"

#include <cstdint>
#include <cstring>
#include <iostream>

#include "../leanify.h"
#include "../utils.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;

// 531E98204F8542F0
const uint8_t Rdb::header_magic[] = { 0x35, 0x33, 0x31, 0x45, 0x39, 0x38, 0x32, 0x30,
                                      0x34, 0x46, 0x38, 0x35, 0x34, 0x32, 0x46, 0x30 };

size_t Rdb::Leanify(size_t size_leanified /*= 0*/) {
  if (size_ <= 0x20) {
    cerr << "Not a valid RDB file." << endl;
    return Format::Leanify(size_leanified);
  }

  depth++;
  uint8_t* p_read = fp_;
  size_t rdb_size_leanified = 0;

  // total number of files including directory
  uint32_t file_num = *(uint32_t*)(p_read + 0x10);

  uint64_t index_offset = *(uint64_t*)(p_read + 0x14);

  if (index_offset > size_) {
    cerr << "index offset out of range: " << index_offset << endl;
    return Format::Leanify(size_leanified);
  }

  uint64_t content_offset = index_offset + *(uint64_t*)(p_read + 0x1C);

  if (content_offset < index_offset || content_offset > size_) {
    cerr << "content offset out of range: " << content_offset << endl;
    return Format::Leanify(size_leanified);
  }

  // move header and indexes
  fp_ -= size_leanified;
  memmove(fp_, p_read, (size_t)content_offset);

  uint8_t* p_index = fp_ + index_offset;
  p_read += content_offset;

  for (uint32_t i = 0; i < file_num; i++) {
    // index
    wchar_t* file_name = (wchar_t*)p_index;

    // note that on Linux wchar_t is 4 bytes instead of 2
    // so I can't use wcslen
    // p_index += (wcslen(file_name) + 1) * 2;
    while (p_index + 2 < fp_ + content_offset && *(uint16_t*)p_index) {
      p_index += 2;
    }
    p_index += 2;

    size_t remaining_size = fp_ + size_leanified + size_ - p_read;
    if (p_index + 8 > fp_ + content_offset) {
      cerr << "index is overlapping with content" << endl;
      memmove(p_read - rdb_size_leanified - size_leanified, p_read, remaining_size);
      p_read += remaining_size;
      break;
    }

    uint64_t file_size = *(uint64_t*)(p_index + 8);
    if (file_size > static_cast<uint64_t>(remaining_size)) {
      cerr << "file size out of range: " << file_size << endl;
      memmove(p_read - rdb_size_leanified - size_leanified, p_read, remaining_size);
      p_read += remaining_size;
      break;
    }

    *(uint64_t*)p_index -= rdb_size_leanified;

    // skip directories
    if (!file_size) {
      p_index += 16;
      continue;
    }

    if (depth <= max_depth) {
      // output filename
      char mbs[256] = { 0 };
      UTF16toMBS(file_name, p_index - reinterpret_cast<uint8_t*>(file_name), mbs, sizeof(mbs));
      PrintFileName(mbs);

      // Leanify inner file
      size_t new_size = LeanifyFile(p_read, (size_t)file_size, rdb_size_leanified + size_leanified, mbs);
      if (new_size != file_size) {
        // update the size in index
        *(uint64_t*)(p_index + 8) = new_size;
        rdb_size_leanified += (size_t)file_size - new_size;
      }
    } else {
      memmove(p_read - rdb_size_leanified - size_leanified, p_read, (size_t)file_size);
    }

    p_read += file_size;

    p_index += 16;
  }
  size_ = p_read - fp_ - size_leanified - rdb_size_leanified;
  depth--;
  return size_;
}