#ifndef FORMATS_PE_H_
#define FORMATS_PE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "../utils.h"
#include "format.h"

// PE format specification
// http://msdn.microsoft.com/en-us/gg463119.aspx

extern bool is_verbose;

class Pe : public Format {
 public:
  using Format::Format;

  size_t Leanify(size_t size_leanified = 0) override;

  static const uint8_t header_magic[2];

 private:
  // modified from winnt.h

  PACK(struct ImageFileHeader {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
  });

  PACK(struct ImageOptionalHeader {
    uint16_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint32_t BaseOfData;  // PE32+ does not have BaseOfData
    uint32_t ImageBase;   // PE32+ has uint64_t ImageBase
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint32_t SizeOfStackReserve;  // do not use anything below if it is PE32+
    uint32_t SizeOfStackCommit;
    uint32_t SizeOfHeapReserve;
    uint32_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
  });

  PACK(struct ImageDataDirectory {
    uint32_t VirtualAddress;
    uint32_t Size;
  });

  PACK(struct ImageSectionHeader {
    uint8_t Name[8];
    union {
      uint32_t PhysicalAddress;
      uint32_t VirtualSize;
    };
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
  });

  PACK(struct ImageResourceDirectory {
    uint32_t Characteristics;
    uint32_t TimeDateStamp;
    uint16_t MajorVersion;
    uint16_t MinorVersion;
    uint16_t NumberOfNamedEntries;
    uint16_t NumberOfIdEntries;
  });

  PACK(struct ImageResourceDirectoryEntry {
    union {
      struct {
        uint32_t NameOffset : 31;
        uint32_t NameIsString : 1;
      };
      uint32_t Name;
      uint16_t Id;
    };
    union {
      uint32_t OffsetToData;
      struct {
        uint32_t OffsetToDirectory : 31;
        uint32_t DataIsDirectory : 1;
      };
    };
  });

  PACK(struct ImageResourceDataEntry {
    uint32_t OffsetToData;
    uint32_t Size;
    uint32_t CodePage;
    uint32_t Reserved;
  });

  struct RsrcEntry {
    ImageResourceDataEntry* entry = nullptr;
    ImageResourceDirectoryEntry* dir_entry = nullptr;
    std::string name;
  };

  // decrease RVA inside rsrc section
  void TraverseRSRC(ImageResourceDirectory* res_dir, std::string name = "", const uint32_t move_size = 0);
  bool IsRSRCValid(uint32_t rsrc_virtual_address, uint32_t rsrc_virtual_size);

  uint8_t* rsrc_ = nullptr;
  uint32_t rsrc_raw_size_ = 0;

  std::vector<RsrcEntry> rsrc_data_;
};

#endif  // FORMATS_PE_H_
