#ifndef RDB_H
#define RDB_H

#include <cstdint>
#include <cstring>
#include <iostream>


#include "format.h"
#include "../leanify.h"
#include "../utils.h"


extern int depth;
extern int max_depth;

class Rdb : Format
{
public:
    Rdb(void *p, size_t s = 0) : Format(p, s) {}
    // using Format::Format;
    // VS2013 does not support C++11 inheriting constructors

    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[16];
};


#endif