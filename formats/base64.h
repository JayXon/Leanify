#ifndef BASE64_H
#define BASE64_H

#include "format.h"

class Base64 : public Format
{
public:
    Base64(void *p, size_t s = 0) : Format(p, s) {}

    size_t Leanify(size_t size_leanified = 0);

};


#endif