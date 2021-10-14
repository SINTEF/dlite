/* integers.h - cross-platform header with typedefs for fix-sized integers
 *
 * Copyright (C) 2017 SINTEF Materials and Chemistry
 * By Jesper Friis <jesper.friis@sintef.no>
 *
 * Distributed under terms of the MIT license.
 */

/**
 * @file integers.h
 * @brief Cross-platform header with typedefs for fix-sized integers.
 *
 * Provided typedefs for the following tytes:
 *
 *     int8_t
 *     int16_t
 *     int32_t
 *     uint8_t
 *     uint16_t
 *     uint32_t
 *
 * On platforms that support 64 bit integers, HAVE_INT64 will be set and
 * the following types are also provided:
 *
 *     int64_t
 *     uint64_t
 */

#ifndef _INTEGERS_H
#define _INTEGERS_H

/** @cond SKIP */

#ifndef HAVE_STDINT_H
#define HAVE_STDINT_H
#endif

#define HAVE_UINT8_T
#define HAVE_UINT16_T
#define HAVE_UINT32_T
#define HAVE_UINT64_T

/* #undef HAVE_LONG_LONG */

#define SIZEOF_CHAR       1
#define SIZEOF_SHORT      2
#define SIZEOF_INT        4
#define SIZEOF_LONG       4
#define SIZEOF_LONG_LONG  8

#include <stdlib.h>
#ifdef HAVE_STDINT_H
# include <stdint.h>
# define HAVE_INT64
#else

#ifndef HAVE_UINT8_T
# if SIZEOF_CHAR == 1
typedef signed char     int8_t;
typedef unsigned char   uint8_t;
# else
#  error char is not 8 bits
# endif
# define HAVE_UINT8_T
#endif

#ifndef HAVE_UINT16_T
# if SIZEOF_SHORT == 2
typedef signed short    int16_t;
typedef unsigned short  uint16_t;
# elif SIZEOF_INT == 2
typedef signed int      int16_t;
typedef unsigned int    uint16_t;
# else
#  error neither short or int are 16 bits
# endif
# define HAVE_UINT16_T
#endif

#ifndef HAVE_UINT32_T
# if SIZEOF_INT == 4
typedef signed int      int32_t;
typedef unsigned int    uint32_t;
# elif SIZEOF_LONG == 4
typedef signed long     int32_t;
typedef unsigned long   uint32_t;
# else
#  error neither int or long are 32 bits
# endif
# define HAVE_UINT32_T
#endif

#ifndef HAVE_UINT64_T
# if SIZEOF_LONG == 8
#  define HAVE_INT64
typedef signed long         int64_t;
typedef unsigned long       uint64_t;
# elif SIZEOF_LONG_LONG == 8
#  define HAVE_INT64
typedef signed long long    int64_t;
typedef unsigned long long  uint64_t;
# endif
#endif

#endif /* HAVE_STDINT_H */

/** @endcond */

#endif /*  _INTEGERS_H */
