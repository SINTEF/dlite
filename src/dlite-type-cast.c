/* This file implements dlite_type_copy_cast(), which is declared in
   dlite-type.h.  Since it is so long we place it in this file rather
   than in dlite-type.c. */
#include "config.h"

#include <assert.h>
#include <string.h>
#include <stddef.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "utils/err.h"
#include "utils/boolean.h"
#include "utils/integers.h"
#include "utils/floats.h"

#include "dlite-macros.h"
#include "dlite-type.h"

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif


/* Returns 1 if `src` points to a negative number, 0 if it is
   non-negative and -1 if it is undefined. */
static int isnegative(const void *src, DLiteType type, size_t size)
{
  double v;
  char *p, *endptr;

  switch (type) {

  case dliteBool:
  case dliteUInt:
    return 0;

  case dliteInt:
    switch (size) {
    case 1: return (*((int8_t *)src) < 0) ? 1 : 0;
    case 2: return (*((int16_t *)src) < 0) ? 1 : 0;
    case 4: return (*((int32_t *)src) < 0) ? 1 : 0;
    case 8: return (*((int64_t *)src) < 0) ? 1 : 0;
    default: return -1;
    }

  case dliteFloat:
    switch (size) {
    case 4: return (*((float32_t *)src) < 0) ? 1 : 0;
    case 8: return (*((float64_t *)src) < 0) ? 1 : 0;
#ifdef HAVE_FLOAT80
    case 10: return (*((float80_t *)src) < 0) ? 1 : 0;
#endif
#ifdef HAVE_FLOAT128
    case 16: return (*((float128_t *)src) < 0) ? 1 : 0;
#endif
    default: return -1;
    }

  case dliteFixString:
  case dliteStringPtr:
    p = (type == dliteFixString) ? (char *)src : *(char **)src;
    if (!p || !*p) return -1;
    v = strtod(p, &endptr);
    if (*endptr) return -1;
    return (v < 0) ? 1 : 0;

  default:
    return -1;
  }
}


/* Returns 1 if `src` points to a true value, 0 if it is false and -1
   if it is undefined. */
static int istrue(const void *src, DLiteType type, size_t size)
{
  size_t i;
  uint8_t *q;
  double v;
  char *p, *endptr;

  switch (type) {

  case dliteBlob:
    q = (uint8_t *)src;
    for (i=0; i<size; i++, q++) if (*q) return 0;
    return 1;

  case dliteBool:
    return *((bool *)src);

  case dliteUInt:
    switch (size) {
    case 1: return (*((uint8_t *)src))  ? 1 : 0;
    case 2: return (*((uint16_t *)src)) ? 1 : 0;
    case 4: return (*((uint32_t *)src)) ? 1 : 0;
    case 8: return (*((uint64_t *)src)) ? 1 : 0;
    default: return -1;
    }

  case dliteInt:
    switch (size) {
    case 1: return (*((int8_t *)src))  ? 1 : 0;
    case 2: return (*((int16_t *)src)) ? 1 : 0;
    case 4: return (*((int32_t *)src)) ? 1 : 0;
    case 8: return (*((int64_t *)src)) ? 1 : 0;
    default: return -1;
    }

  case dliteFloat:
    switch (size) {
    case 4: return (*((float32_t *)src))   ? 1 : 0;
    case 8: return (*((float64_t *)src))   ? 1 : 0;
#ifdef HAVE_FLOAT80
    case 10: return (*((float80_t *)src))  ? 1 : 0;
#endif
#ifdef HAVE_FLOAT128
    case 16: return (*((float128_t *)src)) ? 1 : 0;
#endif
    default: return -1;
    }

  case dliteFixString:
  case dliteStringPtr:
    p = (type == dliteFixString) ? (char *)src : *(char **)src;
    if (!p || !*p) return -1;
    v = strtod(p, &endptr);
    if (*endptr) return -1;
    return (v) ? 1 : 0;

  default:
    return -1;
  }
}


#define toBlob                                                  \
  do {                                                          \
    if (dest_size > src_size) memset(dest, 0, dest_size);       \
    memcpy(dest, src, MIN(src_size, dest_size));                \
    return 0;                                                   \
  } while (0)

