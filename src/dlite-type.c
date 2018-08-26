#include <assert.h>
#include <string.h>
#include <stddef.h>

#include "err.h"
#include "integers.h"
#include "floats.h"
#include "dlite-type.h"


/* Expands to the struct alignment of type */
#define alignof(type) ((size_t)&((struct { char c; type d; } *)0)->d)


/* Name DLite types */
static char *dtype_names[] = {
  "blob",
  "bool",
  "int",
  "uint",
  "float",
  "fixstring",
  "string",
  "triplet",
  "dimension",
  "baseproperty",
  "property",
};

/* Name of fix-sized types (does not include dliteBlob and dliteFixString) */
static struct _TypeDescr {
  char *typename;
  DLiteType dtype;
  size_t size;
  size_t alignment;
} type_table[] = {
  {"bool",     dliteBool,      sizeof(bool),         alignof(bool)},
  {"int",      dliteInt,       sizeof(int),          alignof(int)},
  {"int8",     dliteInt,       1,                    alignof(int8_t)},
  {"int16",    dliteInt,       2,                    alignof(int16_t)},
  {"int32",    dliteInt,       4,                    alignof(int32_t)},
  {"int64",    dliteInt,       8,                    alignof(int64_t)},
  {"uint",     dliteUInt,      sizeof(unsigned int), alignof(unsigned int)},
  {"uint8",    dliteUInt,      1,                    alignof(uint8_t)},
  {"uint16",   dliteUInt,      2,                    alignof(uint16_t)},
  {"uint32",   dliteUInt,      4,                    alignof(uint32_t)},
  {"uint64",   dliteUInt,      8,                    alignof(uint64_t)},
  {"float",    dliteFloat,     sizeof(float),        alignof(float)},
  {"double",   dliteFloat,     sizeof(double),       alignof(double)},
  {"float32",  dliteFloat,     4,                    alignof(float32_t)},
  {"float64",  dliteFloat,     8,                    alignof(float64_t)},
#ifdef HAVE_FLOAT80
  {"float80",  dliteFloat,     10,                   alignof(float80_t)},
#endif
#ifdef HAVE_FLOAT128
  {"float128", dliteFloat,     16,                   alignof(float128_t)},
#endif
  {"string",   dliteStringPtr, sizeof(char *),       alignof(char *)},
  {"triplet",  dliteTriplet,   sizeof(DLiteTriplet), alignof(DLiteTriplet)},
  {NULL,       0,              0,                    0}
};



/*
   Returns descriptive name for `dtype` or NULL on error.
*/
const char *dlite_type_get_dtypename(DLiteType dtype)
{
  if (dtype < 0 || dtype >= sizeof(dtype_names) / sizeof(char *))
    return err(1, "invalid dtype number: %d", dtype), NULL;
  return dtype_names[dtype];
}

/*
  Returns the dtype corresponding to `dtypename` or -1 on error.
*/
DLiteType dlite_type_get_dtype(const char *dtypename)
{
  int i, N=sizeof(dtype_names) / sizeof(char *);
  for (i=0; i<N; i++)
    if (strcmp(dtypename, dtype_names[i]) == 0) return i;
  return -1;
}

/*
  Writes the type name corresponding to `dtype` and `size` to `typename`,
  which must be of size `n`.  Returns non-zero on error.
*/
int dlite_type_set_typename(DLiteType dtype, size_t size,
                            char *typename, size_t n)
{
  switch (dtype) {
  case dliteBlob:
    snprintf(typename, n, "blob%lu", size);
    break;
  case dliteBool:
    if (size != sizeof(bool))
      return err(1, "bool should have size %lu, but %lu was provided",
                 sizeof(bool), size);
    snprintf(typename, n, "bool");
    break;
  case dliteInt:
    snprintf(typename, n, "int%lu", size);
    break;
  case dliteUInt:
    snprintf(typename, n, "uint%lu", size);
    break;
  case dliteFloat:
    snprintf(typename, n, "float%lu", size);
    break;
  case dliteFixString:
    snprintf(typename, n, "string%lu", size);
    break;
  case dliteStringPtr:
    if (size != sizeof(char *))
      return err(1, "string should have size %lu, but %lu was provided",
                 sizeof(char *), size);
    snprintf(typename, n, "string");
    break;
  default:
    return err(1, "unknown dtype number: %d", dtype);
  }
  return 0;
}

/* Return true if name is a DLiteType, otherwise false. */
bool dlite_is_type(const char *name)
{
  DLiteType dtype;
  size_t size;
  return dlite_type_set_dtype_and_size(name, &dtype, &size) == 0 ? true : false;
}

/*
  Assigns `dtype` and `size` from `typename`.  Returns non-zero on error.
*/
int dlite_type_set_dtype_and_size(const char *typename,
                                  DLiteType *dtype, size_t *size)
{
  int i;
  size_t namelen, typesize;
  char *endptr;

  /* Check if typename is a fixed-sized type listed in the type table */
  for (i=0; type_table[i].typename; i++) {
    if (strcmp(typename, type_table[i].typename) == 0) {
      *dtype = type_table[i].dtype;
      *size = type_table[i].size;
      return 0;
    }
  }

  /* Type is not in the type table - extract its size from `typename` */
  namelen = strcspn(typename, "123456789");
  typesize = strtol(typename + namelen, &endptr, 10);
  assert(endptr > typename + namelen);
  if (*endptr) return err(1, "invalid length of type name: %s", typename);
  if (strncmp(typename, "blob", namelen) == 0) {
    *dtype = dliteBlob;
    *size = typesize;
  } else if (strncmp(typename, "string", namelen) == 0) {
    *dtype = dliteFixString;
    *size = typesize;
  } else {
    return err(1, "unknown type: %s", typename);
  }
  return 0;
}


/*
  Returns the struct alignment of the given type or 0 on error.
 */
size_t dlite_type_get_alignment(DLiteType dtype, size_t size)
{
  int i;
  for (i=0; type_table[i].typename; i++)
    if (type_table[i].dtype == dtype && type_table[i].size == size)
      return type_table[i].alignment;

  switch (dtype) {
  case dliteBlob:       return 1;
  case dliteFixString:  return 1;
  default:              return err(0, "cannot determine alignment of "
                                   "dtype=%d, size=%lu", dtype, size);
  }
}


/**
  Returns the offset the current struct member with dtype \a dtype and
  size \a size.  The offset of the previous struct member is \a prev_offset
  and its size is \a prev_size.

  Returns -1 on error.
 */
int dlite_type_get_member_offset(size_t prev_offset, size_t prev_size,
                                 DLiteType dtype, size_t size)
{
  size_t align, offset, padding;
  if ((align = dlite_type_get_alignment(dtype, size)) == 0) return -1;
  offset = prev_offset + prev_size;
  padding = (align - (offset & (align - 1))) & (align - 1);
  return offset + padding;
}


#if 0
/*
  Returns the offset the next struct member with dtype `dtype` and
  size `size`.  `ptr` should point to the current member which has
  size `cursize`.

  Returns -1 on error.
 */
int dlite_type_get_member_offset(const void *cur, size_t cursize,
                                 DLiteType dtype, size_t size)
{
  size_t align = dlite_type_get_alignment(dtype, size);
  const unsigned char *p = (unsigned char *)cur + cursize;
  size_t padding = (align - ((size_t)p & (align - 1))) & (align - 1);
  if (align == 0)
    return -1;
  else
    return cursize + padding;
}
#endif
