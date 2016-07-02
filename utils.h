#ifndef UTILS_H_
#define UTILS_H_

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

extern int depth;
extern bool is_verbose;

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
