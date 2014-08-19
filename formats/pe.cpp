#include "pe.h"



const unsigned char Pe::header_magic[] = { 'M', 'Z' };

size_t Pe::Leanify(size_t size_leanified /*= 0*/)
{

    uint32_t pe_header_offset = *(uint32_t *)(fp + 0x3C);
    // check PE signature: PE00
    if (pe_header_offset < size && *(uint32_t *)(fp + pe_header_offset) != 0x00004550)
    {
        std::cout << "Not a PE file." << std::endl;
        if (size_leanified)
        {
            fp -= size_leanified;
            memmove(fp, fp + size_leanified, size);
        }
        return size;
    }

    ImageFileHeader *image_file_header = (ImageFileHeader *)(fp + pe_header_offset + 4);
    size_t total_header_size = 0x28 + image_file_header->SizeOfOptionalHeader + sizeof(ImageSectionHeader) * image_file_header->NumberOfSections;

    const uint32_t new_pe_header_offset = 0x10;

    // if original file's PE header is not overlapped with DOS header
    // then we can overlap it to save some space
    if (pe_header_offset >= 0x40)
    {
        memcpy(fp - size_leanified, header_magic, sizeof(header_magic));
        memset(fp - size_leanified + sizeof(header_magic), 0, new_pe_header_offset - sizeof(header_magic));
        memmove(fp - size_leanified + new_pe_header_offset, fp + pe_header_offset, total_header_size - new_pe_header_offset);
        image_file_header = (ImageFileHeader *)(fp - size_leanified + 0x14);
        // set new PE Header offset
        *(uint32_t *)(fp - size_leanified + 0x3C) = new_pe_header_offset;
    }
    else
    {
        // this file probably already packed with some extreme packer
        total_header_size += pe_header_offset - new_pe_header_offset;
        if (size_leanified)
        {
            // move entire SizeOfHeaders to make sure nothing was missed
            memmove(fp - size_leanified, fp, *(uint32_t *)(fp + pe_header_offset + 0x54));
            image_file_header = (ImageFileHeader *)(fp - size_leanified + pe_header_offset + 4);
        }
    }

    ImageOptionalHeader *optional_header = (ImageOptionalHeader *)((char *)image_file_header + sizeof(ImageFileHeader));

    // Data Directories
    // PE32:    Magic number: 0x10B     Offset: 0x60
    // PE32+:   Magic number: 0x20B     Offset: 0x70
    ImageDataDirectory *data_directories = (ImageDataDirectory *)((char *)optional_header + (optional_header->Magic == 0x10B ? 0x60 : 0x70));

    // Section Table
    ImageSectionHeader *section_table = (ImageSectionHeader *)((char *)optional_header + image_file_header->SizeOfOptionalHeader);



    // Base Relocation Table VirtualAddress
    uint32_t reloc_virtual_address = data_directories[5].VirtualAddress;

    uint32_t reloc_raw_offset = 0;
    uint32_t reloc_raw_size = 0;

    // 0x2000: IMAGE_FILE_DLL
    // only remove reloc section if it's not dll
    if (!(image_file_header->Characteristics & 0x2000))
    {
        // set IMAGE_FILE_RELOCS_STRIPPED
        image_file_header->Characteristics |= 0x0001;

        for (int i = 0; i < image_file_header->NumberOfSections; i++)
        {
            if (section_table[i].VirtualAddress == reloc_virtual_address)
            {
                reloc_raw_offset = section_table[i].PointerToRawData;
                reloc_raw_size = section_table[i].SizeOfRawData;

                // remove Relocation Table in Data Directories
                data_directories[5].VirtualAddress = 0;
                data_directories[5].Size = 0;

                image_file_header->NumberOfSections--;

                // move sections after reloc forward
                memmove(&section_table[i], &section_table[i + 1], sizeof(ImageSectionHeader) * (image_file_header->NumberOfSections - i));

                total_header_size -= sizeof(ImageSectionHeader);
                break;
            }
        }
    }

    uint32_t header_size_aligned = ((total_header_size - 1) & ~(optional_header->FileAlignment - 1)) + optional_header->FileAlignment;
    // has to be multiple of FileAlignment
    size_t pe_size_leanified = 0;
    if (header_size_aligned < optional_header->SizeOfHeaders)
    {
        pe_size_leanified = optional_header->SizeOfHeaders - header_size_aligned;
        optional_header->SizeOfHeaders = header_size_aligned;
    }
    
    // fill the rest of the headers with 0
    // only if PE Header offset has changed
    if (pe_header_offset >= 0x40)
    {
        pe_header_offset = new_pe_header_offset;
        memset(fp - size_leanified + total_header_size, 0, header_size_aligned - total_header_size);
    }


    if (reloc_raw_size)
    {
        // multiple of SectionAlignment
        uint32_t reloc_virtual_size = ((reloc_raw_size - 1) & ~(optional_header->SectionAlignment - 1)) + optional_header->SectionAlignment;

        // decrease SizeOfImage
        optional_header->SizeOfImage -= reloc_virtual_size;

        // set ASLR in DllCharacteristics to false
        // it seems this isn't necessary
        // *(uint16_t *)(optional_header + 0x46) &= ~0x0040;


        // move other section address forward if it is behind reloc
        for (int i = 0; i < image_file_header->NumberOfSections; i++)
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
            section_table[i].PointerToRawData -= pe_size_leanified;
        }

        reloc_raw_offset -= pe_size_leanified;

        // do it with Data Directories too
        for (int i = 0; i < 16; i++)
        {
            if (data_directories[i].VirtualAddress > reloc_virtual_address)
            {
                data_directories[i].VirtualAddress -= reloc_virtual_size;
            }
        }
    }

    fp -= size_leanified;
    size -= pe_size_leanified;
    if (reloc_raw_size)
    {
        // move everything before reloc
        memmove(fp + header_size_aligned, fp + size_leanified + header_size_aligned + pe_size_leanified, reloc_raw_offset - header_size_aligned);

        // move everything after reloc
        size -= reloc_raw_size;
        memmove(fp + reloc_raw_offset, fp + size_leanified + reloc_raw_offset + reloc_raw_size, size - reloc_raw_offset);
    }
    else
    {
        memmove(fp + header_size_aligned, fp + size_leanified + header_size_aligned + pe_size_leanified, size - header_size_aligned);
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


