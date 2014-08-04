#ifndef RDB_H
#define RDB_H

#include <iostream>
#include <cstring>
#include <cstdint>

#include "format.h"
#include "../leanify.h"

extern int level;

class Rdb : Format
{
public:
    Rdb(void *p, uint32_t s = 0) : Format(p, s) {}
    // using Format::Format;
    // VS2013 does not support C++11 inheriting constructors
    ~Rdb() { level--; }

    uint32_t Leanify(uint32_t size_leanified = 0);

    static const unsigned char header_magic[16];
};


#endif