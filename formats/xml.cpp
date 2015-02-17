#include "xml.h"

#include <cstring>
#include <iostream>
#include <string>

#include "../leanify.h"
#include "../utils.h"



Xml::Xml(void *p, size_t s /*= 0*/) : Format(p, s), doc(true)
{
    unsigned char *q = (unsigned char *)p;

    const unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };

    // tinyxml2 does not support utf16
    /*
    const unsigned char utf16be_bom[] = { 0xFE, 0xFF };
    const unsigned char utf16le_bom[] = { 0xFF, 0xFE };
    */
    // skip utf8 bom
    if (memcmp(q, utf8_bom, sizeof(utf8_bom)) == 0)
    {
        q += sizeof(utf8_bom);
    }
    /*      else if (!memcmp(q, utf16le_bom, sizeof(utf16le_bom)) || !memcmp(q, utf16be_bom, sizeof(utf16be_bom)))
    {
    q += sizeof(utf16le_bom);
    }
    */
    // skip spaces
    while (isspace(*q) && q < (unsigned char *)p + s)
    {
        q++;
    }
    // only parse the file if it starts with '<'
    if (*q == '<')
    {
        is_valid = (doc.Parse(fp, size) == 0);
    }
    else
    {
        is_valid = false;
    }
}




size_t Xml::Leanify(size_t size_leanified /*= 0*/)
{
    if (doc.RootElement())
    {
        const char *root_name = doc.RootElement()->Name();
        // if the XML is fb2 file
        if (strcmp(root_name, "FictionBook") == 0)
        {
            if (is_verbose)
            {
                std::cout << "FB2 detected." << std::endl;
            }
            if (depth < max_depth)
            {
                depth++;

                // iterate through all binary element
                for (auto e = doc.RootElement()->FirstChildElement("binary"); e; e = e->NextSiblingElement("binary"))
                {
                    const char *id = e->Attribute("id");
                    if (id == nullptr || *id == 0)
                    {
                        doc.RootElement()->DeleteChild(e);
                        continue;
                    }

                    for (int i = 1; i < depth; i++)
                    {
                        std::cout << "-> ";
                    }
                    std::cout << id << std::endl;

                    const char *base64_data = e->GetText();
                    if (base64_data == nullptr)
                    {
                        std::cout << "No data found." << std::endl;
                        continue;
                    }
                    size_t base64_len = strlen(base64_data);

                    // 4 base64 character contains information of 3 bytes
                    size_t binary_len = 3 * base64_len / 4;
                    unsigned char *binary_data = new unsigned char[binary_len];

                    if (Base64Decode(base64_data, base64_len, binary_data, &binary_len))
                    {
                        std::cout << "Base64 decode error." << std::endl;
                        delete[] binary_data;
                        continue;
                    }

                    // Leanify embedded image
                    binary_len = LeanifyFile(binary_data, binary_len);

                    // allocate a few more bytes for padding
                    size_t new_base64_len = 4 * binary_len / 3 + 4;
                    char *new_base64_data = new char[new_base64_len];

                    if (Base64Encode(binary_data, binary_len, new_base64_data, new_base64_len))
                    {
                        std::cout << "Base64 encode error." << std::endl;
                    }
                    else if (strlen(new_base64_data) < base64_len)
                    {
                        e->SetText(new_base64_data);
                    }

                    delete[] binary_data;
                    delete[] new_base64_data;
                }
                depth--;
            }
        }
        else if (strcmp(root_name, "svg") == 0)
        {
            if (is_verbose)
            {
                std::cout << "SVG detected." << std::endl;
            }

            // remove XML declaration and doctype
            for (auto child = doc.FirstChild(); child; child = child->NextSibling())
            {
                if (child->ToDeclaration() || child->ToUnknown())
                {
                    doc.DeleteChild(child);
                }
            }

            TraverseElements(doc.RootElement(), [](tinyxml2::XMLElement *e)
            {
                for (auto attr = e->FirstAttribute(); attr; attr = attr->Next())
                {
                    auto value = attr->Value();
                    // remove empty attribute
                    if (value == nullptr || *value == 0)
                    {
                        e->DeleteAttribute(attr->Name());
                        continue;
                    }

                    // shrink spaces and newlines in attribute
                    std::string s(value);
                    size_t ssize = 0;
                    for (size_t i = 0; i < s.size(); i++)
                    {
                        if (s[i] == ' ' || s[i] == '\n' || s[i] == '\t')
                        {
                            do
                            {
                                i++;
                            }
                            while (s[i] == ' ' || s[i] == '\n' || s[i] == '\t');
                            s[ssize++] = ' ';
                        }
                        s[ssize++] = s[i];
                    }
                    if (ssize && s[ssize - 1] == ' ')
                    {
                        ssize--;
                    }
                    s.resize(ssize);
                    e->SetAttribute(attr->Name(), s.c_str());
                }

                // remove empty text element and container element
                auto name = e->Name();
                if (e->NoChildren())
                {
                    if (strcmp(name, "text") == 0 ||
                        strcmp(name, "tspan") == 0 ||
                        strcmp(name, "a") == 0 ||
                        strcmp(name, "defs") == 0 ||
                        strcmp(name, "g") == 0 ||
                        strcmp(name, "marker") == 0 ||
                        strcmp(name, "mask") == 0 ||
                        strcmp(name, "missing-glyph") == 0 ||
                        strcmp(name, "pattern") == 0 ||
                        strcmp(name, "switch") == 0 ||
                        strcmp(name, "symbol") == 0)
                    {
                        e->Parent()->DeleteChild(e);
                        return;
                    }
                }

                if (strcmp(name, "tref") == 0 && e->Attribute("xlink:href") == nullptr)
                {
                    e->Parent()->DeleteChild(e);
                    return;
                }

                if (strcmp(name, "metadata") == 0)
                {
                    e->Parent()->DeleteChild(e);
                    return;
                }
            });
        }
    }

    // print leanified XML to memory
    tinyxml2::XMLPrinter printer(0, true);
    doc.Print(&printer);

    size_t new_size = printer.CStrSize() - 1;     // -1 for the \0
    fp -= size_leanified;
    if (new_size < size)
    {
        memcpy(fp, printer.CStr(), new_size);
        return new_size;
    }
    else if (size_leanified)
    {
        memmove(fp, fp + size_leanified, size);
    }
    return size;
}

void Xml::TraverseElements(tinyxml2::XMLElement *ele, std::function<void(tinyxml2::XMLElement *)> callback)
{
    for (auto e = ele->FirstChildElement(); e; e = e->NextSiblingElement())
    {
        TraverseElements(e, callback);
    }

    callback(ele);
}


