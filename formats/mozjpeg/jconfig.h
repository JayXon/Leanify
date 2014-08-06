#define JPEG_LIB_VERSION  80

/* libjpeg-turbo version */
#define LIBJPEG_TURBO_VERSION 0

/* Support in-memory source/destination managers */
#define MEM_SRCDST_SUPPORTED

#define C_ARITH_CODING_SUPPORTED
#define D_ARITH_CODING_SUPPORTED

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

