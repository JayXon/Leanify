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

void UTF16toMBS(const wchar_t* u, size_t srclen, char* mbs, size_t dstlen);

void PrintFileName(const std::string& name);

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

#endif  // UTILS_H_
