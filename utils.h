#ifndef UTILS_H
#define UTILS_H


#ifdef _WIN32
#include <Windows.h>    // WideCharToMultiByte
#else
#include <iconv.h>      // convert UTF16 to UTF8 on non Windows
#endif


void UTF16toMBS(wchar_t *u, size_t srclen, char *mbs, size_t dstlen);

#endif