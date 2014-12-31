#define JPEG_LIB_VERSION  80

/* libjpeg-turbo version */
#define LIBJPEG_TURBO_VERSION 0

/* Support in-memory source/destination managers */
#define MEM_SRCDST_SUPPORTED

#define C_ARITH_CODING_SUPPORTED
#define D_ARITH_CODING_SUPPORTED

/*
 * Define BITS_IN_JSAMPLE as either
 *   8   for 8-bit sample values (the usual setting)
 *   12  for 12-bit sample values
 * Only 8 and 12 are legal data precisions for lossy JPEG according to the
 * JPEG standard, and the IJG code does not support anything else!
 * We do not support run-time selection of data precision, sorry.
 */

#define BITS_IN_JSAMPLE  8      /* use 8 or 12 */

/* Does your compiler support function prototypes?
 * (If not, you also need to use ansi2knr, see install.txt)
 */
#define HAVE_PROTOTYPES
 
/* Compiler supports 'unsigned char'. */
#define HAVE_UNSIGNED_CHAR

/* Compiler supports 'unsigned short'. */
#define HAVE_UNSIGNED_SHORT

/* Define this if your system has an ANSI-conforming <stddef.h> file.
 */
#define HAVE_STDDEF_H

/* Define this if your system has an ANSI-conforming <stdlib.h> file.
 */
#define HAVE_STDLIB_H

/* Define "boolean" as unsigned char, not int, on Windows systems.
 */
#ifdef _WIN32
#ifndef __RPCNDR_H__        /* don't conflict if rpcndr.h already read */
typedef unsigned char boolean;
#endif
#define HAVE_BOOLEAN        /* prevent jmorecfg.h from redefining it */
#endif

