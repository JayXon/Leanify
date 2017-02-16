/* libjpeg-turbo build number */
#define BUILD

/* How to obtain function inlining. */
#ifndef INLINE
#if defined(__GNUC__)
#define INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define INLINE __forceinline
#else
#define INLINE
#endif
#endif
/* Define to the full name of this package. */
#define PACKAGE_NAME

/* Version number of package */
#define VERSION