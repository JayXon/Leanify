#ifndef MAIN_H_
#define MAIN_H_

#ifdef _WIN32
#define STRTOL wcstol
#define STRCMP(X, Y) wcscmp(X, L##Y)
#else
#define STRTOL strtol
#define STRCMP strcmp
#endif  // _WIN32

bool is_fast;
bool is_verbose;
#ifdef _WIN32
bool is_pause;
#endif  // _WIN32

// iteration of zopfli
int iterations;

// a normal file: depth 1
// file inside zip that is inside another zip: depth 3
int depth;
int max_depth;
bool parallel_processing = false;


#endif  // MAIN_H_
