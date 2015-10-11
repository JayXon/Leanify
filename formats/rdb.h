#ifndef RDB_H
#define RDB_H


#include "format.h"


class Rdb : public Format
{
public:
    using Format::Format;

    size_t Leanify(size_t size_leanified = 0);

    static const uint8_t header_magic[16];
};


#endif