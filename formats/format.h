#ifndef FORMATS_FORMAT_H_
#define FORMATS_FORMAT_H_

#include <cstddef>
#include <cstdint>
#include <cstring>  // memmove
#include <vector>

class Format {
 public:
  explicit Format(std::vector<uint8_t>& data) : fp_(data.data()), size_(data.size()) {}
  Format(void* p, size_t s) : fp_(static_cast<uint8_t*>(p)), size_(s) {}

  virtual ~Format() = default;

  // Disallow copy and assign.
  Format(const Format&) = delete;
  Format& operator=(const Format&) = delete;

  virtual size_t Leanify(size_t size_leanified = 0) {
    if (size_leanified) {
      memmove(fp_ - size_leanified, fp_, size_);
      fp_ -= size_leanified;
    }

    return size_;
  }

 protected:
  // pointer to the file content
  uint8_t* fp_;
  // size of the file
  size_t size_;
};

#endif  // FORMATS_FORMAT_H_
