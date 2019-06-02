#include "xml.h"

#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "../leanify.h"
#include "../utils.h"
#include "base64.h"
#include "data_uri.h"

using std::map;
using std::string;
using std::vector;

Xml::Xml(void* p, size_t s) : Format(p, s) {
  pugi::xml_parse_result result = doc_.load_buffer(
      fp_, size_, pugi::parse_default | pugi::parse_declaration | pugi::parse_doctype | pugi::parse_ws_pcdata_single);
  is_valid_ = result;
  encoding_ = result.encoding;
}

namespace {
// https://github.com/svg/svgo/blob/master/plugins/_collections.js
const map<string, string> kCommonDefaultAttributes = { { "x", "0" },
                                                       { "y", "0" },
                                                       { "clip", "auto" },
                                                       { "clip-path", "none" },
                                                       { "clip-rule", "nonzero" },
                                                       { "mask", "none" },
                                                       { "opacity", "1" },
                                                       { "solid-color", "#000" },
                                                       { "solid-opacity", "1" },
                                                       { "stop-color", "#000" },
                                                       { "stop-opacity", "1" },
                                                       { "fill-opacity", "1" },
                                                       { "fill-rule", "nonzero" },
                                                       { "fill", "#000" },
                                                       { "stroke", "none" },
                                                       { "stroke-width", "1" },
                                                       { "stroke-linecap", "butt" },
                                                       { "stroke-linejoin", "miter" },
                                                       { "stroke-miterlimit", "4" },
                                                       { "stroke-dasharray", "none" },
                                                       { "stroke-dashoffset", "0" },
                                                       { "stroke-opacity", "1" },
                                                       { "paint-order", "normal" },
                                                       { "vector-effect", "none" },
                                                       { "viewport-fill", "none" },
                                                       { "viewport-fill-opacity", "1" },
                                                       { "display", "inline" },
                                                       { "visibility", "visible" },
                                                       { "marker-start", "none" },
                                                       { "marker-mid", "none" },
                                                       { "marker-end", "none" },
                                                       { "color-interpolation", "sRGB" },
                                                       { "color-interpolation-filters", "linearRGB" },
                                                       { "color-rendering", "auto" },
                                                       { "shape-rendering", "auto" },
                                                       { "text-rendering", "auto" },
                                                       { "image-rendering", "auto" },
                                                       { "buffered-rendering", "auto" },
                                                       { "font-style", "normal" },
                                                       { "font-variant", "normal" },
                                                       { "font-weight", "normal" },
                                                       { "font-stretch", "normal" },
                                                       { "font-size", "medium" },
                                                       { "font-size-adjust", "none" },
                                                       { "kerning", "auto" },
                                                       { "letter-spacing", "normal" },
                                                       { "word-spacing", "normal" },
                                                       { "text-decoration", "none" },
                                                       { "text-anchor", "start" },
                                                       { "text-overflow", "clip" },
                                                       { "writing-mode", "lr-tb" },
                                                       { "glyph-orientation-vertical", "auto" },
                                                       { "glyph-orientation-horizontal", "0deg" },
                                                       { "direction", "ltr" },
                                                       { "unicode-bidi", "normal" },
                                                       { "dominant-baseline", "auto" },
                                                       { "alignment-baseline", "baseline" },
                                                       { "baseline-shift", "baseline" },
                                                       { "slope", "1" },
                                                       { "intercept", "0" },
                                                       { "amplitude", "1" },
                                                       { "exponent", "1" },
                                                       { "offset", "0" },
                                                       { "preserveAspectRatio", "xMidYMid" } };

const map<string, map<string, string>> kSingleDefaultAttributes = {
  { "a", { { "target", "_self" } } },
  { "svg",
    { { "width", "100%" },
      { "height", "100%" },
      { "zoomAndPan", "magnify" },
      { "baseProfile", "none" },
      { "contentScriptType", "application/ecmascript" },
      { "contentStyleType", "text/css" } } },
  { "filter",
    { { "primitiveUnits", "userSpaceOnUse" },
      { "x", "-10%" },
      { "y", "-10%" },
      { "width", "120%" },
      { "height", "120%" } } },
  { "mask",
    { { "maskUnits", "objectBoundingBox" },
      { "maskContentUnits", "userSpaceOnUse" },
      { "x", "-10%" },
      { "y", "-10%" },
      { "width", "120%" },
      { "height", "120%" } } }
};

void TraverseElements(pugi::xml_node node, std::function<void(pugi::xml_node)> callback) {
  // cannot use ranged loop here because we need to store the next_sibling before recursion so that if child was removed
  // the loop won't be terminated
  for (pugi::xml_node child = node.first_child(), next; child; child = next) {
    next = child.next_sibling();
    TraverseElements(child, callback);
  }

  callback(node);
}

// Remove single PCData only contains whitespace if xml:space="preserve" is not set.
void RemovePCDataSingle(pugi::xml_node node) {
  if (strcmp(node.attribute("xml:space").value(), "preserve") == 0)
    return;

  // Workaround for some PowerPoint that don't have xml:space="preserve"
  if (strcmp(node.name(), "p:sld") == 0 &&
      strcmp(node.attribute("xmlns:p").value(), "http://schemas.openxmlformats.org/presentationml/2006/main") == 0)
    return;

  pugi::xml_node pcdata = node.first_child();
  if (pcdata.first_child() == nullptr && pcdata.type() == pugi::node_pcdata && pcdata.next_sibling() == nullptr) {
    if (ShrinkSpace(pcdata.value()).empty())
      node.remove_child(pcdata);
    return;
  }

  for (pugi::xml_node child : node.children())
    RemovePCDataSingle(child);
}

// Check the parent and ancestor of the given node if the attribute is an override
bool IsOverride(pugi::xml_node node, const char* name, const string& value) {
  for (pugi::xml_node n = node.parent(); n; n = n.parent()) {
    pugi::xml_attribute n_attr = n.attribute(name);
    // Stop at the first parent that has the same attribute, we can safely remove the attribute in the current node if
    // this parent has the same value, even if there's another parent higher up in the tree which has a different value
    // for this attribute (in which case we won't be able to remove the default attribute in this parent).
    if (!n_attr.empty()) {
      if (n_attr.value() != value)
        return true;
      break;
    }
  }
  return false;
}

// Check if the given attribute is the default
bool IsDefaultAttribute(const map<string, string>* single_default_attrs, const string& name, const string& value) {
  if (single_default_attrs) {
    auto it = single_default_attrs->find(name);
    if (it != single_default_attrs->end())
      return it->second == value;
  }
  // Fallback to common
  auto it = kCommonDefaultAttributes.find(name);
  return it != kCommonDefaultAttributes.end() && it->second == value;
}

struct xml_vector_writer : pugi::xml_writer {
  vector<uint8_t> buf;

