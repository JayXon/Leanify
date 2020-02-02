#include "pe.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <utility>

#include "../leanify.h"
#include "../utils.h"

using std::cerr;
using std::endl;
using std::string;

const uint8_t Pe::header_magic[] = { 'M', 'Z' };

namespace {

const string resource_types[] = {
  // 0 - 9
  "", "CURSOR", "BITMAP", "ICON", "MENU", "DIALOG", "STRING", "FONTDIR", "FONT", "ACCELERATOR",
  // 10 - 19
  "RCDATA", "MESSAGETABLE", "GROUP_CURSOR", "", "GROUP_ICON", "", "VERSION", "DLGINCLUDE", "", "PLUGPLAY",
  // 20 - 24
  "VXD", "ANICURSOR", "ANIICON", "HTML", "MANIFEST"
};

}  // namespace

size_t Pe::Leanify(size_t size_leanified /*= 0*/) {
  uint32_t pe_header_offset = *(uint32_t*)(fp_ + 0x3C);
  ImageFileHeader* image_file_header = reinterpret_cast<ImageFileHeader*>(fp_ + pe_header_offset + 4);
  ImageOptionalHeader* optional_header =
      reinterpret_cast<ImageOptionalHeader*>((char*)image_file_header + sizeof(ImageFileHeader));
  size_t total_header_size = pe_header_offset + 4 + sizeof(ImageFileHeader);
  size_t min_header_size = total_header_size + sizeof(ImageOptionalHeader);

  // check ImageFileHeader is not out of file range
  // check PE signature: PE00
  // check Section Table is not out of file range
  if (min_header_size > size_ || *(uint32_t*)(fp_ + pe_header_offset) != 0x00004550 ||
      (total_header_size += image_file_header->SizeOfOptionalHeader +
                            sizeof(ImageSectionHeader) * image_file_header->NumberOfSections) > size_ ||
      (optional_header->Magic != 0x10B && optional_header->Magic != 0x20B) || optional_header->FileAlignment == 0 ||
      optional_header->SectionAlignment == 0) {
    cerr << "Not a valid PE file." << endl;
    return Format::Leanify(size_leanified);
  }

  if (optional_header->Subsystem == 1) {
    VerbosePrint(".sys (driver file) detected, skip.");
    return Format::Leanify(size_leanified);
  }

  // make sure total_header_size will cover Optional Header
  if (total_header_size < min_header_size)
    total_header_size = min_header_size;

  const uint32_t new_pe_header_offset = 0x10;

  // if original file's PE header is not overlapped with DOS header
  // then we can overlap it to save some space
  if (pe_header_offset >= 0x40) {
    total_header_size -= pe_header_offset - new_pe_header_offset;
    memcpy(fp_ - size_leanified, header_magic, sizeof(header_magic));
    memset(fp_ - size_leanified + sizeof(header_magic), 0, new_pe_header_offset - sizeof(header_magic));
    memmove(fp_ - size_leanified + new_pe_header_offset, fp_ + pe_header_offset,
            total_header_size - new_pe_header_offset);
    image_file_header = reinterpret_cast<ImageFileHeader*>(fp_ - size_leanified + 0x14);
    optional_header = reinterpret_cast<ImageOptionalHeader*>((char*)image_file_header + sizeof(ImageFileHeader));
    // set new PE Header offset
    *(uint32_t*)(fp_ - size_leanified + 0x3C) = new_pe_header_offset;
  } else {
    // this file probably already packed with some extreme packer
    VerbosePrint("PE Header already overlaps DOS Header.");
    if (size_leanified) {
      // move entire SizeOfHeaders to make sure nothing was missed
      memmove(fp_ - size_leanified, fp_, *(uint32_t*)(fp_ + pe_header_offset + 0x54));
      image_file_header = reinterpret_cast<ImageFileHeader*>(fp_ - size_leanified + pe_header_offset + 4);
    }
  }

  // Data Directories
  // PE32:    Magic number: 0x10B     Offset: 0x60
  // PE32+:   Magic number: 0x20B     Offset: 0x70
  ImageDataDirectory* data_directories =
      reinterpret_cast<ImageDataDirectory*>((char*)optional_header + (optional_header->Magic == 0x10B ? 0x60 : 0x70));

  // Section Table
  ImageSectionHeader* section_table =
      reinterpret_cast<ImageSectionHeader*>((char*)optional_header + image_file_header->SizeOfOptionalHeader);

  // Base Relocation Table VirtualAddress
  uint32_t reloc_virtual_address = data_directories[5].VirtualAddress;
  uint32_t rsrc_virtual_address = data_directories[2].VirtualAddress;

  uint32_t reloc_raw_offset = 0;
  uint32_t reloc_raw_size = 0;
  uint32_t rsrc_raw_offset = 0;
  uint32_t rsrc_virtual_size = 0;

  // 0x2000: IMAGE_FILE_DLL
  // only remove reloc section if it's not dll
  if (!(image_file_header->Characteristics & 0x2000)) {
    // set IMAGE_FILE_RELOCS_STRIPPED
    image_file_header->Characteristics |= 0x0001;
  }

  // Some PE have wrong SizeOfHeaders, e.g. files packed by UPX
  // I have to find the smallest PointerToRawData in all sections
  // If that is smaller than SizeOfHeaders, then that's the correct SizeOfHeaders
  uint32_t correct_size_of_headers = optional_header->SizeOfHeaders;

  // looking for reloc and rsrc in Section Table
  for (int i = 0; i < image_file_header->NumberOfSections; i++) {
    if (reloc_virtual_address && section_table[i].VirtualAddress == reloc_virtual_address) {
      // if IMAGE_FILE_RELOCS_STRIPPED is set
      if (image_file_header->Characteristics & 0x0001) {
        reloc_raw_offset = section_table[i].PointerToRawData;
        reloc_raw_size = section_table[i].SizeOfRawData;

        // remove Relocation Table in Data Directories
        data_directories[5].VirtualAddress = 0;
        data_directories[5].Size = 0;

        image_file_header->NumberOfSections--;

        // move sections after reloc forward
        memmove(&section_table[i], &section_table[i + 1],
                sizeof(ImageSectionHeader) * (image_file_header->NumberOfSections - i));

        total_header_size -= sizeof(ImageSectionHeader);
        i--;

        VerbosePrint("Relocation Table removed.");
      }
    } else if (rsrc_virtual_address && section_table[i].VirtualAddress == rsrc_virtual_address) {
      rsrc_raw_offset = section_table[i].PointerToRawData;
      rsrc_raw_size_ = section_table[i].SizeOfRawData;
      rsrc_virtual_size = section_table[i].VirtualSize;
      rsrc_ = fp_ + rsrc_raw_offset;
    }
    if (section_table[i].PointerToRawData < correct_size_of_headers && section_table[i].SizeOfRawData) {
      correct_size_of_headers = section_table[i].PointerToRawData;
    }
  }

  if (correct_size_of_headers > size_)
    correct_size_of_headers = size_;

  // has to be multiple of FileAlignment
  uint32_t header_size_aligned = RoundUp(total_header_size, optional_header->FileAlignment);
  size_t pe_size_leanified = 0;
  size_t header_size_leanified = 0;

  // if header size is reduced after aligned
  if (header_size_aligned < correct_size_of_headers) {
    header_size_leanified = pe_size_leanified = correct_size_of_headers - header_size_aligned;
    optional_header->SizeOfHeaders = header_size_aligned;
  } else if (total_header_size <= correct_size_of_headers) {
    header_size_aligned = correct_size_of_headers;
  }

  // fill the rest of the headers with 0
  // only if PE Header offset has changed
  if (pe_header_offset >= 0x40) {
    memset(fp_ - size_leanified + total_header_size, 0, header_size_aligned - total_header_size);
  }

  // multiple of SectionAlignment
  uint32_t reloc_virtual_size = RoundUp(reloc_raw_size, optional_header->SectionAlignment);

  // set ASLR in DllCharacteristics to false
  // it seems this isn't necessary
  // *(uint16_t *)(optional_header + 0x46) &= ~0x0040;

  uint32_t reloc_end = reloc_raw_offset + reloc_raw_size;
  size_t rsrc_size_leanified = 0;
  if (rsrc_raw_size_) {
    // decrease RVA inside rsrc section
    if (rsrc_virtual_address > reloc_virtual_address) {
      TraverseRSRC(reinterpret_cast<ImageResourceDirectory*>(fp_ + rsrc_raw_offset), "", reloc_virtual_size);
      rsrc_virtual_address -= reloc_virtual_size;
    } else {
      // only save all the data addresses to vector
      TraverseRSRC(reinterpret_cast<ImageResourceDirectory*>(fp_ + rsrc_raw_offset));
    }

    VerbosePrint(rsrc_data_.size(), " embedded resources found.");
    // sort it according to it's data offset
    std::sort(rsrc_data_.begin(), rsrc_data_.end(),
              [](const RsrcEntry& a, const RsrcEntry& b) { return a.entry->OffsetToData < b.entry->OffsetToData; });

    // detect non standard resource, maybe produced by some packer
    if (rsrc_data_.empty() || !IsRSRCValid(rsrc_virtual_address, rsrc_virtual_size)) {
      VerbosePrint("Non standard resource detected.");
      if (reloc_raw_size) {
        // move everything before reloc
        memmove(fp_ - size_leanified + header_size_aligned, fp_ + header_size_aligned + pe_size_leanified,
                reloc_raw_offset - header_size_aligned);

        // move everything after reloc
        memmove(fp_ - size_leanified + reloc_raw_offset - pe_size_leanified, fp_ + reloc_end, size_ - reloc_raw_offset);
        pe_size_leanified += reloc_raw_size;
      } else {
        memmove(fp_ - size_leanified + header_size_aligned, fp_ + header_size_aligned + pe_size_leanified,
                size_ - header_size_aligned - pe_size_leanified);
      }
    } else {
      // should do this before memmove
      uint32_t old_end = rsrc_data_.back().entry->OffsetToData + rsrc_data_.back().entry->Size;
      uint32_t last_end = rsrc_data_[0].entry->OffsetToData;

      if (reloc_raw_size && reloc_raw_offset < rsrc_raw_offset) {
        // move everything before reloc
        memmove(fp_ - size_leanified + header_size_aligned, fp_ + header_size_aligned + pe_size_leanified,
                reloc_raw_offset - header_size_aligned - pe_size_leanified);

        // move things between reloc and first data of rsrc
        memmove(fp_ - size_leanified + reloc_raw_offset - pe_size_leanified, fp_ + reloc_end,
                rsrc_raw_offset - reloc_end + rsrc_data_[0].entry->OffsetToData - rsrc_virtual_address);
        pe_size_leanified += reloc_raw_size;

      } else {
        // move everything before first data of rsrc
        memmove(fp_ - size_leanified + header_size_aligned, fp_ + header_size_aligned + pe_size_leanified,
                rsrc_raw_offset - pe_size_leanified - header_size_aligned + rsrc_data_[0].entry->OffsetToData -
                    rsrc_virtual_address);
      }

      depth++;

      for (auto& res : rsrc_data_) {
        if (depth <= max_depth) {
          // print resource name
          PrintFileName(res.name);
        }

        // The offset of the data entry itself.
        uint32_t entry_offset = reinterpret_cast<uint8_t*>(res.entry) - rsrc_ + rsrc_virtual_address;
        // Some PE stores the data entry right before actual data.
        if (entry_offset >= last_end && entry_offset < res.entry->OffsetToData) {
          memmove(fp_ - size_leanified + rsrc_raw_offset + last_end - rsrc_virtual_address - pe_size_leanified,
                  fp_ + rsrc_raw_offset + entry_offset - rsrc_virtual_address, sizeof(ImageResourceDataEntry));
          uint32_t diff = entry_offset - last_end;
          res.entry = reinterpret_cast<ImageResourceDataEntry*>(reinterpret_cast<uint8_t*>(res.entry) - diff);

          res.dir_entry = reinterpret_cast<ImageResourceDirectoryEntry*>(reinterpret_cast<uint8_t*>(res.dir_entry) -
                                                                         pe_size_leanified - size_leanified);
          res.dir_entry->OffsetToData -= diff;
          last_end = entry_offset + sizeof(ImageResourceDataEntry);
        }

        res.entry = reinterpret_cast<ImageResourceDataEntry*>(reinterpret_cast<uint8_t*>(res.entry) -
                                                              pe_size_leanified - size_leanified);
        ImageResourceDataEntry* entry = res.entry;

        // it seems some of the resource has to be aligned to 4 bytes in order to work
        // only align the resource if it is aligned before
        if ((entry->OffsetToData & 3) == 0) {
          // fill the gap with 0
          uint32_t gap = RoundUp(last_end, 4) - last_end;
          memset(fp_ - size_leanified + rsrc_raw_offset + last_end - rsrc_virtual_address - pe_size_leanified, 0, gap);
          last_end += gap;
        }

        size_t new_size = LeanifyFile(fp_ + rsrc_raw_offset + entry->OffsetToData - rsrc_virtual_address, entry->Size,
                                      entry->OffsetToData - last_end + pe_size_leanified + size_leanified, res.name);
        entry->OffsetToData = last_end;
        entry->Size = new_size;
        last_end += new_size;
      }
      depth--;
      rsrc_size_leanified = old_end - last_end;
      uint32_t rsrc_new_end = rsrc_raw_offset + last_end - rsrc_virtual_address;
      uint32_t rsrc_new_end_aligned = RoundUp(rsrc_new_end, optional_header->FileAlignment);
      uint32_t rsrc_correct_end_aligned =
          RoundUp(rsrc_raw_offset + old_end - rsrc_virtual_address, optional_header->FileAlignment);
      uint32_t rsrc_end = rsrc_raw_offset + rsrc_raw_size_;

      // fill the rest of rsrc with 0
      memset(fp_ - size_leanified + rsrc_new_end - pe_size_leanified, 0, rsrc_size_leanified);

      bool iat_in_rsrc =
          data_directories[1].VirtualAddress > rsrc_virtual_address &&
          data_directories[1].VirtualAddress + data_directories[1].Size <= rsrc_virtual_address + rsrc_raw_size_;
      // if rsrc_correct_end_aligned != rsrc_end then there are extra data in rsrc section
      if (rsrc_new_end_aligned <= rsrc_end && rsrc_correct_end_aligned == rsrc_end && !iat_in_rsrc) {
        if (rsrc_new_end_aligned > rsrc_new_end + rsrc_size_leanified) {
          memmove(fp_ - size_leanified + rsrc_new_end + rsrc_size_leanified - pe_size_leanified,
                  fp_ + rsrc_new_end + rsrc_size_leanified, rsrc_new_end_aligned - rsrc_new_end - rsrc_size_leanified);
        }
        rsrc_virtual_size = rsrc_new_end - rsrc_raw_offset;
        rsrc_size_leanified = rsrc_end - rsrc_new_end_aligned;
        pe_size_leanified += rsrc_size_leanified;
        rsrc_raw_size_ = rsrc_new_end_aligned - rsrc_raw_offset;
      } else {
        if (rsrc_end > rsrc_new_end + rsrc_size_leanified) {
          memmove(fp_ - size_leanified + rsrc_new_end + rsrc_size_leanified - pe_size_leanified,
                  fp_ + rsrc_new_end + rsrc_size_leanified, rsrc_end - rsrc_new_end - rsrc_size_leanified);
        }
        rsrc_size_leanified = 0;
      }
      if (reloc_raw_size && reloc_raw_offset > rsrc_raw_offset) {
        memmove(fp_ - size_leanified + rsrc_end - pe_size_leanified, fp_ + rsrc_end, reloc_raw_offset - rsrc_end);
        memmove(fp_ - size_leanified + reloc_raw_offset - pe_size_leanified, fp_ + reloc_end, size_ - reloc_end);
        pe_size_leanified += reloc_raw_size;
      } else {
        memmove(fp_ - size_leanified + rsrc_end - pe_size_leanified, fp_ + rsrc_end, size_ - rsrc_end);
      }
    }

  } else if (reloc_raw_size) {
    // no rsrc but have reloc to remove

    // move everything before reloc
    memmove(fp_ - size_leanified + header_size_aligned, fp_ + header_size_aligned + pe_size_leanified,
            reloc_raw_offset - header_size_aligned);

    // move everything after reloc
    memmove(fp_ - size_leanified + reloc_raw_offset - pe_size_leanified, fp_ + reloc_end, size_ - reloc_raw_offset);
    pe_size_leanified += reloc_raw_size;
  } else {
    // no rsrc and no reloc
    memmove(fp_ - size_leanified + header_size_aligned, fp_ + header_size_aligned + pe_size_leanified,
            size_ - header_size_aligned - pe_size_leanified);
  }

  // decrease SizeOfImage
  int rsrc_decrease_size = RoundUp(data_directories[2].Size, optional_header->SectionAlignment) -
                           RoundUp(rsrc_virtual_size, optional_header->SectionAlignment);
  if (rsrc_decrease_size < 0)
    rsrc_decrease_size = 0;

  optional_header->SizeOfImage -= reloc_virtual_size + rsrc_decrease_size;

  VerbosePrint("Update Section Table.");

  for (int i = 0; i < image_file_header->NumberOfSections; i++) {
    if (section_table[i].VirtualAddress > reloc_virtual_address) {
      section_table[i].VirtualAddress -= reloc_virtual_size;
    }
    if (section_table[i].VirtualAddress > rsrc_virtual_address) {
      section_table[i].VirtualAddress -= rsrc_decrease_size;
    } else if (section_table[i].VirtualAddress == rsrc_virtual_address) {
      section_table[i].SizeOfRawData = rsrc_raw_size_;
      section_table[i].VirtualSize = rsrc_virtual_size;
    }
    if (section_table[i].PointerToRawData > reloc_raw_offset) {
      section_table[i].PointerToRawData -= reloc_raw_size;
    }
    if (section_table[i].PointerToRawData > rsrc_raw_offset) {
      section_table[i].PointerToRawData -= rsrc_size_leanified;
    }
    if (section_table[i].PointerToRawData > header_size_aligned) {
      section_table[i].PointerToRawData -= header_size_leanified;
    }
  }

  // read NumberOfRvaAndSizes
  // this can work on both PE32 and PE32+
  uint32_t num_data_dir = *(uint32_t*)((char*)data_directories - 4);
  if (num_data_dir > 16)
    num_data_dir = 16;

  // update Data Directories too
  for (uint32_t i = 0; i < num_data_dir; i++) {
    if (data_directories[i].VirtualAddress > reloc_virtual_address) {
      data_directories[i].VirtualAddress -= reloc_virtual_size;
    }
    if (data_directories[i].VirtualAddress >=
        rsrc_virtual_address + RoundUp(rsrc_virtual_size, optional_header->SectionAlignment)) {
      data_directories[i].VirtualAddress -= rsrc_decrease_size;
    }
  }

  // the size in Data Directory can not larger than VirtualSize in Section Table
  // but for some of the packed file, it will be much smaller
  // so only update size in Data Directory if exe is not packed by these kind of packer
  if (data_directories[2].Size > rsrc_virtual_size)
    data_directories[2].Size = rsrc_virtual_size;

  fp_ -= size_leanified;
  size_ -= pe_size_leanified;

  return size_;
}

