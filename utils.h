#ifndef UTILS_H
#define UTILS_H


#include <cstddef>
#include <cstdint>


void UTF16toMBS(const wchar_t *u, size_t srclen, char *mbs, size_t dstlen);


#endif