  void write(const void* data, size_t size) override {
    auto p = static_cast<const uint8_t*>(data);
    buf.insert(buf.end(), p, p + size);
  }
};
}  // namespace

size_t Xml::Leanify(size_t size_leanified /*= 0*/) {
  RemovePCDataSingle(doc_);

  // if the XML is fb2 file
  if (doc_.child("FictionBook")) {
    VerbosePrint("FB2 detected.");
    if (depth < max_depth) {
      depth++;

      pugi::xml_node root = doc_.child("FictionBook");

      // iterate through all binary element
      for (pugi::xml_node binary = root.child("binary"), next; binary; binary = next) {
        next = binary.next_sibling("binary");
        pugi::xml_attribute id = binary.attribute("id");
        if (id == nullptr || id.value() == nullptr || id.value()[0] == 0) {
          root.remove_child(binary);
          continue;
        }

        PrintFileName(id.value());

        const char* base64_data = binary.child_value();
        if (base64_data == nullptr || base64_data[0] == 0) {
          VerbosePrint("No data found.");
          continue;
        }
        size_t base64_len = strlen(base64_data);
        // copy to a new location because base64_data is const
        vector<char> new_base64_data(base64_data, base64_data + base64_len + 1);

        size_t new_base64_len = Base64(new_base64_data.data(), base64_len).Leanify();

        if (new_base64_len < base64_len) {
          new_base64_data[new_base64_len] = 0;
          binary.text() = new_base64_data.data();
        }
      }
      depth--;
    }
  } else if (doc_.child("svg")) {
    VerbosePrint("SVG detected.");

    // remove XML declaration and doctype
    for (pugi::xml_node child = doc_.first_child(), next; child; child = next) {
      next = child.next_sibling();

      if (child.type() == pugi::node_declaration || child.type() == pugi::node_doctype)
        doc_.remove_child(child);
    }

    TraverseElements(doc_.child("svg"), [](pugi::xml_node node) {
      auto single_default_attrs_iter = kSingleDefaultAttributes.find(node.name());
      const map<string, string>* single_default_attrs = nullptr;
      if (single_default_attrs_iter != kSingleDefaultAttributes.end())
        single_default_attrs = &single_default_attrs_iter->second;

      for (pugi::xml_attribute attr = node.first_attribute(), next; attr; attr = next) {
        next = attr.next_attribute();

        string value = ShrinkSpace(attr.value());
        // Remove empty attribute
        if (value.empty()) {
          node.remove_attribute(attr);
          continue;
        }

        // the second parameter in preserveAspectRatio meet by default
        if (value.size() > 5 && strcmp(attr.name(), "preserveAspectRatio") == 0 &&
            value.substr(value.size() - 5) == " meet")
          value.resize(value.size() - 5);

        // Remove default attribute
        if (IsDefaultAttribute(single_default_attrs, attr.name(), value)) {
          // Only remove it if it's not an override of a parent non-default attribute
          if (!IsOverride(node, attr.name(), value)) {
            node.remove_attribute(attr);
            continue;
          }
        }

        const string kDataURIMagic = "data:";
        if ((strcmp(attr.name(), "href") == 0 || strcmp(attr.name(), "xlink:href") == 0) &&
            value.size() > kDataURIMagic.size() &&
            memcmp(value.data(), kDataURIMagic.data(), kDataURIMagic.size()) == 0) {
          DataURI data_uri(&value[0], value.size());
          data_uri.SetSingleMode(true);
          size_t new_size = data_uri.Leanify();
          value.resize(new_size);
        }

        attr = value.c_str();
      }

      // remove empty text element and container element
      const char* name = node.name();
      if (node.first_child() == nullptr) {
        if (strcmp(name, "text") == 0 || strcmp(name, "tspan") == 0 || strcmp(name, "a") == 0 ||
            strcmp(name, "defs") == 0 || strcmp(name, "g") == 0 || strcmp(name, "marker") == 0 ||
            strcmp(name, "mask") == 0 || strcmp(name, "missing-glyph") == 0 || strcmp(name, "pattern") == 0 ||
            strcmp(name, "switch") == 0 || strcmp(name, "symbol") == 0) {
          node.parent().remove_child(node);
          return;
        }
      }

      if (strcmp(name, "tref") == 0 && node.attribute("xlink:href").empty()) {
        node.parent().remove_child(node);
        return;
      }

      if (strcmp(name, "metadata") == 0) {
        node.parent().remove_child(node);
        return;
      }
    });
  }

  // print leanified XML to memory
  xml_vector_writer writer;
  doc_.save(writer, nullptr, pugi::format_raw | pugi::format_no_declaration, encoding_);
  fp_ -= size_leanified;
  if (writer.buf.size() < size_) {
    size_ = writer.buf.size();
    memcpy(fp_, writer.buf.data(), size_);
  } else {
    memmove(fp_, fp_ + size_leanified, size_);
  }
  return size_;
}
