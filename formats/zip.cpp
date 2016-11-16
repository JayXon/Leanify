#include "zip.h"

#include <algorithm>
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

namespace {

PACK(struct LocalHeader {
  uint8_t magic[4];
  uint16_t version_needed;
  uint16_t flag;
  uint16_t compression_method;
  uint16_t last_mod_time;
  uint16_t last_mod_date;
  uint32_t crc32;
  uint32_t compressed_size;
  uint32_t uncompressed_size;
  uint16_t filename_len;
  uint16_t extra_field_len;
});

PACK(struct CDHeader {
  uint8_t magic[4] = { 0x50, 0x4B, 0x01, 0x02 };
  uint16_t version_made_by;
  uint16_t version_needed;
  uint16_t flag;
  uint16_t compression_method;
  uint16_t last_mod_time;
  uint16_t last_mod_date;
  uint32_t crc32;
  uint32_t compressed_size;
  uint32_t uncompressed_size;
  uint16_t filename_len;
  uint16_t extra_field_len;
  uint16_t comment_len;
  uint16_t disk_file_start;
  uint16_t internal_file_attributes;
  uint32_t external_file_attributes;
  uint32_t local_header_offset;
});

PACK(struct EOCD {
  uint8_t magic[4] = { 0x50, 0x4B, 0x05, 0x06 };
  uint16_t disk_num;
  uint16_t disk_cd_start;
  uint16_t num_records;
  uint16_t num_records_total;
  uint32_t cd_size;
  uint32_t cd_offset;
  uint16_t comment_len;
});

}  // namespace

