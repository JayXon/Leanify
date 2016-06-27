#include "lua.h"

#include <cstdint>
#include <cstring>

const uint8_t Lua::header_magic[] = { 0x1B, 0x4C, 0x75, 0x61 };

// copied from an old code, need to refactor someday
void Lua::FunctionParser() {
  uint32_t i;
  uint8_t* oldp;

  // Function header
  i = *(uint32_t*)p_read_;
  p_read_ += i + 4;

  // remove Source name, Line defined and last line defined
  memset(p_write_, 0, 12);
  *(uint32_t*)(p_write_ + 12) = *(uint32_t*)(p_read_ + 8);
  p_write_ += 0x10;
  p_read_ += 0xC;

  // Instruction list
  i = *(uint32_t*)p_read_ * 4 + 4;
  memmove(p_write_, p_read_, i);
  p_write_ += i;
  p_read_ += i;

  oldp = p_read_;
  // Constant list
  i = *(uint32_t*)p_read_;
  p_read_ += 4;
  while (i--) {
    switch (*p_read_++) {
      // 1=LUA_TBOOLEAN
      case 1:
        p_read_++;
        break;
      // 3=LUA_TNUMBER
      case 3:
        p_read_ += 8;
        break;
      // 4=LUA_TSTRING
      case 4:
        p_read_ += *(uint32_t*)p_read_ + 4;
    }
  }

  // Function prototype list
  i = *(uint32_t*)p_read_;
  p_read_ += 4;

  memmove(p_write_, oldp, p_read_ - oldp);
  p_write_ += p_read_ - oldp;
  while (i--) {
    FunctionParser();
  }

  // Source line position list (optional debug data)
  p_read_ += *((uint32_t*)p_read_) * 4 + 4;

  // Local list (optional debug data)
  i = *(uint32_t*)p_read_;
  p_read_ += 4;
  while (i--) {
    p_read_ += *(uint32_t*)p_read_ + 12;
  }

  // Upvalue list (optional debug data)
  i = *(uint32_t*)p_read_;
  p_read_ += 4;
  while (i--) {
    p_read_ += *(uint32_t*)p_read_ + 4;
  }

  // strip above optional debug data
  memset(p_write_, 0, 12);
  p_write_ += 12;
}

size_t Lua::Leanify(size_t size_leanified /*= 0*/) {
  // skip header
  p_read_ += 0xC;
  p_write_ = p_read_ - size_leanified;
  fp_ -= size_leanified;
  FunctionParser();

  return p_write_ - fp_;
}