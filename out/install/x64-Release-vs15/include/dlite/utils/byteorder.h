/* byteorder.h - cross-platform header for byte ordering
 *
 * Copyright (C) 2017 SINTEF Materials and Chemistry
 * By Jesper Friis <jesper.friis@sintef.no>
 *
 * Distributed under terms of the MIT license.
 */

/**
 * @file byteorder.h
 * @brief Cross-platform header for byte ordering.
 *
 * Provided functions for reversing the byte-order:
 *
 *     uint16_t bswap_16(uint16_t x);
 *     uint32_t bswap_32(uint32_t x);
 *     uint64_t bswap_64(uint64_t x);
 *
 * Provided functions for byte order convertion:
 *
 *     uint16_t htobe16(uint16_t host_16bits);
 *     uint16_t htole16(uint16_t host_16bits);
 *     uint16_t be16toh(uint16_t big_endian_16bits);
 *     uint16_t le16toh(uint16_t little_endian_16bits);
 *
 *     uint32_t htobe32(uint32_t host_32bits);
 *     uint32_t htole32(uint32_t host_32bits);
 *     uint32_t be32toh(uint32_t big_endian_32bits);
 *     uint32_t le32toh(uint32_t little_endian_32bits);
 *
 *     uint64_t htobe64(uint64_t host_64bits);
 *     uint64_t htole64(uint64_t host_64bits);
 *     uint64_t be64toh(uint64_t big_endian_64bits);
 *     uint64_t le64toh(uint64_t little_endian_64bits);
 */

#ifndef _BYTEORDER_H
#define _BYTEORDER_H

/** @cond SKIP */

/* The following macros should be defined in CMakeLists.txt: */
#ifndef HAVE_STDINT_H
#define HAVE_STDINT_H        /* Whether stdint.h exists */
#endif
/* #undef HAVE_ENDIAN_H */
/* #undef HAVE_BYTESWAP_H */

/* #undef IS_BIG_ENDIAN */

/* #undef HAVE_BSWAP_16 */
/* #undef HAVE_BSWAP_32 */
/* #undef HAVE_BSWAP_64 */

/* #undef HAVE_BYTESWAP_USHORT */
/* #undef HAVE_BYTESWAP_ULONG */
/* #undef HAVE_BYTESWAP_UINT64 */

/* #undef HAVE_HTOBE16 */
/* #undef HAVE_HTOBE32 */
/* #undef HAVE_HTOBE64 */


#ifdef HAVE_STDINT_H
# include <stdint.h>
#else
# include "integers.h"
#endif

#ifdef HAVE_ENDIAN_H
# include <endian.h>
#endif

#ifdef HAVE_BYTESWAP_H
# include <byteswap.h>
#endif


/* Byte order of target machine */
#ifndef LITTLE_ENDIAN
# define LITTLE_ENDIAN 1234
#endif

#ifndef BIG_ENDIAN
# define BIG_ENDIAN 4321
#endif

#ifndef BYTE_ORDER
# ifdef IS_BIG_ENDIAN
#  define BYTE_ORDER BIG_ENDIAN
# else
#  define BYTE_ORDER LITTLE_ENDIAN
# endif
#endif

#ifndef HAVE_BSWAP_16
# ifdef HAVE__BYTESWAP_USHORT
#  include <stdlib.h>
#  define bswap_16(x) _byteswap_ushort(x)
# else
#  define bswap_16(value) \
  ((((value) & 0xff) << 8) | ((value) >> 8))
# endif
#endif

#ifndef HAVE_BSWAP_32
# ifdef HAVE__BYTESWAP_ULONG
#  include <stdlib.h>
#  define bswap_32(x) _byteswap_ulong(x)
# else
#  define bswap_32(value)                                     \
  (((uint32_t)bswap_16((uint16_t)((value) & 0xffff)) << 16) | \
   (uint32_t)bswap_16((uint16_t)((value) >> 16)))
# endif
#endif

#ifndef HAVE_BSWAP_64
# ifdef HAVE__BYTESWAP_UINT64
#  include <stdlib.h>
#  define bswap_64(x) _byteswap_uint64(x)
# else
#  define bswap_64(value)                                         \
  (((uint64_t)bswap_32((uint32_t)((value) & 0xffffffff)) << 32) | \
   (uint64_t)bswap_32((uint32_t)((value) >> 32)))
# endif
#endif


/* Byte order conversion */
#ifndef HAVE_HTOBE16
# if BYTE_ORDER == LITTLE_ENDIAN
#  define htobe16(x) bswap_16(x)
#  define htole16(x) (x)
#  define be16toh(x) bswap_16(x)
#  define le16toh(x) (x)
# elif BYTE_ORDER == BIG_ENDIAN
#  define htobe16(x) (x)
#  define htole16(x) bswap_16(x)
#  define be16toh(x) (x)
#  define le16toh(x) bswap_16(x)
# else
#  error Byte order not supported
# endif
#endif

#ifndef HAVE_HTOBE32
# if BYTE_ORDER == LITTLE_ENDIAN
#  define htobe32(x) bswap_32(x)
#  define htole32(x) (x)
#  define be32toh(x) bswap_32(x)
#  define le32toh(x) (x)
# elif BYTE_ORDER == BIG_ENDIAN
#  define htobe32(x) (x)
#  define htole32(x) bswap_32(x)
#  define be32toh(x) (x)
#  define le32toh(x) bswap_32(x)
# else
#  error Byte order not supported
# endif
#endif

#ifndef HAVE_HTOBE64
# if BYTE_ORDER == LITTLE_ENDIAN
#  define htobe64(x) bswap_64(x)
#  define htole64(x) (x)
#  define be64toh(x) bswap_64(x)
#  define le64toh(x) (x)
# elif BYTE_ORDER == BIG_ENDIAN
#  define htobe64(x) (x)
#  define htole64(x) bswap_64(x)
#  define be64toh(x) (x)
#  define le64toh(x) bswap_64(x)
# else
#  error Byte order not supported
# endif
#endif

/** @endcond */

#endif /* _BYTEORDER_H */