// decrease RVA inside rsrc section
void Pe::TraverseRSRC(ImageResourceDirectory* res_dir, string name /*= ""*/, const uint32_t move_size /*= 0*/) {
  ImageResourceDirectoryEntry* entry = (ImageResourceDirectoryEntry*)((char*)res_dir + sizeof(ImageResourceDirectory));
  for (int i = 0; i < res_dir->NumberOfNamedEntries + res_dir->NumberOfIdEntries; i++) {
    string new_name;
    if (entry[i].NameIsString) {
      // the name string has a 2 byte size preceding the UNICODE string
      uint16_t len = *(uint16_t*)(rsrc_ + entry[i].NameOffset);
      char mbs[256] = { 0 };
      UTF16toMBS((wchar_t*)(rsrc_ + entry[i].NameOffset + 2), len * 2, mbs, sizeof(mbs));
      new_name = name + mbs;
    } else if (name.empty() && entry[i].Name < sizeof(resource_types) / sizeof(string) &&
               !resource_types[entry[i].Name].empty()) {
      // use Predefined Resource Types string instead of an ID
      new_name = resource_types[entry[i].Name];
    } else {
      new_name = name + std::to_string(entry[i].Name);
    }

    if (entry[i].OffsetToDirectory > rsrc_raw_size_) {
      cerr << "Invalid resource address!" << endl;
      return;
    }
    if (entry[i].DataIsDirectory) {
      TraverseRSRC(reinterpret_cast<ImageResourceDirectory*>(rsrc_ + entry[i].OffsetToDirectory), new_name + '/',
                   move_size);
    } else {
      *(uint32_t*)(rsrc_ + entry[i].OffsetToData) -= move_size;
      // remember the address to Leanify resource file later
      rsrc_data_.push_back(
          { reinterpret_cast<ImageResourceDataEntry*>(rsrc_ + entry[i].OffsetToData), &entry[i], new_name });
    }
  }
}

bool Pe::IsRSRCValid(uint32_t rsrc_virtual_address, uint32_t rsrc_virtual_size) {
  uint32_t prev_end = 0;
  for (const RsrcEntry& entry : rsrc_data_) {
    uint32_t offset = entry.entry->OffsetToData;
    uint32_t size = entry.entry->Size;

    if (offset < rsrc_virtual_address || offset > rsrc_virtual_address + rsrc_virtual_size)
      return false;
    if (offset - rsrc_virtual_address + size > rsrc_raw_size_)
      return false;
    if (prev_end > offset)
      return false;
    prev_end = offset + size;
  }
  return true;
}
