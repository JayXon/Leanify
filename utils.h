#ifndef UTILS_H_
#define UTILS_H_

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

extern int depth;
extern bool is_verbose;

#ifdef _MSC_VER
#define BSWAP32(x) _byteswap_ulong(x)
#elif defined __GNUC__
#define BSWAP32(x) __builtin_bswap32(x)
#else
#define BSWAP32(x) _bswap(x)
#endif

#ifdef _MSC_VER
#define PACK(...) __pragma(pack(push, 1)) __VA_ARGS__ __pragma(pack(pop))
#elif defined __GNUC__
#define PACK(...) __VA_ARGS__ __attribute__((__packed__))
#endif

void UTF16toMBS(const wchar_t* u, size_t srclen, char* mbs, size_t dstlen);

void PrintFileName(const std::string& name);

std::string ShrinkSpace(const char* value);

template <typename T>
void VerbosePrint(const T& t) {
  if (!is_verbose)
    return;

  std::cout << t << std::endl;
}

template <typename T, typename... Args>
void VerbosePrint(const T& t, const Args&... args) {
  if (!is_verbose)
    return;

  std::cout << t;
  VerbosePrint(args...);
}

// Divide |x| by |y| and round up to the nearest integer.
constexpr uint64_t DivRoundUp(uint64_t x, uint64_t y) {
  return (x + y - 1) / y;
}
// Round |x| up to be a multiple of |y|.
constexpr uint64_t RoundUp(uint64_t x, uint64_t y) {
  return DivRoundUp(x, y) * y;
}
#endif  // UTILS_H_
