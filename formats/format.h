#ifndef FORMAT_H
#define FORMAT_H


#include <cstring>  // memmove


class Format
{
public:
    Format(void *p, size_t s = 0) : fp((char *)p), size(s) {};


    size_t Leanify(size_t size_leanified = 0)
    {
        if (size_leanified)
        {
            memmove(fp - size_leanified, fp, size);
            fp -= size_leanified;
        }

        return size;
    }

protected:
    char *fp;
    size_t size;
};


#endif