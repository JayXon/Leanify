#ifndef UTILS_H_
#define UTILS_H_

#include <cstddef>
#include <cstdint>
#include <string>

extern int depth;

void UTF16toMBS(const wchar_t* u, size_t srclen, char* mbs, size_t dstlen);

void PrintFileName(const std::string& name);

#endif  // UTILS_H_
