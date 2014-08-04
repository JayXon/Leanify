#ifndef XML_H
#define XML_H



#include <cstring>

#include "tinyxml2/tinyxml2.h"

#include "format.h"




class Xml : Format
{
public:
    Xml(void *p, size_t s = 0) : Format(p, s), doc(true, tinyxml2::COLLAPSE_WHITESPACE)
    {
        unsigned char *q = (unsigned char *)p;
        while (isspace(*q))
        {
            q++;
        }
        if (*q == '<')
        {
            is_valid = (doc.Parse(fp, size) == 0);
        }
        else
        {
            is_valid = false;
        }
    }

    bool IsValid() const
    {
        return is_valid;
    }

    size_t Leanify(size_t size_leanified = 0);

private:
    bool is_valid;
    tinyxml2::XMLDocument doc;
};


#endif