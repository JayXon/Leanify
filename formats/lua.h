#ifndef LUA_H
#define LUA_H


#include "format.h"


class Lua : public Format
{
public:
    explicit Lua(void *p, size_t s = 0) : Format(p, s), p_read(static_cast<uint8_t *>(p)), p_write(static_cast<uint8_t *>(p)) {}

    size_t Leanify(size_t size_leanified = 0) override;

    static const uint8_t header_magic[4];

private:
    void FunctionParser();
    uint8_t *p_read, *p_write;
};


#endif