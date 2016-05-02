#ifndef LEANIFY_H_
#define LEANIFY_H_

#include <cstddef>
#include <string>

extern int depth;
extern int max_depth;

size_t LeanifyFile(void *file_pointer, size_t file_size, size_t size_leanified = 0, const std::string& filename = "");

size_t ZlibRecompress(void *src, size_t src_len, size_t size_leanified = 0);

#endif  // LEANIFY_H_
