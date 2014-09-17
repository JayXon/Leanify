#ifndef UTILS_H
#define UTILS_H


#include <cstdint>

#ifdef _WIN32
#include <Windows.h>    // WideCharToMultiByte
#else
#include <cstdio>
#include <iconv.h>      // convert UTF16 to UTF8 on non Windows
#endif


void UTF16toMBS(const wchar_t *u, size_t srclen, char *mbs, size_t dstlen);


int base64decode(const char *in, size_t inLen, unsigned char *out, size_t *outLen);

int base64encode(const void* data_buf, size_t dataLength, char* result, size_t resultSize);

#endif