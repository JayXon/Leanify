#ifndef UTILS_H
#define UTILS_H


#include <cstddef>
#include <cstdint>


void UTF16toMBS(const wchar_t *u, size_t srclen, char *mbs, size_t dstlen);


int Base64Decode(const char *in, size_t in_len, uint8_t *out, size_t *out_len);

int Base64Encode(const void *in, size_t in_len, char *out, size_t out_len);

#endif