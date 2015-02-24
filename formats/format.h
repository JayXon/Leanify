#ifndef FORMAT_H
#define FORMAT_H


#include <cstddef>
#include <cstring>  // memmove


class Format
{
public:
    Format(void *p, size_t s = 0) : fp((char *)p), size(s) {};

    virtual ~Format() {};

    virtual size_t Leanify(size_t size_leanified = 0)
    {
        if (size_leanified)
        {
            memmove(fp - size_leanified, fp, size);
            fp -= size_leanified;
        }

        return size;
    }

protected:
    // pointer to the file content
    char *fp;
    // size of the file
    size_t size;
};


#endif