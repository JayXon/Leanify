#include "utils.h"


void UTF16toMBS(wchar_t *u, size_t srclen, char *mbs, size_t dstlen)
{
#ifdef _WIN32
    WideCharToMultiByte(CP_ACP, 0, u, srclen / 2, mbs, dstlen, nullptr, nullptr);
#else
    iconv_t conv = iconv_open("UTF-8", "UTF-16");
    iconv(conv, (char **)&u, &srclen, &mbs, &dstlen);
    iconv_close(conv);
#endif // _WIN32
}
