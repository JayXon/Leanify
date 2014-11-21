#ifndef LEANIFY_H
#define LEANIFY_H

#include <cstddef>

extern int depth;
extern int max_depth;

size_t LeanifyFile(void *file_pointer, size_t file_size, size_t size_leanified = 0);

size_t ZlibRecompress(unsigned char *src, size_t src_len, size_t size_leanified = 0);


#endif