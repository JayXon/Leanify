#ifndef BASE64_H
#define BASE64_H

#include "format.h"

class Base64 : public Format
{
public:
    using Format::Format;

    size_t Leanify(size_t size_leanified = 0);

};


#endif