size_t Zip::Leanify(size_t size_leanified /*= 0*/) {
  depth++;

  EOCD eocd;
  uint8_t* p_end = fp_ + size_;
  // smallest possible location of EOCD if there's a 64K comment
  uint8_t* p_searchstart = std::max(fp_, p_end - 65535 - sizeof(eocd.magic));
  uint8_t* p_eocd = std::find_end(p_searchstart, p_end, eocd.magic, eocd.magic + sizeof(eocd.magic));
  if (p_eocd == p_end) {
    cerr << "EOCD not found!" << endl;
    return Format::Leanify(size_leanified);
  }

  if (p_eocd + sizeof(EOCD) > p_end) {
    cerr << "EOF with EOCD!" << endl;
    return Format::Leanify(size_leanified);
  }
  memcpy(&eocd, p_eocd, sizeof(EOCD));

  if (eocd.disk_num != 0 || eocd.disk_cd_start != 0 || eocd.num_records != eocd.num_records_total) {
    cerr << "Neither split nor spanned archives is supported!" << endl;
    return Format::Leanify(size_leanified);
  }
  uint8_t* cd_end = fp_ + eocd.cd_offset + eocd.cd_size;
  if (cd_end > p_eocd) {
    cerr << "Central directory too large!" << endl;
    return Format::Leanify(size_leanified);
  }

  uint8_t* first_local_header =
      std::search(fp_, fp_ + eocd.cd_offset, header_magic, header_magic + sizeof(header_magic));
  // The offset of the first local header, we should keep everything before this offset.
  off_t zip_offset = first_local_header - fp_;
  // The offset that all the offsets in the zip file based on (relative to).
  // Should be 0 by default except when we detected that the input file has a base offset.
  off_t base_offset = 0;

  // Copy cd headers to vector
  vector<CDHeader> cd_headers;
  uint8_t* p_read = fp_ + eocd.cd_offset;
  for (int i = 0; i < eocd.num_records; i++) {
    CDHeader cd_header;
    if (p_read + sizeof(CDHeader) > cd_end) {
      cerr << "Central directory header " << i << " passed end, all remaining headers ignored." << endl;
      break;
    }
    if (memcmp(p_read, cd_header.magic, sizeof(cd_header.magic))) {
      // The offset might be relative to the first local file header instead of the beginning of the file.
      if (i == 0 && cd_end + zip_offset <= p_end &&
          memcmp(p_read + zip_offset, cd_header.magic, sizeof(cd_header.magic)) == 0) {
        // This is indeed the case, set the |base_offset| to |zip_offset| and move pointer forward.
        base_offset = zip_offset;
        p_read += base_offset;
        cd_end += base_offset;
      } else {
        cerr << "Central directory header magic mismatch at offset 0x" << std::hex << p_read - fp_ << std::dec << endl;
        break;
      }
    }
    memcpy(&cd_header, p_read, sizeof(CDHeader));
    p_read += sizeof(CDHeader) + cd_header.filename_len + cd_header.extra_field_len + cd_header.comment_len;

    cd_headers.push_back(cd_header);
  }
  if (p_read != cd_end) {
    cerr << "Warning: Central directory size mismatch!" << endl;
  }

  std::sort(cd_headers.begin(), cd_headers.end(),
            [](const CDHeader& a, const CDHeader& b) { return a.local_header_offset < b.local_header_offset; });

  uint8_t* fp_w = fp_ - size_leanified;
  uint8_t* fp_w_base = fp_w + base_offset;
  memmove(fp_w, fp_, zip_offset);
  uint8_t* p_write = fp_w + zip_offset;
  // Local file header
  for (CDHeader& cd_header : cd_headers) {
    p_read = fp_ + base_offset + cd_header.local_header_offset;

    if (p_read + sizeof(LocalHeader) > p_end || memcmp(p_read, header_magic, sizeof(header_magic))) {
      cerr << "Invalid local header offset: 0x" << std::hex << cd_header.local_header_offset << std::dec << endl;
      continue;
    }
    cd_header.local_header_offset = p_write - fp_w_base;

    LocalHeader* local_header = reinterpret_cast<LocalHeader*>(p_read);

    if (local_header->filename_len != cd_header.filename_len) {
      cerr << "Warning: Filename length mismatch between local file header and central directory!" << endl;
    }

    size_t header_size = sizeof(LocalHeader) + local_header->filename_len;
    if (p_read + header_size > p_end) {
      cerr << "Reached EOF in local header!" << endl;
      break;
    }
    // move header
    memmove(p_write, p_read, header_size);
    local_header = reinterpret_cast<LocalHeader*>(p_write);

    // if Extra field length is not 0, then skip it and set it to 0
    if (local_header->extra_field_len) {
      p_read += local_header->extra_field_len;
      local_header->extra_field_len = 0;
    }

    if (local_header->flag & 8) {
      // set this bit to 0, we don't use data descriptor to save 16 byte
      local_header->flag &= ~8;
      cd_header.flag &= ~8;

      // Use the correct value from central directory
      local_header->crc32 = cd_header.crc32;
      local_header->compressed_size = cd_header.compressed_size;
      local_header->uncompressed_size = cd_header.uncompressed_size;
    }

    string filename(reinterpret_cast<char*>(local_header) + sizeof(LocalHeader), local_header->filename_len);
    // do not output filename if it is a directory
    if ((local_header->compressed_size || local_header->compression_method) && depth <= max_depth)
      PrintFileName(filename);

    if (p_read + local_header->compressed_size > p_end) {
      cerr << "Compressed size too large: " << local_header->compressed_size << endl;
      break;
    }

    p_read += header_size;
    p_write += header_size;

    // If the method is store, just Leanify the embedded file
    // don't try to change it to deflate, it might break some file.
    if (local_header->compression_method == 0) {
      // method is store
      if (local_header->compressed_size) {
        uint32_t new_size = LeanifyFile(p_read, local_header->compressed_size, p_read - p_write, filename);
        cd_header.crc32 = local_header->crc32 = mz_crc32(0, p_write, new_size);
        cd_header.compressed_size = local_header->compressed_size = new_size;
        cd_header.uncompressed_size = local_header->uncompressed_size = new_size;
        p_write += local_header->compressed_size;
      }
      continue;
    }

    // If unsupported compression method or fast mode or encrypted, just move it.
    if (local_header->compression_method != 8 || is_fast || local_header->flag & 1) {
      memmove(p_write, p_read, local_header->compressed_size);
      p_write += local_header->compressed_size;
      continue;
    }

    // Switch from deflate to store for empty file.
    if (local_header->uncompressed_size == 0) {
      cd_header.compression_method = local_header->compression_method = 0;
      cd_header.compressed_size = local_header->compressed_size = 0;
      continue;
    }

    // decompress
    size_t decompressed_size = 0;
    uint8_t* decompress_buf = static_cast<uint8_t*>(
        tinfl_decompress_mem_to_heap(p_read, local_header->compressed_size, &decompressed_size, 0));

    if (!decompress_buf || decompressed_size != local_header->uncompressed_size ||
        local_header->crc32 != mz_crc32(0, decompress_buf, local_header->uncompressed_size)) {
      cerr << "Decompression failed or CRC32 mismatch, skipping this file." << endl;
      mz_free(decompress_buf);
      memmove(p_write, p_read, local_header->compressed_size);
      p_write += local_header->compressed_size;
      continue;
    }

    // Leanify uncompressed file
    uint32_t new_uncomp_size = LeanifyFile(decompress_buf, decompressed_size, 0, filename);

    // recompress
    uint8_t bp = 0, *compress_buf = nullptr;
    size_t new_comp_size = 0;
    ZopfliDeflate(&zopfli_options_, 2, 1, decompress_buf, new_uncomp_size, &bp, &compress_buf, &new_comp_size);

    // switch to store if deflate makes file larger
    if (new_uncomp_size <= new_comp_size && new_uncomp_size <= local_header->compressed_size) {
      cd_header.compression_method = local_header->compression_method = 0;
      cd_header.crc32 = local_header->crc32 = mz_crc32(0, decompress_buf, new_uncomp_size);
      cd_header.compressed_size = local_header->compressed_size = new_uncomp_size;
      cd_header.uncompressed_size = local_header->uncompressed_size = new_uncomp_size;
      memcpy(p_write, decompress_buf, new_uncomp_size);
    } else if (new_comp_size < local_header->compressed_size) {
      cd_header.crc32 = local_header->crc32 = mz_crc32(0, decompress_buf, new_uncomp_size);
      cd_header.compressed_size = local_header->compressed_size = new_comp_size;
      cd_header.uncompressed_size = local_header->uncompressed_size = new_uncomp_size;
      memcpy(p_write, compress_buf, new_comp_size);
    } else {
      memmove(p_write, p_read, local_header->compressed_size);
    }
    p_write += local_header->compressed_size;

    mz_free(decompress_buf);
    delete[] compress_buf;
  }

  // central directory offset
  eocd.cd_offset = p_write - fp_w_base;
  for (CDHeader& cd_header : cd_headers) {
    cd_header.extra_field_len = cd_header.comment_len = 0;

    memcpy(p_write, &cd_header, sizeof(CDHeader));
    p_write += sizeof(CDHeader);
    // Copy the filename from local file header to central directory,
    // the old central directory might have been overwritten already because we sort them.
    memcpy(p_write, fp_w_base + cd_header.local_header_offset + 30, cd_header.filename_len);
    p_write += cd_header.filename_len;
  }

  // Update end of central directory record
  eocd.num_records = eocd.num_records_total = cd_headers.size();
  eocd.cd_size = p_write - fp_w_base - eocd.cd_offset;
  eocd.comment_len = 0;

  memcpy(p_write, &eocd, sizeof(EOCD));

  fp_ -= size_leanified;
  size_ = p_write + sizeof(EOCD) - fp_;
  return size_;
}
