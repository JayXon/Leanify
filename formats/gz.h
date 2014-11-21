#ifndef GZ_H
#define GZ_H


#include "format.h"


extern bool is_fast;
extern int iterations;

class Gz : Format
{
public:
    Gz(void *p, size_t s = 0) : Format(p, s) {}

    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[3];
};


#endif