#ifndef SWF_H
#define SWF_H

#include "format.h"

extern bool is_verbose;
extern bool is_fast;
extern int iterations;

class Swf : public Format
{
public:
    using Format::Format;

    size_t Leanify(size_t size_leanified = 0) override;

    static const uint8_t header_magic[3];
    static const uint8_t header_magic_deflate[3];
    static const uint8_t header_magic_lzma[3];

private:

    void UpdateTagLength(uint8_t *tag_content, size_t header_length, size_t new_length);
};


#endif