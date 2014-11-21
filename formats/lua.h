#ifndef LUA_H
#define LUA_H


#include "format.h"


class Lua : Format
{
public:
    Lua(void *p, size_t s = 0) : Format(p, s), p_read((char *)p), p_write((char *)p) {}

    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[4];
private:
    void FunctionParser();
    char *p_read, *p_write;
};


#endif