#define toBool                                          \
  do {                                                  \
    switch (istrue(src, src_type, src_size)) {          \
    case 0: *((bool *)src) = 0; return 0;               \
    case 1: *((bool *)src) = 1; return 0;               \
    default: goto fail;                                 \
    }                                                   \
  } while (0)

#define toFixString                                                     \
  do {                                                                  \
    if (dlite_type_print(dest, dest_size, src, src_type, src_size,      \
                         0, -2, 0) < 0) goto fail;                      \
    return 0;                                                           \
  } while (0)

#define toStringPtr                                                     \
  do {                                                                  \
    int n, m;                                                           \
    char *p;                                                            \
    UNUSED(m);  /* m is unused if NDEBUG is non-zero */                 \
    if ((n = dlite_type_print(NULL, 0, src, src_type, src_size,         \
                              0, -2, 0)) < 0) goto fail;                \
    if (!(p = realloc(*(char **)dest, n + 1)))                          \
      FAIL("reallocation failure");                                     \
    *(char **)dest = p;                                                 \
    m = dlite_type_print(*(char **)dest, n+1, src, src_type, src_size,  \
                         0, -2, 0);                                     \
    assert(m == n);                                                     \
    return 0;                                                           \
  } while (0)


/*
  Copies value from `src` to `dest`.  If `dest_type` and `dest_size` differs
  from `src_type` and `src_size` the value will be casted, if possible.

  If `dest_type` contains allocated data, new memory will be allocated
  for `dest`.  Information may get lost in this case.

  Returns non-zero on error.
*/
int dlite_type_copy_cast(void *dest, DLiteType dest_type, size_t dest_size,
                         const void *src, DLiteType src_type, size_t src_size)
{
  char *p, *endptr;
  long long int vi;
  long double vf;
  char stype[32], dtype[32];

  switch (src_type) {

  case dliteBlob:
    switch (dest_type) {
    case dliteBlob:
    case dliteBool:
    case dliteInt:
    case dliteUInt:
    case dliteFloat:
      if (dest_size > src_size) memset(dest, 0, dest_size);
      memcpy(dest, src, MIN(src_size, dest_size));
      return 0;
    case dliteFixString:
      toFixString;
    case dliteStringPtr:
      toStringPtr;
    default:
      goto fail;
    }
    assert(0);

  case dliteBool:
    assert(src_size == sizeof(bool));
    switch (dest_type) {
    case dliteBlob:
      toBlob;
    case dliteBool:
      toBool;
    case dliteUInt:
      switch (dest_size) {
      case 1: *((bool *)dest) = (*((uint8_t *)src)) ? 1 : 0;  return 0;
      case 2: *((bool *)dest) = (*((uint16_t *)src)) ? 1 : 0; return 0;
      case 4: *((bool *)dest) = (*((uint32_t *)src)) ? 1 : 0; return 0;
      case 8: *((bool *)dest) = (*((uint64_t *)src)) ? 1 : 0; return 0;
      default: goto fail;
      }
    case dliteInt:
      switch (dest_size) {
      case 1: *((bool *)dest) = (*((int8_t *)src)) ? 1 : 0;  return 0;
      case 2: *((bool *)dest) = (*((int16_t *)src)) ? 1 : 0; return 0;
      case 4: *((bool *)dest) = (*((int32_t *)src)) ? 1 : 0; return 0;
      case 8: *((bool *)dest) = (*((int64_t *)src)) ? 1 : 0; return 0;
      default: goto fail;
      }
    case dliteFloat:
      switch (dest_size) {
      case 4: *((bool *)dest) = (*((float32_t *)src)) ? 1 : 0;   return 0;
      case 8: *((bool *)dest) = (*((float64_t *)src)) ? 1 : 0;   return 0;
#ifdef HAVE_FLOAT80
      case 10: *((bool *)dest) = (*((float80_t *)src)) ? 1 : 0;  return 0;
#endif
 #ifdef HAVE_FLOAT128
      case 16: *((bool *)dest) = (*((float128_t *)src)) ? 1 : 0; return 0;
#endif
      default: goto fail;
      }
    case dliteFixString:
      toFixString;
    case dliteStringPtr:
      toStringPtr;
    default:
      goto fail;
    }
    assert(0);

  case dliteUInt:
    switch (dest_type) {
    case dliteBlob:
    case dliteUInt:
      if (dest_size > src_size) memset(dest, 0, dest_size);
      memcpy(dest, src, MIN(src_size, dest_size));
      return 0;
    case dliteBool:
      toBool;
    case dliteInt:
      switch (dest_size) {
      case 1:
        switch (src_size) {
        case 1: *((int8_t *)dest) = *((uint8_t *)src);  return 0;
        case 2: *((int8_t *)dest) = *((uint16_t *)src); return 0;
        case 4: *((int8_t *)dest) = *((uint32_t *)src); return 0;
        case 8: *((int8_t *)dest) = *((uint64_t *)src); return 0;
        default: goto fail;
        }
      case 2:
        switch (src_size) {
        case 1: *((int16_t *)dest) = *((uint8_t *)src);  return 0;
        case 2: *((int16_t *)dest) = *((uint16_t *)src); return 0;
        case 4: *((int16_t *)dest) = *((uint32_t *)src); return 0;
        case 8: *((int16_t *)dest) = *((uint64_t *)src); return 0;
        default: goto fail;
        }
      case 4:
        switch (src_size) {
        case 1: *((int32_t *)dest) = *((uint8_t *)src);  return 0;
        case 2: *((int32_t *)dest) = *((uint16_t *)src); return 0;
        case 4: *((int32_t *)dest) = *((uint32_t *)src); return 0;
        case 8: *((int32_t *)dest) = *((uint64_t *)src); return 0;
        default: goto fail;
        }
      case 8:
        switch (src_size) {
        case 1: *((int64_t *)dest) = *((uint8_t *)src);  return 0;
        case 2: *((int64_t *)dest) = *((uint16_t *)src); return 0;
        case 4: *((int64_t *)dest) = *((uint32_t *)src); return 0;
        case 8: *((int64_t *)dest) = *((uint64_t *)src); return 0;
        default: goto fail;
        }
      default:
        goto fail;
      }
      assert(0);
    case dliteFloat:
      switch (dest_size) {
      case 4:
        switch (src_size) {
        case 1: *((float32_t *)dest) = *((uint8_t *)src);  return 0;
        case 2: *((float32_t *)dest) = *((uint16_t *)src); return 0;
        case 4: *((float32_t *)dest) = *((uint32_t *)src); return 0;
        case 8: *((float32_t *)dest) = *((uint64_t *)src); return 0;
        default: goto fail;
        }
      case 8:
        switch (src_size) {
        case 1: *((float64_t *)dest) = *((uint8_t *)src);  return 0;
        case 2: *((float64_t *)dest) = *((uint16_t *)src); return 0;
        case 4: *((float64_t *)dest) = *((uint32_t *)src); return 0;
        case 8: *((float64_t *)dest) = *((uint64_t *)src); return 0;
        default: goto fail;
        }
#ifdef HAVE_FLOAT80
      case 10:
        switch (src_size) {
        case 1: *((float80_t *)dest) = *((uint8_t *)src);  return 0;
        case 2: *((float80_t *)dest) = *((uint16_t *)src); return 0;
        case 4: *((float80_t *)dest) = *((uint32_t *)src); return 0;
        case 8: *((float80_t *)dest) = *((uint64_t *)src); return 0;
        default: goto fail;
        }
#endif
#ifdef HAVE_FLOAT128
      case 16:
        switch (src_size) {
        case 1: *((float128_t *)dest) = *((uint8_t *)src);  return 0;
        case 2: *((float128_t *)dest) = *((uint16_t *)src); return 0;
        case 4: *((float128_t *)dest) = *((uint32_t *)src); return 0;
        case 8: *((float128_t *)dest) = *((uint64_t *)src); return 0;
        default: goto fail;
        }
#endif
      default:
        goto fail;
      }
      assert(0);
    case dliteFixString:
      toFixString;
    case dliteStringPtr:
      toStringPtr;
    default:
      goto fail;
    }
    assert(0);

  case dliteInt:
    switch (dest_type) {
    case dliteBlob:
      toBlob;
    case dliteBool:
      toBool;
    case dliteUInt:
      switch (isnegative(src, src_type, src_size)) {
      case 0: break;
      case 1: return err(1, "cannot cast negative int%lu_t to uint%lu_t",
                         (unsigned long)src_size*8,
                         (unsigned long)dest_size*8);
      default: goto fail;
      }
      switch (dest_size) {
      case 1:
        switch (src_size) {
        case 1: *((uint8_t *)dest) = *((int8_t *)src);  return 0;
        case 2: *((uint8_t *)dest) = *((int16_t *)src); return 0;
        case 4: *((uint8_t *)dest) = *((int32_t *)src); return 0;
        case 8: *((uint8_t *)dest) = *((int64_t *)src); return 0;
        default: goto fail;
        }
      case 2:
        switch (src_size) {
        case 1: *((uint16_t *)dest) = *((int8_t *)src);  return 0;
        case 2: *((uint16_t *)dest) = *((int16_t *)src); return 0;
        case 4: *((uint16_t *)dest) = *((int32_t *)src); return 0;
        case 8: *((uint16_t *)dest) = *((int64_t *)src); return 0;
        default: goto fail;
        }
      case 4:
        switch (src_size) {
        case 1: *((uint32_t *)dest) = *((int8_t *)src);  return 0;
        case 2: *((uint32_t *)dest) = *((int16_t *)src); return 0;
        case 4: *((uint32_t *)dest) = *((int32_t *)src); return 0;
        case 8: *((uint32_t *)dest) = *((int64_t *)src); return 0;
        default: goto fail;
        }
      case 8:
        switch (src_size) {
        case 1: *((uint64_t *)dest) = *((int8_t *)src);  return 0;
        case 2: *((uint64_t *)dest) = *((int16_t *)src); return 0;
        case 4: *((uint64_t *)dest) = *((int32_t *)src); return 0;
        case 8: *((uint64_t *)dest) = *((int64_t *)src); return 0;
        default: goto fail;
        }
      default:
        goto fail;
      }
      assert(0);
    case dliteInt:  /* int -> int */
      switch (dest_size) {
      case 1:
        switch (src_size) {
        case 1: *((int8_t *)dest) = *((int8_t *)src);  return 0;
        case 2: *((int8_t *)dest) = *((int16_t *)src); return 0;
        case 4: *((int8_t *)dest) = *((int32_t *)src); return 0;
        case 8: *((int8_t *)dest) = *((int64_t *)src); return 0;
        default: goto fail;
        }
      case 2:
        switch (src_size) {
        case 1: *((int16_t *)dest) = *((int8_t *)src);  return 0;
        case 2: *((int16_t *)dest) = *((int16_t *)src); return 0;
        case 4: *((int16_t *)dest) = *((int32_t *)src); return 0;
        case 8: *((int16_t *)dest) = *((int64_t *)src); return 0;
        default: goto fail;
        }
      case 4:
        switch (src_size) {
        case 1: *((int32_t *)dest) = *((int8_t *)src);  return 0;
        case 2: *((int32_t *)dest) = *((int16_t *)src); return 0;
        case 4: *((int32_t *)dest) = *((int32_t *)src); return 0;
        case 8: *((int32_t *)dest) = *((int64_t *)src); return 0;
        default: goto fail;
        }
      case 8:
        switch (src_size) {
        case 1: *((int64_t *)dest) = *((int8_t *)src);  return 0;
        case 2: *((int64_t *)dest) = *((int16_t *)src); return 0;
        case 4: *((int64_t *)dest) = *((int32_t *)src); return 0;
        case 8: *((int64_t *)dest) = *((int64_t *)src); return 0;
        default: goto fail;
        }
      default:
        goto fail;
      }
      assert(0);
    case dliteFloat:
      switch (dest_size) {
      case 4:
        switch (src_size) {
        case 1: *((float32_t *)dest) = *((int8_t *)src);  return 0;
        case 2: *((float32_t *)dest) = *((int16_t *)src); return 0;
        case 4: *((float32_t *)dest) = *((int32_t *)src); return 0;
        case 8: *((float32_t *)dest) = *((int64_t *)src); return 0;
        default: goto fail;
        }
      case 8:
        switch (src_size) {
        case 1: *((float64_t *)dest) = *((int8_t *)src);  return 0;
        case 2: *((float64_t *)dest) = *((int16_t *)src); return 0;
        case 4: *((float64_t *)dest) = *((int32_t *)src); return 0;
        case 8: *((float64_t *)dest) = *((int64_t *)src); return 0;
        default: goto fail;
        }
#ifdef HAVE_FLOAT80
      case 10:
        switch (src_size) {
        case 1: *((float80_t *)dest) = *((int8_t *)src);  return 0;
        case 2: *((float80_t *)dest) = *((int16_t *)src); return 0;
        case 4: *((float80_t *)dest) = *((int32_t *)src); return 0;
        case 8: *((float80_t *)dest) = *((int64_t *)src); return 0;
        default: goto fail;
        }
#endif
#ifdef HAVE_FLOAT128
      case 16:
        switch (src_size) {
        case 1: *((float128_t *)dest) = *((int8_t *)src);  return 0;
        case 2: *((float128_t *)dest) = *((int16_t *)src); return 0;
        case 4: *((float128_t *)dest) = *((int32_t *)src); return 0;
        case 8: *((float128_t *)dest) = *((int64_t *)src); return 0;
        default: goto fail;
        }
#endif
      default:
        goto fail;
      }
      assert(0);
    case dliteFixString:
      toFixString;
    case dliteStringPtr:
      toStringPtr;
    default:
      goto fail;
    }
    assert(0);

  case dliteFloat:
    switch (dest_type) {
    case dliteBlob:
      toBlob;
    case dliteBool:
      toBool;
    case dliteUInt:
      switch (isnegative(src, src_type, src_size)) {
      case 0: break;
      case 1: return err(1, "cannot cast negative float%lu_t to uint%lu_t",
                         (unsigned long)src_size*8, (unsigned long)dest_size*8);
      default: goto fail;
      }
      switch (dest_size) {
      case 1:
        switch (src_size) {
        case 4: *((uint8_t *)dest) = *((float32_t *)src);   return 0;
        case 8: *((uint8_t *)dest) = *((float64_t *)src);   return 0;
#ifdef HAVE_FLOAT80
        case 10: *((uint8_t *)dest) = *((float80_t *)src);  return 0;
#endif
#ifdef HAVE_FLOAT128
        case 16: *((uint8_t *)dest) = *((float128_t *)src); return 0;
#endif
        default: goto fail;
        }
      case 2:
        switch (src_size) {
        case 4: *((uint16_t *)dest) = *((float32_t *)src);   return 0;
        case 8: *((uint16_t *)dest) = *((float64_t *)src);   return 0;
#ifdef HAVE_FLOAT80
        case 10: *((uint16_t *)dest) = *((float80_t *)src);  return 0;
#endif
#ifdef HAVE_FLOAT128
        case 16: *((uint16_t *)dest) = *((float128_t *)src); return 0;
#endif
        default: goto fail;
        }
      case 4:
        switch (src_size) {
        case 4: *((uint32_t *)dest) = *((float32_t *)src);   return 0;
        case 8: *((uint32_t *)dest) = *((float64_t *)src);   return 0;
#ifdef HAVE_FLOAT80
        case 10: *((uint32_t *)dest) = *((float80_t *)src);  return 0;
#endif
#ifdef HAVE_FLOAT128
        case 16: *((uint32_t *)dest) = *((float128_t *)src); return 0;
#endif
        default: goto fail;
        }
      case 8:
        switch (src_size) {
        case 4: *((uint64_t *)dest) = *((float32_t *)src);   return 0;
        case 8: *((uint64_t *)dest) = *((float64_t *)src);   return 0;
#ifdef HAVE_FLOAT80
        case 10: *((uint64_t *)dest) = *((float80_t *)src);  return 0;
#endif
#ifdef HAVE_FLOAT128
        case 16: *((uint64_t *)dest) = *((float128_t *)src); return 0;
#endif
        default: goto fail;
        }
      default:
        goto fail;
      }
      assert(0);
    case dliteInt:
      switch (dest_size) {
      case 1:
        switch (src_size) {
        case 4: *((int8_t *)dest) = *((float32_t *)src);   return 0;
        case 8: *((int8_t *)dest) = *((float64_t *)src);   return 0;
#ifdef HAVE_FLOAT80
        case 10: *((int8_t *)dest) = *((float80_t *)src);  return 0;
#endif
#ifdef HAVE_FLOAT128
        case 16: *((int8_t *)dest) = *((float128_t *)src); return 0;
#endif
        default: goto fail;
        }
      case 2:
        switch (src_size) {
        case 4: *((int16_t *)dest) = *((float32_t *)src);   return 0;
        case 8: *((int16_t *)dest) = *((float64_t *)src);   return 0;
#ifdef HAVE_FLOAT80
        case 10: *((int16_t *)dest) = *((float80_t *)src);  return 0;
#endif
#ifdef HAVE_FLOAT128
        case 16: *((int16_t *)dest) = *((float128_t *)src); return 0;
#endif
        default: goto fail;
        }
      case 4:
        switch (src_size) {
        case 4: *((int32_t *)dest) = *((float32_t *)src);   return 0;
        case 8: *((int32_t *)dest) = *((float64_t *)src);   return 0;
#ifdef HAVE_FLOAT80
        case 10: *((int32_t *)dest) = *((float80_t *)src);  return 0;
#endif
#ifdef HAVE_FLOAT128
        case 16: *((int32_t *)dest) = *((float128_t *)src); return 0;
#endif
        default: goto fail;
        }
      case 8:
        switch (src_size) {
        case 4: *((int64_t *)dest) = *((float32_t *)src);   return 0;
        case 8: *((int64_t *)dest) = *((float64_t *)src);   return 0;
#ifdef HAVE_FLOAT80
        case 10: *((int64_t *)dest) = *((float80_t *)src);  return 0;
#endif
#ifdef HAVE_FLOAT128
        case 16: *((int64_t *)dest) = *((float128_t *)src); return 0;
#endif
        default: goto fail;
        }
      default:
        goto fail;
      }
      abort();  /* should never be reached */
    case dliteFloat:
      switch (dest_size) {
      case 4:
        switch (src_size) {
        case 4: *((float32_t *)dest) = *((float32_t *)src);   return 0;
        case 8: *((float32_t *)dest) = *((float64_t *)src);   return 0;
#ifdef HAVE_FLOAT80
        case 10: *((float32_t *)dest) = *((float80_t *)src);  return 0;
#endif
#ifdef HAVE_FLOAT128
        case 16: *((float32_t *)dest) = *((float128_t *)src); return 0;
#endif
        default: goto fail;
        }
      case 8:
        switch (src_size) {
        case 4: *((float64_t *)dest) = *((float32_t *)src);   return 0;
        case 8: *((float64_t *)dest) = *((float64_t *)src);   return 0;
#ifdef HAVE_FLOAT80
        case 10: *((float64_t *)dest) = *((float80_t *)src);  return 0;
#endif
#ifdef HAVE_FLOAT128
        case 16: *((float64_t *)dest) = *((float128_t *)src); return 0;
#endif
        default: goto fail;
        }
#ifdef HAVE_FLOAT80
      case 10:
        switch (src_size) {
        case 4: *((float80_t *)dest) = *((float32_t *)src);   return 0;
        case 8: *((float80_t *)dest) = *((float64_t *)src);   return 0;
        case 10: *((float80_t *)dest) = *((float80_t *)src);  return 0;
#ifdef HAVE_FLOAT128
        case 16: *((float80_t *)dest) = *((float128_t *)src); return 0;
#endif
        default: goto fail;
        }
#endif
#ifdef HAVE_FLOAT128
      case 16:
        switch (src_size) {
        case 4: *((float128_t *)dest) = *((float32_t *)src);   return 0;
        case 8: *((float|18_t *)dest) = *((float64_t *)src);   return 0;
#ifdef HAVE_FLOAT80
        case 10: *((float128_t *)dest) = *((float80_t *)src);  return 0;
#endif
        case 16: *((float128_t *)dest) = *((float128_t *)src); return 0;
        default: goto fail;
        }
#endif
      default:
        goto fail;
      }
      assert(0);
    case dliteFixString:
      toFixString;
    case dliteStringPtr:
      toStringPtr;
    default:
      goto fail;
    }
    assert(0);

  case dliteFixString:
  case dliteStringPtr:
    p = (src_type == dliteFixString) ? (char *)src : *(char **)src;
    switch (dest_type) {
    case dliteBlob:
      memset(dest, 0, dest_size);
      memcpy(dest, p, strlen(p));
      return 0;
    case dliteBool:
      *(bool *)src = (p && *p) ? 1 : 0;
      return 0;
    case dliteUInt:
      switch (isnegative(src, src_type, src_size)) {
      case 0: break;
      case 1: return err(1, "cannot cast negative string value \"%s\" to "
                         "uint%lu_t", p, (unsigned long)(dest_size*8));
      default: goto fail;
      }
      vi = strtoll(p, &endptr, 0);
      if (*endptr) return err(1, "cannot cast string \"%s\" to uint", p);
      if (vi < 0) return err(1, "cannot cast string \"%s\" to uint", p);
      switch (dest_size) {
      case 1: *((uint8_t *)dest) = vi;  return 0;
      case 2: *((uint16_t *)dest) = vi; return 0;
      case 4: *((uint32_t *)dest) = vi; return 0;
      case 8: *((uint64_t *)dest) = vi; return 0;
      default: goto fail;
      }
    case dliteInt:
      vi = strtoll(p, &endptr, 0);
      if (*endptr) return err(1, "cannot cast string \"%s\" to int", p);
      switch (dest_size) {
      case 1: *((int8_t *)dest) = vi;  return 0;
      case 2: *((int16_t *)dest) = vi; return 0;
      case 4: *((int32_t *)dest) = vi; return 0;
      case 8: *((int64_t *)dest) = vi; return 0;
      default: goto fail;
      }
    case dliteFloat:
      vf = strtold(p, &endptr);
      if (*endptr) return err(1, "cannot cast string \"%s\" to float", p);
      switch (dest_size) {
      case 4: *((float32_t *)dest) = vf;   return 0;
      case 8: *((float64_t *)dest) = vf;   return 0;
#ifdef HAVE_FLOAT80
      case 10: *((float80_t *)dest) = vf;  return 0;
#endif
#ifdef HAVE_FLOAT128
      case 16: *((float128_t *)dest) = vf; return 0;
#endif
      default: goto fail;
      }
    case dliteFixString:
      toFixString;
    case dliteStringPtr:
      toStringPtr;
    default:
      goto fail;
    }
    assert(0);

  case dliteDimension:
    if (dest_type == dliteDimension) {
      assert(dest_size == src_size);
      if (!dlite_type_copy(dest, src, dest_type, dest_size)) goto fail;
      return 0;
    }
    goto fail;

  case dliteProperty:
    if (dest_type == dliteProperty) {
      assert(dest_size == src_size);
      if (!dlite_type_copy(dest, src, dest_type, dest_size)) goto fail;
      return 0;
    }
    goto fail;

  case dliteRelation:
    if (dest_type == dliteRelation) {
      assert(dest_size == src_size);
      if (!dlite_type_copy(dest, src, dest_type, dest_size)) goto fail;
      return 0;
    }
    goto fail;

  default:
    goto fail;
  }
  assert(0);

 fail:
  dlite_type_set_typename(src_type, src_size, stype, sizeof(stype));
  dlite_type_set_typename(dest_type, dest_size, dtype, sizeof(dtype));
  return err(1, "cannot cast %s to %s", stype, dtype);
}
