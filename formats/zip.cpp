#include "zip.h"

#include <algorithm>  // std::search
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <miniz/miniz.h>

#include "../leanify.h"
#include "../utils.h"

using std::cerr;
using std::endl;
using std::string;
using std::vector;

const uint8_t Zip::header_magic[] = { 0x50, 0x4B, 0x03, 0x04 };

size_t Zip::Leanify(size_t size_leanified /*= 0*/) {
  depth++;
  uint8_t* fp_r = fp_;
  uint8_t* p_read;
  uint8_t* p_end = fp_ + size_;
  uint8_t* fp_w = fp_ - size_leanified;
  uint8_t* p_write = fp_w;

  //1. find EOCD
  // End of central directory record
  const uint8_t eocd_header_magic[] = { 0x50, 0x4B, 0x05, 0x06 };
  uint8_t* p_searchstart = p_end - 65535 - 22;
  if (p_searchstart < fp_r) p_searchstart = fp_r;
  uint8_t* p_eocd = std::find_end(p_searchstart, p_end, eocd_header_magic, eocd_header_magic + sizeof(eocd_header_magic));
  if (p_eocd == p_end) {
    cerr << "EOCD not found!" << endl;
    // abort
    return Format::Leanify(size_leanified);
  }

  //2. find CD with EOCD
  p_read = p_eocd;
  if (p_read + 22 > p_end) {
    cerr << "EOF with EOCD!" << endl;
    // abort
    return Format::Leanify(size_leanified);
  }
  uint16_t cd_parts = *(uint16_t*)(p_read + 4);
  uint16_t cd_parts_total = *(uint16_t*)(p_read + 6);
  uint16_t cd_count = *(uint16_t*)(p_read + 8);
  uint16_t cd_count_total = *(uint16_t*)(p_read + 10);
  uint32_t cd_size = *(uint32_t*)(p_read + 12);
  uint8_t* cd_off = fp_r + *(uint32_t*)(p_read + 16);
  uint16_t cd_comment_len = *(uint16_t*)(p_read + 20);

  if (cd_parts > 0 || cd_parts_total > 0 ||
    cd_count != cd_count_total) {
    cerr << "Neither split split nor spanned archives is supported!" << endl;
    // abort
    return Format::Leanify(size_leanified);
  }
  if (cd_off + cd_size > p_end) {
    cerr << "EOF with central directory!" << endl;
    // ignore
  }

  //3. find all local file headers with CD
  vector<uint32_t> local_header_offsets_r;
  p_read = cd_off;
  const uint8_t cd_header_magic[] = { 0x50, 0x4B, 0x01, 0x02 };
  for (int entity = 0; entity < cd_count; ++entity) {
    if (p_read + 46 > p_end) {
      cerr << "EOF with central directory!" << endl;
      // ignore
      break;
    }
    if (memcmp(cd_off, cd_header_magic, sizeof(cd_header_magic))) {
      cerr << "Central directory header magic mismatch!" << endl;
      // ignore
      break;
    }
    local_header_offsets_r.push_back(*(uint32_t*)(p_read + 42));
    p_read += 46 + *(uint16_t*)(p_read + 28) + *(uint16_t*)(p_read + 30) + *(uint16_t*)(p_read + 32);
  }
  if (p_read - cd_off != cd_size) {
    cerr << "Central directory size mismatch!" << endl;
    //ignore
  }

  vector<uint32_t> local_header_offsets_w;
  // TODO: check EOF using size_
  // Local file header
  for (uint32_t local_header_offset : local_header_offsets_r) {
    p_read = fp_r + local_header_offset;
    local_header_offsets_w.push_back(p_write - fp_w);

    uint16_t filename_length = *(uint16_t*)(p_read + 26);

    size_t header_size = 30 + filename_length;
    // move header
    memmove(p_write, p_read, header_size);

    // if Extra field length is not 0, then skip it and set it to 0
    if (*(uint16_t*)(p_write + 28)) {
      p_read += *(uint16_t*)(p_write + 28);
      *(uint16_t*)(p_write + 28) = 0;
    }

    uint32_t* crc = (uint32_t*)(p_write + 14);
    uint32_t* compressed_size = crc + 1;
    uint32_t* uncompressed_size = compressed_size + 1;

    uint32_t orig_comp_size = *compressed_size;

    uint16_t flag = *(uint16_t*)(p_write + 6);
    uint16_t* compression_method = (uint16_t*)(p_write + 8);

    string filename(reinterpret_cast<char*>(p_write) + 30, filename_length);
    // do not output filename if it is a directory
    if ((orig_comp_size || *compression_method || flag & 8) && depth <= max_depth)
      PrintFileName(filename);

    // From Wikipedia:
    // If bit 3 (0x08) of the general-purpose flags field is set,
    // then the CRC-32 and file sizes are not known when the header is written.
    // The fields in the local header are filled with zero,
    // and the CRC-32 and size are appended in a 12-byte structure
    // (optionally preceded by a 4-byte signature) immediately after the compressed data
    if (flag & 8) {
      // set this bit to 0
      *(uint16_t*)(p_write + 6) &= ~8;

      // data descriptor signature
      const uint8_t dd_sign[] = { 0x50, 0x4B, 0x07, 0x08 };
      // search for signature
      uint8_t* dd = p_read + header_size;
      do {
        dd = std::search(dd + 1, p_end, dd_sign, dd_sign + 4);
        if (dd == p_end) {
          cerr << "data descriptor signature not found!" << endl;
          // abort
          // zip does not have 4-byte signature preceded
          return Format::Leanify(size_leanified);
        }
      } while (*(uint32_t*)(dd + 8) != dd - p_read - header_size);

      *crc = *(uint32_t*)(dd + 4);
      *compressed_size = orig_comp_size = *(uint32_t*)(dd + 8);
      *uncompressed_size = *(uint32_t*)(dd + 12);
    }

    // if compression method is not deflate or fast mode
    // then only Leanify embedded file if the method is store
    // otherwise just memmove the compressed part
    if (*compression_method != 8 || (flag & 1) || is_fast) {
      if (*compression_method == 0 && depth <= max_depth && !(flag & 1)) {
        // method is store
        if (orig_comp_size) {
          uint32_t new_size = LeanifyFile(p_read + header_size, orig_comp_size, p_read - p_write, filename);
          p_read += header_size + orig_comp_size;
          *compressed_size = *uncompressed_size = new_size;
          *crc = mz_crc32(0, p_write + header_size, new_size);
        } else {
          p_read += header_size;
        }
      } else {
        // unsupported compression method or encrypted, move it
        memmove(p_write + header_size, p_read + header_size, orig_comp_size);
        p_read += header_size + orig_comp_size;
      }
      p_write += header_size + *compressed_size;

    } else {
      // the method is deflate, uncompress it and recompress with zopfli

      p_read += header_size;
      p_write += header_size;

      if (*uncompressed_size) {
        // uncompress
        size_t s = 0;
        uint8_t* buffer = static_cast<uint8_t*>(tinfl_decompress_mem_to_heap(p_read, orig_comp_size, &s, 0));

        if (!buffer || s != *uncompressed_size || *crc != mz_crc32(0, buffer, *uncompressed_size)) {
          cerr << "ZIP file corrupted!" << endl;
          mz_free(buffer);
          memmove(p_write, p_read, orig_comp_size);
          p_read += orig_comp_size;
          p_write += orig_comp_size;
          continue;
        }

        // Leanify uncompressed file
        uint32_t new_uncomp_size = LeanifyFile(buffer, s, 0, filename);

        // recompress
        uint8_t bp = 0, *out = nullptr;
        size_t new_comp_size = 0;
        ZopfliDeflate(&zopfli_options_, 2, 1, buffer, new_uncomp_size, &bp, &out, &new_comp_size);

        // switch to store if deflate makes file larger
        if (new_uncomp_size <= new_comp_size && new_uncomp_size <= orig_comp_size) {
          *compression_method = 0;
          *crc = mz_crc32(0, buffer, new_uncomp_size);
          *compressed_size = new_uncomp_size;
          *uncompressed_size = new_uncomp_size;
          memcpy(p_write, buffer, new_uncomp_size);
          p_write += new_uncomp_size;
        } else if (new_comp_size < orig_comp_size) {
          *crc = mz_crc32(0, buffer, new_uncomp_size);
          *compressed_size = new_comp_size;
          *uncompressed_size = new_uncomp_size;
          memcpy(p_write, out, new_comp_size);
          p_write += new_comp_size;
        } else {
          memmove(p_write, p_read, orig_comp_size);
          p_write += orig_comp_size;
        }
        p_read += orig_comp_size;

        mz_free(buffer);
        delete[] out;
      } else {
        *compression_method = 0;
        *compressed_size = 0;
        p_read += orig_comp_size;
      }
    }

    // we don't use data descriptor, so that can save more bytes (16 per file)
    if (flag & 8)
      p_read += 16;
  }

  uint8_t* central_directory = p_write;
  p_read = cd_off;
  // TODO: check EOF using size_
  for (uint32_t local_header_offset : local_header_offsets_w) {
    if (memcmp(p_read, cd_header_magic, sizeof(cd_header_magic))) {
      //Already checked before
      cerr << "Central directory header magic mismatch!" << endl;
      break;
    }
    if (p_read + 46 > p_end) {
      //Already checked before
      cerr << "EOF with central directory!" << endl;
      break;
    }
    int header_size = 46 + *(uint16_t*)(p_read + 28);
    // move header
    memmove(p_write, p_read, header_size);

    // set bit 3 of General purpose bit flag to 0
    *(uint16_t*)(p_write + 8) &= ~8;

    // if Extra field length is not 0, then skip it and set it to 0
    if (*(uint16_t*)(p_write + 30)) {
      p_read += *(uint16_t*)(p_write + 30);
      *(uint16_t*)(p_write + 30) = 0;
    }

    // if File comment length is not 0, then skip it and set it to 0
    if (*(uint16_t*)(p_write + 32)) {
      p_read += *(uint16_t*)(p_write + 32);
      *(uint16_t*)(p_write + 32) = 0;
    }

    uint8_t* local_header = fp_w + local_header_offset;

    // copy new CRC-32, Compressed size, Uncompressed size_
    // from Local file header to Central directory file header
    memcpy(p_write + 16, local_header + 14, 12);

    // update compression method
    *(uint16_t*)(p_write + 10) = *(uint16_t*)(local_header + 8);

    // new Local file header offset
    *(uint32_t*)(p_write + 42) = local_header_offset + out_embed_off_;

    p_read += header_size;
    p_write += header_size;
  }

  // End of central directory record
  p_read = p_eocd;

  memmove(p_write, p_read, 12);
  // Number of central directory records on this disk, Total number of central directory records
  *(uint16_t*)(p_write + 8) = *(uint16_t*)(p_write + 10) = local_header_offsets_w.size();
  // central directory size
  *(uint32_t*)(p_write + 12) = p_write - central_directory;
  // central directory offset
  *(uint32_t*)(p_write + 16) = central_directory - fp_w + out_embed_off_;
  // set comment length to 0
  *(uint16_t*)(p_write + 20) = 0;

  // 22 is the length of EOCD
  size_ = p_write + 22 - fp_w;
  fp_ = fp_w;
  return size_;
}
