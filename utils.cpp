#include "utils.h"

#ifdef _WIN32
#include <Windows.h>    // WideCharToMultiByte
#else
#include <cstdio>
#include <iconv.h>      // convert UTF16 to UTF8 on non Windows
#endif


// convert UNICODE string aka UTF16 string
// to multi byte string aka UTF8 string
void UTF16toMBS(const wchar_t *u, size_t srclen, char *mbs, size_t dstlen)
{
#ifdef _WIN32
    WideCharToMultiByte(CP_ACP, 0, u, srclen / 2, mbs, dstlen, nullptr, nullptr);
#else
    iconv_t conv = iconv_open("UTF-8", "UTF-16");
    if (iconv(conv, (char **)&u, &srclen, &mbs, &dstlen) == (size_t) - 1)
    {
        perror("iconv");
    }
    iconv_close(conv);
#endif // _WIN32
}



int Base64Decode(const char *in, size_t in_len, uint8_t *out, size_t *out_len)
{
    static const uint8_t d[] =
    {
        66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 64, 66, 66, 64, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
        66, 66, 66, 66, 66, 66, 66, 64, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 62, 66, 66, 66, 63, 52, 53,
        54, 55, 56, 57, 58, 59, 60, 61, 66, 66, 66, 65, 66, 66, 66, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 66, 66, 66, 66, 66, 66, 26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 66, 66,
        66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
        66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
        66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
        66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
        66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
        66, 66, 66, 66, 66, 66
    };

    const char *end = in + in_len;
    size_t buf = 1, len = 0;

    while (in < end)
    {
        uint8_t c = d[(uint8_t) * in++];

        switch (c)
        {
        case 66:
            return 2;   /* invalid input, return error */
        case 65:
            in = end;   /* pad character, end of data */
        case 64:
            continue;   /* skip whitespace */
        default:
            buf = buf << 6 | c;

            /* If the buffer is full, split it into bytes */
            if (buf & 0x1000000)
            {
                if ((len += 3) > *out_len) return 1; /* buffer overflow */
                *out++ = buf >> 16;
                *out++ = buf >> 8;
                *out++ = buf;
                buf = 1;
            }
        }
    }

    if (buf & 0x40000)
    {
        if ((len += 2) > *out_len) return 1; /* buffer overflow */
        *out++ = buf >> 10;
        *out++ = buf >> 2;
    }
    else if (buf & 0x1000)
    {
        if (++len > *out_len) return 1; /* buffer overflow */
        *out++ = buf >> 4;
    }

    *out_len = len; /* modify to reflect the actual output size */
    return 0;
}

int Base64Encode(const void *in, size_t in_len, char *out, size_t out_len)
{
    static const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const uint8_t *data = (const uint8_t *)in;
    size_t resultIndex = 0;

    /* increment over the length of the string, three characters at a time */
    for (size_t x = 0; x < in_len; x += 3)
    {
        if (resultIndex + 4 >= out_len) return 1;   /* indicate failure: buffer too small */

        /* these three 8-bit (ASCII) characters become one 24-bit number */
        uint32_t n = data[x] << 16;

        if ((x + 2) < in_len)
        {
            n += (data[x + 1] << 8) + data[x + 2];
        }
        else if ((x + 1) < in_len)
        {
            n += data[x + 1] << 8;
        }

        /*
        * if we have one byte available, then its encoding is spread
        * out over two characters
        */
        out[resultIndex++] = base64chars[(uint8_t)(n >> 18) & 63];
        out[resultIndex++] = base64chars[(uint8_t)(n >> 12) & 63];

        if ((x + 2) < in_len)
        {
            out[resultIndex++] = base64chars[(uint8_t)(n >> 6) & 63];
            out[resultIndex++] = base64chars[(uint8_t)n & 63];
        }
        else
        {
            if ((x + 1) < in_len)
            {
                out[resultIndex++] = base64chars[(uint8_t)(n >> 6) & 63];
            }
            else
            {
                out[resultIndex++] = '=';
            }
            out[resultIndex++] = '=';
        }

    }

    if (resultIndex >= out_len) return 1;   /* indicate failure: buffer too small */
    out[resultIndex] = 0;
    return 0;   /* indicate success */
}