#ifndef MAIN_H
#define MAIN_H


#include <iostream>
#include <iomanip>

#include "leanify.h"
#include "fileio.h"
#include "version.h"



bool is_fast;
bool is_verbose;

// iteration of zopfli
int iterations;

// a normal file: depth 1
// file inside zip that is inside another zip: depth 3
int depth;
int max_depth;




#endif