#ifndef FORMATS_BMP_H_
#define FORMATS_BMP_H_

#include "../utils.h"
#include "format.h"

// The support for this format is not yet implemented see bmp.cpp for details.

class Bmp : public Format {
 public:
  using Format::Format;

  size_t Leanify(size_t size_leanified = 0) override;

  static const uint8_t header_magic[2];

  PACK(struct BITMAPINFOHEADER {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
  });
};

#endif  // FORMATS_BMP_H_
