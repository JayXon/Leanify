#ifndef LEANIFY_H
#define LEANIFY_H

#include <iostream>

#include "formats/zopfli/zlib_container.h"

#include "formats/format.h"
#include "formats/gft.h"
#include "formats/gz.h"
#include "formats/ico.h"
#include "formats/jpeg.h"
#include "formats/lua.h"
#include "formats/pe.h"
#include "formats/png.h"
#include "formats/swf.h"
#include "formats/tar.h"
#include "formats/rdb.h"
#include "formats/xml.h"
#include "formats/zip.h"

extern int depth;
extern int max_depth;

size_t LeanifyFile(void *file_pointer, size_t file_size, size_t size_leanified = 0);

size_t ZlibRecompress(unsigned char *src, size_t src_len, size_t size_leanified = 0);


#endif