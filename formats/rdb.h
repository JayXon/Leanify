#ifndef RDB_H
#define RDB_H


#include "format.h"


class Rdb : public Format
{
public:
    Rdb(void *p, size_t s = 0) : Format(p, s) {}
    // using Format::Format;
    // VS2013 does not support C++11 inheriting constructors

    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[16];
};


#endif