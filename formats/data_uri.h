#ifndef DATA_URI_H
#define DATA_URI_H


#include "format.h"


extern bool is_verbose;


class DataURI : public Format
{
public:
    DataURI(void *p, size_t s = 0) : Format(p, s) {}

    size_t Leanify(size_t size_leanified = 0);
};


#endif