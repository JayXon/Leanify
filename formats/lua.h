#ifndef FORMATS_LUA_H_
#define FORMATS_LUA_H_

#include "format.h"

class Lua : public Format
{
public:
    explicit Lua(void *p, size_t s = 0) : Format(p, s), p_read_(static_cast<uint8_t *>(p)), p_write_(static_cast<uint8_t *>(p)) {}

    size_t Leanify(size_t size_leanified = 0) override;

    static const uint8_t header_magic[4];

private:
    void FunctionParser();
    uint8_t *p_read_, *p_write_;
};

#endif  // FORMATS_LUA_H_
