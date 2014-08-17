#include "pe.h"



const unsigned char Pe::header_magic[] = { 'M', 'Z' };

size_t Pe::Leanify(size_t size_leanified /*= 0*/)
{

    uint32_t pe_header = *(uint32_t *)(fp + 0x3C);
    // check PE signature: PE00
    if (pe_header < size && *(uint32_t *)(fp + pe_header) != 0x00004550)
    {
        std::cout << "Not a PE file." << std::endl;
        if (size_leanified)
        {
            fp -= size_leanified;
            memmove(fp, fp + size_leanified, size);
        }
        return size;
    }
    ImageFileHeader *pimage_file_header = (ImageFileHeader *)(fp + pe_header + 4);

    // 0x0001: IMAGE_FILE_RELOCS_STRIPPED
    // 0x2000: IMAGE_FILE_DLL
    // only remove reloc section when both bit are not set
    if (!(pimage_file_header->Characteristics & 0x0001) && !(pimage_file_header->Characteristics & 0x2000))
    {
        // set IMAGE_FILE_RELOCS_STRIPPED
        pimage_file_header->Characteristics |= 0x0001;

        char *optional_header = (char *)pimage_file_header + sizeof(ImageFileHeader);
        uint32_t section_alignment = *(uint32_t *)(optional_header + 0x20);

        // Data Directories
        // PE32:    Magic number: 0x10B     Offset: 0x60
        // PE32+:   Magic number: 0x20B     Offset: 0x70
        ImageDataDirectory *data_directories = (ImageDataDirectory *)(optional_header + (*(uint16_t *)optional_header == 0x10B ? 0x60 : 0x70));

        // Base Relocation Table VirtualAddress
        uint32_t reloc_virtual_address = data_directories[5].VirtualAddress;
        // test if it is a valid address
        // some PE will use this space for other purpose
        if (reloc_virtual_address % section_alignment == 0 && data_directories[5].Size < size)
        {
            data_directories[5].VirtualAddress = 0;
            // Base Relocation Table Size
            data_directories[5].Size = 0;
        }

        // Section Table
        ImageSectionHeader *section_table = (ImageSectionHeader *)(optional_header + pimage_file_header->SizeOfOptionalHeader);
        uint32_t reloc_raw_offset = 0;
        uint32_t reloc_raw_size = 0;
        for (int i = 0; i < pimage_file_header->NumberOfSections; i++)
        {
            if (section_table[i].VirtualAddress == reloc_virtual_address)
            {
                reloc_raw_offset = section_table[i].PointerToRawData;
                reloc_raw_size = section_table[i].SizeOfRawData;

                pimage_file_header->NumberOfSections--;

                // move sections after reloc forward
                memmove(&section_table[i], &section_table[i + 1], sizeof(ImageSectionHeader) * (pimage_file_header->NumberOfSections - i));

                // fill the last section with 0
                memset(&section_table[pimage_file_header->NumberOfSections], 0, sizeof(ImageSectionHeader));
                break;
            }
        }
        if (reloc_raw_size)
        {
            // multiple of SectionAlignment
            uint32_t reloc_virtual_size = ((reloc_raw_size - 1) & ~(section_alignment - 1)) + section_alignment;

            // decrease SizeOfImage
            *(uint32_t *)(optional_header + 0x38) -= reloc_virtual_size;

            // set ASLR in DllCharacteristics to false
            // it seems this isn't necessary
            // *(uint16_t *)(optional_header + 0x46) &= ~0x0040;



            // move other section address forward if it is behind reloc
            for (int i = 0; i < pimage_file_header->NumberOfSections; i++)
            {
                if (section_table[i].VirtualAddress > reloc_virtual_address)
                {
                    // move rsrc
                    if (section_table[i].VirtualAddress == data_directories[2].VirtualAddress)
                    {
                        MoveRSRC(fp + section_table[i].PointerToRawData, (ImageResourceDirectory *)(fp + section_table[i].PointerToRawData), reloc_virtual_size);
                    }
                    section_table[i].VirtualAddress -= reloc_virtual_size;
                }
                if (section_table[i].PointerToRawData > reloc_raw_offset)
                {
                    section_table[i].PointerToRawData -= reloc_raw_size;
                }
            }
            // do it with Data Directories too
            for (int i = 0; i < 16; i++)
            {
                if (data_directories[i].VirtualAddress > reloc_virtual_address)
                {
                    data_directories[i].VirtualAddress -= reloc_virtual_size;
                }
            }
        }

        // move everything before reloc
        if (size_leanified)
        {
            fp -= size_leanified;
            memmove(fp, fp + size_leanified, reloc_raw_offset);
        }
        // move everything after reloc
        size -= reloc_raw_size;
        memmove(fp + reloc_raw_offset, fp + size_leanified + reloc_raw_offset + reloc_raw_size, size - reloc_raw_offset);
    }
    else
    {
        if (size_leanified)
        {
            fp -= size_leanified;
            memmove(fp, fp + size_leanified, size);
        }
    }

    return size;

}


// decrease RVA inside rsrc section
void Pe::MoveRSRC(char *rsrc, ImageResourceDirectory *res_dir, uint32_t move_size)
{
    ImageResourceDirectoryEntry *entry = (ImageResourceDirectoryEntry *)((char *)res_dir + sizeof(ImageResourceDirectory));
    for (int i = 0; i < res_dir->NumberOfNamedEntries + res_dir->NumberOfIdEntries; i++)
    {
        if (entry[i].DataIsDirectory)
        {
            MoveRSRC(rsrc, (ImageResourceDirectory *)(rsrc + entry[i].OffsetToDirectory), move_size);
        }
        else
        {
            *(uint32_t *)(rsrc + entry[i].OffsetToData) -= move_size;
        }
    }
}


