#ifndef LEANIFY_H
#define LEANIFY_H

#include <iostream>

#include "formats/format.h"
#include "formats/gft.h"
#include "formats/gz.h"
#include "formats/ico.h"
#include "formats/lua.h"
#include "formats/png.h"
#include "formats/rdb.h"
#include "formats/xml.h"
#include "formats/zip.h"


size_t LeanifyFile(void *file_pointer, size_t file_size, size_t size_leanified = 0);


#endif