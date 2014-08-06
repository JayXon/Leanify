#include "xml.h"



Xml::Xml(void *p, size_t s /*= 0*/) : Format(p, s), doc(true, tinyxml2::COLLAPSE_WHITESPACE)
{
    unsigned char *q = (unsigned char *)p;

    const unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };

    // tinyxml2 does not support utf16
    /*
    const unsigned char utf16be_bom[] = { 0xFE, 0xFF };
    const unsigned char utf16le_bom[] = { 0xFF, 0xFE };
    */
    // skip utf8 bom
    if (!memcmp(q, utf8_bom, sizeof(utf8_bom)))
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
