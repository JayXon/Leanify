#ifndef XML_H
#define XML_H


#include <functional>

#include "../lib/tinyxml2/tinyxml2.h"

#include "format.h"


extern bool is_verbose;


class Xml : public Format
{
public:
    Xml(void *p, size_t s = 0);

    bool IsValid() const
    {
        return is_valid;
    }

    size_t Leanify(size_t size_leanified = 0);

private:
    bool is_valid;
    tinyxml2::XMLDocument doc;

    void TraverseElements(tinyxml2::XMLElement *e, std::function<void(tinyxml2::XMLElement *)> callback);
};


#endif