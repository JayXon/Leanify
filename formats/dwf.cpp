#include "dwf.h"

#include "zip.h"

// "(DWF V06."
const uint8_t Dwf::header_magic[] = { 0x28, 0x44, 0x57, 0x46, 0x20, 0x56, 0x30, 0x36, 0x2E };

size_t Dwf::Leanify(size_t size_leanified /*= 0*/) {
  const size_t dwf_header_len = 12;
  if (memcmp(fp_ + dwf_header_len, Zip::header_magic, sizeof(Zip::header_magic))) {
    return Format::Leanify(size_leanified);
  }
  memmove(fp_ - size_leanified, fp_, dwf_header_len);
  Zip zip(fp_, size_, dwf_header_len);
  size_ = zip.Leanify(size_leanified - dwf_header_len) + dwf_header_len;
  fp_ -= size_leanified;
  return size_;
}
