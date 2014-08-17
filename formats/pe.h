#ifndef PE_H
#define PE_H



#include <cstring>
#include <cstdint>
#include <iostream>

#include "format.h"

// PE format specification
// http://msdn.microsoft.com/en-us/gg463119.aspx


class Pe : Format
{

public:

    Pe(void *p, size_t s = 0) : Format(p, s) {}


    size_t Leanify(size_t size_leanified = 0);

    static const unsigned char header_magic[2];

};


// modified from winnt.h

struct ImageFileHeader {
    uint16_t    Machine;
    uint16_t    NumberOfSections;
    uint32_t    TimeDateStamp;
    uint32_t    PointerToSymbolTable;
    uint32_t    NumberOfSymbols;
    uint16_t    SizeOfOptionalHeader;
    uint16_t    Characteristics;
};

struct ImageSectionHeader {
    uint8_t     Name[8];
    union {
        uint32_t    PhysicalAddress;
        uint32_t    VirtualSize;
    } Misc;
    uint32_t    VirtualAddress;
    uint32_t    SizeOfRawData;
    uint32_t    PointerToRawData;
    uint32_t    PointerToRelocations;
    uint32_t    PointerToLinenumbers;
    uint16_t    NumberOfRelocations;
    uint16_t    NumberOfLinenumbers;
    uint32_t    Characteristics;
};


#endif