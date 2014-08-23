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
    uint32_t rsrc_virtual_address = data_directories[2].VirtualAddress;

    uint32_t reloc_raw_offset = 0;
    uint32_t reloc_raw_size = 0;
    uint32_t rsrc_raw_offset = 0;
    uint32_t rsrc_raw_size = 0;
    uint32_t rsrc_virtual_size = 0;

    // 0x2000: IMAGE_FILE_DLL
    // only remove reloc section if it's not dll
    if (!(image_file_header->Characteristics & 0x2000))
    {
        // set IMAGE_FILE_RELOCS_STRIPPED
        image_file_header->Characteristics |= 0x0001;
    }

    // Some PE have wrong SizeOfHeaders, e.g. files packed by UPX
    // I have to find the smallest PointerToRawData in all sections
    // If that is smaller than SizeOfHeaders, then that's the correct SizeOfHeaders
    uint32_t correct_size_of_headers = optional_header->SizeOfHeaders;

    // looking for reloc and rsrc in Section Table
    for (int i = 0; i < image_file_header->NumberOfSections; i++)
    {
        if (section_table[i].VirtualAddress == reloc_virtual_address)
        {
            // if IMAGE_FILE_RELOCS_STRIPPED is set
            if (image_file_header->Characteristics & 0x0001)
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
                i--;
            }
        }
        else if (section_table[i].VirtualAddress == rsrc_virtual_address)
        {
            rsrc_raw_offset = section_table[i].PointerToRawData;
            rsrc_raw_size = section_table[i].SizeOfRawData;
            rsrc_virtual_size = section_table[i].VirtualSize;
        }
        if (section_table[i].PointerToRawData < correct_size_of_headers)
        {
            correct_size_of_headers = section_table[i].PointerToRawData;
        }
    }

    uint32_t header_size_aligned = ((total_header_size - 1) | (optional_header->FileAlignment - 1)) + 1;
    // has to be multiple of FileAlignment
    size_t pe_size_leanified = 0;
    size_t header_size_leanified = 0;

    if (header_size_aligned < correct_size_of_headers)
    {
        header_size_leanified = pe_size_leanified = correct_size_of_headers - header_size_aligned;
        optional_header->SizeOfHeaders = header_size_aligned;
    }

    // fill the rest of the headers with 0
    // only if PE Header offset has changed
    if (pe_header_offset >= 0x40)
    {
        pe_header_offset = new_pe_header_offset;
        memset(fp - size_leanified + total_header_size, 0, header_size_aligned - total_header_size);
    }

    // multiple of SectionAlignment
    uint32_t reloc_virtual_size = ((reloc_raw_size - 1) | (optional_header->SectionAlignment - 1)) + 1;

    // set ASLR in DllCharacteristics to false
    // it seems this isn't necessary
    // *(uint16_t *)(optional_header + 0x46) &= ~0x0040;

    uint32_t reloc_end = reloc_raw_offset + reloc_raw_size;
    size_t rsrc_size_leanified = 0;
    if (rsrc_raw_size)
    {
        // decrease RVA inside rsrc section
        if (rsrc_virtual_address > reloc_virtual_address)
        {
            TraverseRSRC(fp + rsrc_raw_offset, (ImageResourceDirectory *)(fp + rsrc_raw_offset), reloc_virtual_size);
            rsrc_virtual_address -= reloc_virtual_size;
        }
        else
        {
            // only save all the data addresses to vector
            TraverseRSRC(fp + rsrc_raw_offset, (ImageResourceDirectory *)(fp + rsrc_raw_offset));
        }

        // sort it according to it's data offset
        std::sort(rsrc_data.begin(), rsrc_data.end(), [](const uint32_t *a, const uint32_t *b){ return *a < *b; });
        uint32_t last_end = rsrc_data[0][0];

        // detect non standard resource, maybe produced by some packer
        if (last_end < rsrc_virtual_address || last_end > rsrc_virtual_address + rsrc_virtual_size)
        {
            if (is_verbose)
            {
                std::cout << "Non standard resource detected." << std::endl;
            }
            if (reloc_raw_size)
            {
                // move everything before reloc
                memmove(fp - size_leanified + header_size_aligned, fp + header_size_aligned + pe_size_leanified, reloc_raw_offset - header_size_aligned);

                // move everything after reloc
                memmove(fp - size_leanified + reloc_raw_offset - pe_size_leanified, fp + reloc_end, size - reloc_raw_offset);
                pe_size_leanified += reloc_raw_size;
            }
            else
            {
                memmove(fp - size_leanified + header_size_aligned, fp + header_size_aligned + pe_size_leanified, size - header_size_aligned - pe_size_leanified);
            }
        }
        else
        {

            if (reloc_raw_size && reloc_raw_offset < rsrc_raw_offset)
            {
                // move everything before reloc
                memmove(fp - size_leanified + header_size_aligned, fp + header_size_aligned + pe_size_leanified, reloc_raw_offset - header_size_aligned - pe_size_leanified);

                // move things between reloc and first data of rsrc
                memmove(fp - size_leanified + reloc_raw_offset - pe_size_leanified, fp + reloc_end, rsrc_raw_offset - reloc_end + rsrc_data[0][0] - rsrc_virtual_address);
                pe_size_leanified += reloc_raw_size;

            }
            else
            {
                // move everything before first data of rsrc
                memmove(fp - size_leanified + header_size_aligned, fp + header_size_aligned + pe_size_leanified, rsrc_raw_offset - pe_size_leanified - header_size_aligned + rsrc_data[0][0] - rsrc_virtual_address);
            }

            // p[0] is RVA, p[1] is size
            for (auto &p : rsrc_data)
            {
                p -= pe_size_leanified / 4;
                size_t new_size = LeanifyFile(fp + rsrc_raw_offset + p[0] - rsrc_virtual_address, p[1], p[0] - last_end + pe_size_leanified);
                p[0] = last_end;
                rsrc_size_leanified += p[1] - new_size;
                p[1] = new_size;
                // it seems some of the resource has to be aligned to 8 in order to work
                last_end += (new_size + 7) & ~7;
            }

            uint32_t rsrc_new_end = rsrc_raw_offset + *rsrc_data.back() - rsrc_virtual_address + *(rsrc_data.back() + 1);
            uint32_t rsrc_new_end_aligned = ((rsrc_new_end - 1) | (optional_header->FileAlignment - 1)) + 1;
            uint32_t rsrc_correct_end_aligned = ((rsrc_new_end + rsrc_size_leanified - 1) | (optional_header->FileAlignment - 1)) + 1;
            uint32_t rsrc_end = rsrc_raw_offset + rsrc_raw_size;

            // fill the rest of rsrc with 0
            memset(fp - size_leanified + rsrc_new_end - pe_size_leanified, 0, rsrc_size_leanified);

            // if rsrc_correct_end_aligned != rsrc_end then there are extra data in rsrc section
            if (rsrc_new_end_aligned <= rsrc_end && rsrc_correct_end_aligned == rsrc_end)
            {
                memmove(fp - size_leanified + rsrc_new_end + rsrc_size_leanified - pe_size_leanified, fp + rsrc_new_end + rsrc_size_leanified, rsrc_new_end_aligned - rsrc_new_end - rsrc_size_leanified);
                rsrc_virtual_size = rsrc_new_end - rsrc_raw_offset;
                rsrc_size_leanified = rsrc_raw_size - (rsrc_new_end_aligned - rsrc_raw_offset);
                pe_size_leanified += rsrc_size_leanified;
                rsrc_raw_size = rsrc_new_end_aligned - rsrc_raw_offset;
            }
            else
            {
                memmove(fp - size_leanified + rsrc_new_end + rsrc_size_leanified - pe_size_leanified, fp + rsrc_new_end + rsrc_size_leanified, rsrc_end - rsrc_new_end - rsrc_size_leanified);
                // this is necessary because some SizeOfRawData is not aligned
                rsrc_new_end_aligned = rsrc_raw_offset + rsrc_raw_size;
                rsrc_size_leanified = 0;
            }
            if (reloc_raw_size && reloc_raw_offset > rsrc_raw_offset)
            {
                memmove(fp - size_leanified + rsrc_end - pe_size_leanified, fp + rsrc_end, reloc_raw_offset - rsrc_end);
                memmove(fp - size_leanified + reloc_raw_offset - pe_size_leanified, fp + reloc_end, size - reloc_end);
                pe_size_leanified += reloc_raw_size;
            }
            else
            {
                memmove(fp - size_leanified + rsrc_end - pe_size_leanified, fp + rsrc_end, size - rsrc_end);
            }
        }

    }
    else if (reloc_raw_size)
    {

        // move everything before reloc
        memmove(fp - size_leanified + header_size_aligned, fp + header_size_aligned + pe_size_leanified, reloc_raw_offset - header_size_aligned);

        // move everything after reloc
        memmove(fp - size_leanified + reloc_raw_offset - pe_size_leanified, fp + reloc_end, size - reloc_raw_offset);
        pe_size_leanified += reloc_raw_size;
    }
    else
    {
        memmove(fp - size_leanified + header_size_aligned, fp + header_size_aligned + pe_size_leanified, size - header_size_aligned - pe_size_leanified);
    }


    // decrease SizeOfImage
    int rsrc_decrease_size = ((data_directories[2].Size - 1) | (optional_header->SectionAlignment - 1)) - ((rsrc_virtual_size - 1) | (optional_header->SectionAlignment - 1));
    if (rsrc_decrease_size < 0)
    {
        rsrc_decrease_size = 0;
    }
    optional_header->SizeOfImage -= reloc_virtual_size + rsrc_decrease_size;

    // update Section Table
    for (int i = 0; i < image_file_header->NumberOfSections; i++)
    {
        if (section_table[i].VirtualAddress > reloc_virtual_address)
        {
            section_table[i].VirtualAddress -= reloc_virtual_size;
        }
        if (section_table[i].VirtualAddress == rsrc_virtual_address)
        {
            section_table[i].SizeOfRawData = rsrc_raw_size;
            section_table[i].VirtualSize = rsrc_virtual_size;
        }
        if (section_table[i].PointerToRawData > reloc_raw_offset)
        {
            section_table[i].PointerToRawData -= reloc_raw_size;
        }
        if (section_table[i].PointerToRawData > rsrc_raw_offset)
        {
            section_table[i].PointerToRawData -= rsrc_size_leanified;
        }
        section_table[i].PointerToRawData -= header_size_leanified;
    }

    // this can work on both PE32 and PE32+
    int num_data_dir = *(uint32_t *)((char *)data_directories - 4);
    // update Data Directories too
    for (int i = 0; i < num_data_dir; i++)
    {
        if (data_directories[i].VirtualAddress > reloc_virtual_address)
        {
            data_directories[i].VirtualAddress -= reloc_virtual_size;
        }
    }

    // the size in Data Directory can not larger than VirtualSize in Section Table
    // but for some of the packed file, it will be much smaller
    // so only update size in Data Directory if exe is not packed by these kind of packer
    if (data_directories[2].Size > rsrc_virtual_size)
    {
        data_directories[2].Size = rsrc_virtual_size;
    }

    fp -= size_leanified;
    size -= pe_size_leanified;

    return size;
}


// decrease RVA inside rsrc section
void Pe::TraverseRSRC(char *rsrc, ImageResourceDirectory *res_dir, uint32_t move_size /*= 0*/)
{
    ImageResourceDirectoryEntry *entry = (ImageResourceDirectoryEntry *)((char *)res_dir + sizeof(ImageResourceDirectory));
    for (int i = 0; i < res_dir->NumberOfNamedEntries + res_dir->NumberOfIdEntries; i++)
    {
        if (entry[i].DataIsDirectory)
        {
            TraverseRSRC(rsrc, (ImageResourceDirectory *)(rsrc + entry[i].OffsetToDirectory), move_size);
        }
        else
        {
            *(uint32_t *)(rsrc + entry[i].OffsetToData) -= move_size;
            // remember the address to Leanify resource file later
            rsrc_data.push_back((uint32_t *)(rsrc + entry[i].OffsetToData));
        }
    }
}


