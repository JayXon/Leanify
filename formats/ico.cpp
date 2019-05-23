#include "ico.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include <zopflipng/lodepng/lodepng.h>

#include "../utils.h"
#include "bmp.h"
#include "png.h"

using std::cerr;
using std::endl;
using std::vector;

const uint8_t Ico::header_magic[] = { 0x00, 0x00, 0x01, 0x00 };

namespace {

PACK(struct IconDirEntry {
  uint8_t bWidth;          // Width, in pixels, of the image
  uint8_t bHeight;         // Height, in pixels, of the image
  uint8_t bColorCount;     // Number of colors in image (0 if >=8bpp)
  uint8_t bReserved;       // Reserved ( must be 0)
  uint16_t wPlanes;        // Color Planes
  uint16_t wBitCount;      // Bits per pixel
  uint32_t dwBytesInRes;   // How many bytes in this resource?
  uint32_t dwImageOffset;  // Where in the file is this image?
});

}  // namespace

size_t Ico::Leanify(size_t size_leanified /*= 0*/) {
  // number of images inside ico file
  const uint16_t num_of_img = *(uint16_t*)(fp_ + 4);

  // corrupt file: no image or file size too small
  if (num_of_img == 0 || 6 + num_of_img * sizeof(IconDirEntry) >= size_) {
    return Format::Leanify(size_leanified);
  }

  // sort the entries by offset just in case it's not sorted before
  IconDirEntry* entry_addr = reinterpret_cast<IconDirEntry*>(fp_ + 6);
  vector<IconDirEntry> entries(entry_addr, entry_addr + num_of_img);
  std::sort(entries.begin(), entries.end(),
            [](const IconDirEntry& a, const IconDirEntry& b) { return a.dwImageOffset < b.dwImageOffset; });

  // check overlaps
  for (size_t i = 1; i < entries.size(); i++) {
    if (entries[i - 1].dwImageOffset + entries[i - 1].dwBytesInRes > entries[i].dwImageOffset) {
      cerr << "Error: Found overlapping icon entries!" << endl;
      return Format::Leanify(size_leanified);
    }
  }

  // is file size enough?
  if (static_cast<uint64_t>(entries.back().dwImageOffset) + entries.back().dwBytesInRes > size_) {
    return Format::Leanify(size_leanified);
  }

  for (size_t i = 0; i < entries.size(); i++) {
    uint32_t old_offset = entries[i].dwImageOffset;
    // write new offset
    if (i != 0) {
      entries[i].dwImageOffset = entries[i - 1].dwImageOffset + entries[i - 1].dwBytesInRes;
    } else {
      entries[i].dwImageOffset = 6 + num_of_img * sizeof(IconDirEntry);
    }

    // Leanify PNG
    if (memcmp(fp_ + old_offset, Png::header_magic, sizeof(Png::header_magic)) == 0) {
      entries[i].dwBytesInRes = Png(fp_ + old_offset, entries[i].dwBytesInRes)
                                    .Leanify(size_leanified + old_offset - entries[i].dwImageOffset);
      continue;
    }

    // Convert 256x256 BMP to PNG if possible
    if (entries[i].bWidth == 0 && entries[i].bHeight == 0) {
      auto dib = reinterpret_cast<Bmp::BITMAPINFOHEADER*>(fp_ + old_offset);
      if (dib->biSize >= 40 && dib->biWidth == 256 && dib->biHeight == 512 &&  // DIB in ICO always has double height
          dib->biPlanes == 1 && dib->biBitCount == 32 &&                       // only support RGBA for now
          dib->biCompression == 0 &&                                           // BI_RGB aka no compression
          dib->biSize + std::max(dib->biSizeImage, 256 * 256 * 4U) <= entries[i].dwBytesInRes &&
          (dib->biSizeImage == 0 || dib->biSizeImage >= 256 * 256 * 4U) && dib->biClrUsed == 0) {
        VerbosePrint("Converting 256x256 BMP to PNG...");
        // BMP stores ARGB in little endian, so it's actually BGRA, convert it to normal RGBA
        // It also stores the pixels upside down for some reason, so reverse it.
        uint8_t* bmp_row = fp_ + old_offset + dib->biSize + 256 * 256 * 4;
        vector<uint8_t> raw(256 * 256 * 4), png;
        // TODO: detect 0RGB and convert it to RGBA using mask
        for (size_t j = 0; j < 256; j++) {
          bmp_row -= 256 * 4;
          for (size_t k = 0; k < 256; k++) {
            raw[(j * 256 + k) * 4 + 0] = bmp_row[k * 4 + 2];
            raw[(j * 256 + k) * 4 + 1] = bmp_row[k * 4 + 1];
            raw[(j * 256 + k) * 4 + 2] = bmp_row[k * 4 + 0];
            raw[(j * 256 + k) * 4 + 3] = bmp_row[k * 4 + 3];
          }
        }
        if (lodepng::encode(png, raw, 256, 256) == 0) {
          // Optimize the new PNG
          size_t png_size = Png(png).Leanify();
          if (png_size < entries[i].dwBytesInRes) {
            entries[i].dwBytesInRes = png_size;
            memcpy(fp_ + entries[i].dwImageOffset - size_leanified, png.data(), png_size);
            continue;
          }
        }
      }
    }
    memmove(fp_ + entries[i].dwImageOffset - size_leanified, fp_ + old_offset, entries[i].dwBytesInRes);
  }

  fp_ -= size_leanified;

  // write headers if moved
  if (size_leanified) {
    memcpy(fp_, header_magic, sizeof(header_magic));
    *(uint16_t*)(fp_ + 4) = num_of_img;
  }
  // write new entries
  memcpy(fp_ + 6, entries.data(), entries.size() * sizeof(IconDirEntry));

  // offset + size of last img
  size_ = entries.back().dwImageOffset + entries.back().dwBytesInRes;
  return size_;
}
