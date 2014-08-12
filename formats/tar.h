#ifndef TAR_H
#define TAR_H


#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdint>

#include "format.h"
#include "../leanify.h"

extern bool is_verbose;
extern int level;

class Tar : Format
{
public:
    Tar(void *p, size_t s = 0) : Format(p, s) {}

    ~Tar() { level--; }

    size_t Leanify(size_t size_leanified = 0);

    bool IsValid() const
    {
        return CalcChecksum((unsigned char *)fp) == strtol(fp + 148, nullptr, 8);
    }

private:
    int CalcChecksum(unsigned char *header) const;

};


#endif