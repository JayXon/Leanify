#ifndef MAIN_H
#define MAIN_H


#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cmath>

#include "leanify.h"
#include "fileio.h"


#define VERSION "0.4.2"


bool is_fast;
bool is_verbose;

// iteration of zopfli
int iterations;

// file inside zip: level 1
// file inside zip that is inside another zip: level 2
// used to print "->"
int level;





#endif