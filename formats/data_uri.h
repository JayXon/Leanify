#ifndef DATA_URI_H
#define DATA_URI_H


#include "format.h"


extern bool is_verbose;


class DataURI : public Format
{
public:
    using Format::Format;

    size_t Leanify(size_t size_leanified = 0) override;
};


#endif