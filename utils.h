#ifndef UTILS_H
#define UTILS_H


#include <cstddef>
#include <cstdint>
#include <string>

extern int depth;

void UTF16toMBS(const wchar_t *u, size_t srclen, char *mbs, size_t dstlen);

void PrintFileName(const char *name);
void PrintFileName(const std::string name);

#endif