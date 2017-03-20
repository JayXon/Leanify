#include "png.h"

#include <cstdint>
#include <cstring>
#include <iostream>

#include <zopflipng/lodepng/lodepng.h>
#include <zopflipng/zopflipng_lib.h>

#include "../leanify.h"
#include "../utils.h"

using std::cerr;
using std::cout;
using std::endl;
using std::vector;

const uint8_t Png::header_magic[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
bool Png::keep_icc_profile_ = false;

size_t Png::Leanify(size_t size_leanified /*= 0*/) {
  // header
  uint8_t* p_read = fp_;
  uint8_t* p_write = p_read - size_leanified;

  memmove(p_write, p_read, sizeof(header_magic));

  p_read += sizeof(header_magic);
  p_write += sizeof(header_magic);

  // chunk
  uint32_t chunk_type;

  uint8_t* idat_addr = nullptr;

  do {
    // read chunk length
    // use bswap to convert Big-Endian to Little-Endian
    // 12 = length: 4 + type: 4 + crc: 4
    uint32_t chunk_length = BSWAP32(*(uint32_t*)p_read) + 12;

    // detect truncated file
    if (p_read + chunk_length > fp_ + size_) {
      memmove(p_write, p_read, fp_ + size_ - p_read);
      cerr << "PNG file corrupted!" << endl;
      fp_ -= size_leanified;
      size_ -= p_read - p_write - size_leanified;
      return size_;
    }

    // read chunk type
    chunk_type = *(uint32_t*)(p_read + 4);

    if (chunk_type == 0x54414449) {
      // save IDAT chunk address
      idat_addr = p_write;
    }

    bool should_remove = [&]() {
      // Check the case of first letter, keep all critical chunks.
      if ((chunk_type & 0x20) == 0)
        return false;

      // Remove all ancillary chunks except the following.
      switch (chunk_type) {
        case 0x4C546361:  // acTL     APNG
        case 0x4C546366:  // fcTL     APNG
        case 0x54416466:  // fdAT     APNG    TODO: use Zopfli to recompress fdAT
        case 0x6354706E:  // npTc     Android 9Patch images (*.9.png)
          return false;

        case 0x534E5274:  // tRNS     transparent
                          // tRNS must be before IDAT according to PNG spec
          return idat_addr != nullptr;
        case 0x50434369:  // iCCP     ICC profile
          return !keep_icc_profile_;
        default:
          return true;
      }
    }();
    if (should_remove) {
      if (is_verbose) {
        // chunk name
        for (int i = 4; i < 8; i++)
          cout << static_cast<char>(p_read[i]);

        cout << " chunk removed, " << chunk_length << " bytes." << endl;
      }
      // remove this chunk
      p_read += chunk_length;
      continue;
    }

    // move this chunk
    memmove(p_write, p_read, chunk_length);

    // skip whole chunk
    p_write += chunk_length;
    p_read += chunk_length;

  } while (chunk_type != 0x444E4549);  // IEND

  fp_ -= size_leanified;
  size_ = p_write - fp_;

  size_t resultpng_size = 0;

  {
    ZopfliPNGOptions zopflipng_options;
    zopflipng_options.use_zopfli = !is_fast;
    zopflipng_options.lossy_transparent = true;
    // see the switch above for information about these chunks
    zopflipng_options.keepchunks = { "acTL", "fcTL", "fdAT", "npTc" };
    if (keep_icc_profile_)
      zopflipng_options.keepchunks.push_back("iCCP");
    zopflipng_options.num_iterations = iterations;
    zopflipng_options.num_iterations_large = iterations;

    const vector<uint8_t> origpng(fp_, fp_ + size_);
    vector<uint8_t> resultpng;

    if (!ZopfliPNGOptimize(origpng, zopflipng_options, is_verbose, &resultpng)) {
      // only use the result PNG if it is smaller
      // sometimes the original PNG is already highly optimized
      // then maybe ZopfliPNG will produce bigger file
      resultpng_size = resultpng.size();
      if (resultpng.size() < size_) {
        size_ = resultpng_size;
        memcpy(fp_, resultpng.data(), resultpng_size);
        return size_;
      }
    } else {
      cerr << "ZopfliPNG failed!" << endl;
    }
  }

  // If the result is exactly the same size as before, chances are it was optimized by ZopfliPNG.
  // So there's no need to try Zopfli only, but if it's very small file, that might be a coincidence,
  // even if it's not, it won't take much time.
  if (idat_addr && (resultpng_size != size_ || size_ < 32768)) {
    // sometimes the strategy chosen by ZopfliPNG is worse than original
    // then try to recompress IDAT chunk using only Zopfli
    VerbosePrint("ZopfliPNG failed to reduce size, try Zopfli only.");
    uint32_t idat_length = BSWAP32(*(uint32_t*)idat_addr);
    uint32_t new_idat_length = ZlibRecompress(idat_addr + 8, idat_length);
    if (idat_length != new_idat_length) {
      *(uint32_t*)idat_addr = BSWAP32(new_idat_length);
      *(uint32_t*)(idat_addr + new_idat_length + 8) = BSWAP32(lodepng_crc32(idat_addr + 4, new_idat_length + 4));
      uint8_t* idat_end = idat_addr + idat_length + 12;
      memmove(idat_addr + new_idat_length + 12, idat_end, fp_ + size_ - idat_end);
      size_ -= idat_length - new_idat_length;
    }
  }

  return size_;
}
