#ifndef PE_H
#define PE_H

#include <cstdint>
#include <string>
#include <vector>

#include "format.h"


// PE format specification
// http://msdn.microsoft.com/en-us/gg463119.aspx

extern bool is_verbose;

class Pe : public Format
{

public:

    Pe(void *p, size_t s = 0) : Format(p, s), rsrc(nullptr), rsrc_raw_size(0) {}


    size_t Leanify(size_t size_leanified = 0);

    static const uint8_t header_magic[2];

private:

    // modified from winnt.h

    struct ImageFileHeader
    {
        uint16_t    Machine;
        uint16_t    NumberOfSections;
        uint32_t    TimeDateStamp;
        uint32_t    PointerToSymbolTable;
        uint32_t    NumberOfSymbols;
        uint16_t    SizeOfOptionalHeader;
        uint16_t    Characteristics;
    };

    struct ImageOptionalHeader
    {
        uint16_t    Magic;
        uint8_t     MajorLinkerVersion;
        uint8_t     MinorLinkerVersion;
        uint32_t    SizeOfCode;
        uint32_t    SizeOfInitializedData;
        uint32_t    SizeOfUninitializedData;
        uint32_t    AddressOfEntryPoint;
        uint32_t    BaseOfCode;
        uint32_t    BaseOfData;     // PE32+ does not have BaseOfData
        uint32_t    ImageBase;      // PE32+ has uint64_t ImageBase
        uint32_t    SectionAlignment;
        uint32_t    FileAlignment;
        uint16_t    MajorOperatingSystemVersion;
        uint16_t    MinorOperatingSystemVersion;
        uint16_t    MajorImageVersion;
        uint16_t    MinorImageVersion;
        uint16_t    MajorSubsystemVersion;
        uint16_t    MinorSubsystemVersion;
        uint32_t    Win32VersionValue;
        uint32_t    SizeOfImage;
        uint32_t    SizeOfHeaders;
        uint32_t    CheckSum;
        uint16_t    Subsystem;
        uint16_t    DllCharacteristics;
        uint32_t    SizeOfStackReserve;     // do not use anything below if it is PE32+
        uint32_t    SizeOfStackCommit;
        uint32_t    SizeOfHeapReserve;
        uint32_t    SizeOfHeapCommit;
        uint32_t    LoaderFlags;
        uint32_t    NumberOfRvaAndSizes;
    };

    struct ImageDataDirectory
    {
        uint32_t    VirtualAddress;
        uint32_t    Size;
    };

    struct ImageSectionHeader
    {
        uint8_t     Name[8];
        union
        {
            uint32_t    PhysicalAddress;
            uint32_t    VirtualSize;
        };
        uint32_t    VirtualAddress;
        uint32_t    SizeOfRawData;
        uint32_t    PointerToRawData;
        uint32_t    PointerToRelocations;
        uint32_t    PointerToLinenumbers;
        uint16_t    NumberOfRelocations;
        uint16_t    NumberOfLinenumbers;
        uint32_t    Characteristics;
    };


    struct ImageResourceDirectory
    {
        uint32_t    Characteristics;
        uint32_t    TimeDateStamp;
        uint16_t    MajorVersion;
        uint16_t    MinorVersion;
        uint16_t    NumberOfNamedEntries;
        uint16_t    NumberOfIdEntries;
    };

    struct ImageResourceDirectoryEntry
    {
        union
        {
            struct
            {
                uint32_t    NameOffset : 31;
                uint32_t    NameIsString : 1;
            };
            uint32_t    Name;
            uint16_t    Id;
        };
        union
        {
            uint32_t    OffsetToData;
            struct
            {
                uint32_t    OffsetToDirectory : 31;
                uint32_t    DataIsDirectory : 1;
            };
        };
    };

    // decrease RVA inside rsrc section
    void TraverseRSRC(ImageResourceDirectory *res_dir, std::string name = "", const uint32_t move_size = 0);

    uint8_t *rsrc;
    uint32_t rsrc_raw_size;

    std::vector<std::pair<uint32_t *, std::string> > rsrc_data;

    static const std::string resource_types[];
};

#endif