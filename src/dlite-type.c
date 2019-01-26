#include "config.h"

#include <assert.h>
#include <string.h>
#include <stddef.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "utils/err.h"
#include "uuid/integers.h"
#include "floats.h"
#include "triplestore.h"
#include "dlite-entity.h"
#include "dlite-type.h"


/* Name DLite types */
static char *dtype_names[] = {
  "blob",
  "bool",
  "int",
  "uint",
  "float",
  "fixstring",
  "string",

  "dimension",
  "property",
  "relation",
};

/* Type enum names */
static char *dtype_enum_names[] = {
  "dliteBlob",
  "dliteBool",
  "dliteInt",
  "dliteUInt",
  "dliteFloat",
  "dliteFixString",
  "dliteStringPtr",

  "dliteDimension",
  "dliteProperty",
  "dliteRelation",
};

/* Name of fix-sized types (does not include dliteBlob and dliteFixString) */
static struct _TypeDescr {
  char *typename;
  DLiteType dtype;
  size_t size;
  size_t alignment;
} type_table[] = {
  {"bool",     dliteBool,      sizeof(bool),          alignof(bool)},
  {"int",      dliteInt,       sizeof(int),           alignof(int)},
  {"int8",     dliteInt,       1,                     alignof(int8_t)},
  {"int16",    dliteInt,       2,                     alignof(int16_t)},
  {"int32",    dliteInt,       4,                     alignof(int32_t)},
  {"int64",    dliteInt,       8,                     alignof(int64_t)},
  {"uint",     dliteUInt,      sizeof(unsigned int),  alignof(unsigned int)},
  {"uint8",    dliteUInt,      1,                     alignof(uint8_t)},
  {"uint16",   dliteUInt,      2,                     alignof(uint16_t)},
  {"uint32",   dliteUInt,      4,                     alignof(uint32_t)},
  {"uint64",   dliteUInt,      8,                     alignof(uint64_t)},
  {"float",    dliteFloat,     sizeof(float),         alignof(float)},
  {"double",   dliteFloat,     sizeof(double),        alignof(double)},
  {"longdouble",dliteFloat,    sizeof(long double),   alignof(long double)},
  {"float32",  dliteFloat,     4,                     alignof(float32_t)},
  {"float64",  dliteFloat,     8,                     alignof(float64_t)},
#ifdef HAVE_FLOAT80
  {"float80",  dliteFloat,     10,                    alignof(float80_t)},
#endif
#ifdef HAVE_FLOAT128
  {"float128", dliteFloat,     16,                    alignof(float128_t)},
#endif
  {"string",   dliteStringPtr, sizeof(char *),        alignof(char *)},

  {"dimension",dliteDimension, sizeof(DLiteDimension),alignof(DLiteDimension)},
  {"property", dliteProperty,  sizeof(DLiteProperty), alignof(DLiteProperty)},
  {"relation", dliteRelation,  sizeof(DLiteRelation), alignof(DLiteRelation)},

  {NULL,       0,              0,                     0}
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
  Returns enum name for `dtype` or NULL on error.
 */
const char *dlite_type_get_enum_name(DLiteType dtype)
{
  if (dtype < 0 || dtype >= sizeof(dtype_names) / sizeof(char *))
    return err(1, "invalid dtype number: %d", dtype), NULL;
  return dtype_enum_names[dtype];
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
    snprintf(typename, n, "blob%zu", size);
    break;
  case dliteBool:
    if (size != sizeof(bool))
      return errx(1, "bool should have size %zu, but %zu was provided",
                 sizeof(bool), size);
    snprintf(typename, n, "bool");
    break;
  case dliteInt:
    snprintf(typename, n, "int%zu", size*8);
    break;
  case dliteUInt:
    snprintf(typename, n, "uint%zu", size*8);
    break;
  case dliteFloat:
    snprintf(typename, n, "float%zu", size*8);
    break;
  case dliteFixString:
    snprintf(typename, n, "string%zu", size);
    break;
  case dliteStringPtr:
    if (size != sizeof(char *))
      return errx(1, "string should have size %zu, but %zu was provided",
                 sizeof(char *), size);
    snprintf(typename, n, "string");
    break;
  case dliteDimension:
    snprintf(typename, n, "dimension");
    break;
  case dliteProperty:
    snprintf(typename, n, "property");
    break;
  case dliteRelation:
    snprintf(typename, n, "relation");
    break;
  default:
    return errx(1, "unknown dtype number: %d", dtype);
  }
  return 0;
}


/*
  If the type specified with `dtype` and `size` has a native type name
  (like "short" and "double"), return a pointer to this typename.  Otherwise
  NULL is returned.
*/
const char *dlite_type_get_native_typename(DLiteType dtype, size_t size)
{
  switch (dtype) {
  case dliteInt:
    switch (size) {
    case sizeof(char):      return "char";
    case sizeof(short):     return "short";
    case sizeof(int):       return "int";
#if SIZEOF_LONG != SIZEOF_INT
    case sizeof(long):      return "long";
#endif
#if defined(HAVE_LONG_LONG) && SIZEOF_LONG_LONG != SIZEOF_LONG
    case sizeof(long long): return "long long";
#endif
    }
    break;
  case dliteUInt:
    switch (size) {
    case sizeof(unsigned char):      return "unsigned char";
    case sizeof(unsigned short):     return "unsigned short";
    case sizeof(unsigned int):       return "unsigned int";
#if SIZEOF_LONG != SIZEOF_INT
    case sizeof(unsigned long):      return "unsigned long";
#endif
#if defined(HAVE_LONG_LONG) && SIZEOF_LONG_LONG != SIZEOF_LONG
    case sizeof(unsigned long long): return "unsigned long long";
#endif
    }
    break;
  case dliteFloat:
    switch (size) {
    case sizeof(float):        return "float";
    case sizeof(double):       return "double";
#if defined(HAVE_LONG_DOUBLE) && SIZEOF_LONG_DOUBLE != SIZEOF_DOUBLE
    case sizeof(long double):  return "long double";
#endif
    }
    break;
  default:
    break;
  }
  return NULL;
}

/*
  Writes C declaration to `pcdecl` of a C variable with given `dtype`
  and `size`.  The size of the memory pointed to by `pcdecl` must be
  at least `n` bytes.

  If `native` is non-zero, the native typename will be written to `pcdecl`
  (e.g. "double") instead of the portable typename (e.g. "float64_t").

  `name` is the name of the C variable.

  `nref` is the number of extra * to add in front of `name`.

  Returns the number of bytes written or -1 on error.
*/
int dlite_type_set_cdecl(DLiteType dtype, size_t size, const char *name,
                         size_t nref, char *pcdecl, size_t n, int native)
{
  int m;
  char ref[32];
  const char *native_type;

  if (nref >= sizeof(ref))
    return errx(-1, "too many dereferences to write: %zu", nref);
  memset(ref, '*', sizeof(ref));
  ref[nref] = '\0';

  switch (dtype) {
  case dliteBlob:
    m = snprintf(pcdecl, n, "uint8_t %s%s[%zu]", ref, name, size);
    break;
  case dliteBool:
    if (size != sizeof(bool))
      return errx(-1, "bool should have size %zu, but %zu was provided",
                 sizeof(bool), size);
    m = snprintf(pcdecl, n, "bool %s%s", ref, name);
    break;
  case dliteInt:
    if (native && (native_type = dlite_type_get_native_typename(dtype, size)))
      m = snprintf(pcdecl, n, "%s %s%s", native_type, ref, name);
    else
      m = snprintf(pcdecl, n, "int%zu_t %s%s", size*8, ref, name);
    break;
  case dliteUInt:
    if (native && (native_type = dlite_type_get_native_typename(dtype, size)))
      m = snprintf(pcdecl, n, "%s %s%s", native_type, ref, name);
    else
      m = snprintf(pcdecl, n, "uint%zu_t %s%s", size*8, ref, name);
    break;
  case dliteFloat:
    if (native && (native_type = dlite_type_get_native_typename(dtype, size)))
      m = snprintf(pcdecl, n, "%s %s%s", native_type, ref, name);
    else
      m = snprintf(pcdecl, n, "float%zu_t %s%s", size*8, ref, name);
    break;
  case dliteFixString:
    m = snprintf(pcdecl, n, "char %s%s[%zu]", ref, name, size);
    break;
  case dliteStringPtr:
    if (size != sizeof(char *))
      return errx(-1, "string should have size %zu, but %zu was provided",
                 sizeof(char *), size);
    m = snprintf(pcdecl, n, "char *%s%s", ref, name);
    break;
  case dliteDimension:
    if (size != sizeof(DLiteDimension))
      return errx(-1, "DLiteDimension must have size %zu, got %zu",
                  sizeof(DLiteDimension), size);
    m = snprintf(pcdecl, n, "DLiteDimension %s%s", ref, name);
    break;
  case dliteProperty:
    if (size != sizeof(DLiteProperty))
      return errx(-1, "DLiteProperty must have size %zu, got %zu",
                  sizeof(DLiteProperty), size);
    m = snprintf(pcdecl, n, "DLiteProperty %s%s", ref, name);
    break;
  case dliteRelation:
    if (size != sizeof(DLiteRelation))
      return errx(-1, "DLiteRelation must have size %zu, got %zu",
                  sizeof(DLiteRelation), size);
    m = snprintf(pcdecl, n, "DLiteRelation %s%s", ref, name);
    break;
  default:
    return errx(-1, "unknown dtype number: %d", dtype);
  }
  if (m < 0)
    return err(-1, "error writing C declaration for dtype %d", dtype);
  return m;
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
  Returns non-zero id `dtype` contains allocated data, like dliteStringPtr.
 */
int dlite_type_is_allocated(DLiteType dtype)
{
  switch (dtype) {
  case dliteBlob:
  case dliteBool:
  case dliteInt:
  case dliteUInt:
  case dliteFloat:
  case dliteFixString:
    return 0;
  case dliteStringPtr:
  case dliteDimension:
  case dliteProperty:
  case dliteRelation:
    return 1;
  }
  abort();  /* should never be reached */
}

/*
  Copies value of given dtype from `src` to `dest`.  If the dtype contains
  allocated data, new memory will be allocated for `dest`.

  Returns a pointer to the memory area `dest` or NULL on error.
*/
void *dlite_type_copy(void *dest, const void *src, DLiteType dtype, size_t size)
{
  switch (dtype) {
  case dliteBlob:
  case dliteBool:
  case dliteInt:
  case dliteUInt:
  case dliteFloat:
  case dliteFixString:
    memcpy(dest, src, size);
    break;
  case dliteStringPtr:
    {
      char *s = *((char **)src);
      if (s) {
        size_t len = strlen(s) + 1;
        *((void **)dest) = realloc(*((void **)dest), len);
        memcpy(*((void **)dest), s, len);
      } else if (*((void **)dest)) {
        free(*((void **)dest));
        *((void **)dest) = NULL;
      }
    }
    break;
  case dliteDimension:
    {
      DLiteDimension *d=dest;
      const DLiteDimension *s=src;
      d->name = strdup(s->name);
      d->description = strdup(s->description);
    }
    break;
  case dliteProperty:
    {
      DLiteProperty *d=dest;
      const DLiteProperty *s=src;
      d->name = strdup(s->name);
      d->type = s->type;
      d->size = s->size;
      d->ndims = s->ndims;
      if (d->ndims) {
        d->dims = malloc(d->ndims*sizeof(int));
        memcpy(d->dims, s->dims, d->ndims*sizeof(int));
      } else
        d->dims = NULL;
      d->unit = (s->unit) ? strdup(s->unit) : NULL;
      d->description = (s->description) ? strdup(s->description) : NULL;
    }
    break;
  case dliteRelation:
    {
      DLiteRelation *d=dest;
      const DLiteRelation *s=src;
      d->s = strdup(s->s);
      d->p = strdup(s->p);
      d->o = strdup(s->o);
      d->id = (s->id) ? strdup(s->id) : NULL;
    }
    break;
  }
  return dest;
}

/*
  Clears the memory pointed to by `p`.  Its type is gived by `dtype` and `size`.

  Returns a pointer to the memory area `p` or NULL on error.
*/
void *dlite_type_clear(void *p, DLiteType dtype, size_t size)
{
  switch (dtype) {
  case dliteBlob:
  case dliteBool:
  case dliteInt:
  case dliteUInt:
  case dliteFloat:
  case dliteFixString:
    break;
  case dliteStringPtr:
    free(*((char **)p));
    break;
  case dliteDimension:
    free(((DLiteDimension *)p)->name);
    free(((DLiteDimension *)p)->description);
    break;
  case dliteProperty:
    free(((DLiteProperty *)p)->name);
    if (((DLiteProperty *)p)->dims) free(((DLiteProperty *)p)->dims);
    if (((DLiteProperty *)p)->unit) free(((DLiteProperty *)p)->unit);
    if (((DLiteProperty *)p)->description)
      free(((DLiteProperty *)p)->description);
    break;
  case dliteRelation:
    free(((DLiteRelation *)p)->s);
    free(((DLiteRelation *)p)->p);
    free(((DLiteRelation *)p)->o);
    if (((DLiteRelation *)p)->id) free(((DLiteRelation *)p)->id);
    break;
  }
  return memset(p, 0, size);
}


/*
  Serialises data of type `dtype` and size `size` pointed to by `p`.
  The string representation is written to `dest`.  No more than
  `n` bytes are written (incl. the terminating NUL).

  The `width` and `prec` arguments corresponds to the printf() minimum
  field width and precision/length modifier.  If you set them to -1, a
  suitable value will selected according to `type`.  To ignore their
  effect, set `width` to zero or `prec` to -2.

  Returns number of bytes written to `dest`.  If the output is
  truncated because it exceeds `n`, the number of bytes that would
  have been written if `n` was large enough is returned.  On error, a
  negative value is returned.
 */
int dlite_type_snprintf(const void *p, DLiteType dtype, size_t size,
			int width, int prec, char *dest, size_t n)
{
  int m, w=width, r=prec;
  switch (dtype) {
  case dliteBlob:
    return err(-1, "serialising binary blobs is not yet supported");
  case dliteBool:
    m = snprintf(dest, n, "%*.*s", w, r,
                      (*((bool *)p)) ? "true" : "false");
    break;
  case dliteInt:
    if (w == -1) w = 8;
    switch (size) {
#ifdef HAVE_INTTYPES_H
    case 1: m = snprintf(dest, n, "%*.*"PRId8,  w, r, *((int8_t *)p));  break;
    case 2: m = snprintf(dest, n, "%*.*"PRId16, w, r, *((int16_t *)p)); break;
    case 4: m = snprintf(dest, n, "%*.*"PRId32, w, r, *((int32_t *)p)); break;
    case 8: m = snprintf(dest, n, "%*.*"PRId64, w, r, *((int64_t *)p)); break;
#else
    case 1: m = snprintf(dest, n, "%*.*hhd", w, r, *((int8_t *)p));  break;
    case 2: m = snprintf(dest, n, "%*.*hd",  w, r, *((int16_t *)p)); break;
    case 4: m = snprintf(dest, n, "%*.*d",   w, r, *((int32_t *)p)); break;
    case 8: m = snprintf(dest, n, "%*.*ld",  w, r, *((int64_t *)p)); break;
    default: return err(-1, "invalid int size: %zu", size);
#endif
    }
    break;
  case dliteUInt:
    if (w == -1) w = 8;
    switch (size) {
#ifdef HAVE_INTTYPES_H
    case 1: m = snprintf(dest, n, "%*.*"PRIu8,  w, r, *((uint8_t *)p));  break;
    case 2: m = snprintf(dest, n, "%*.*"PRIu16, w, r, *((uint16_t *)p)); break;
    case 4: m = snprintf(dest, n, "%*.*"PRIu32, w, r, *((uint32_t *)p)); break;
    case 8: m = snprintf(dest, n, "%*.*"PRIu64, w, r, *((uint64_t *)p)); break;
#else
    case 1: m = snprintf(dest, n, "%*.*hhu", w, r, *((uint8_t *)p));  break;
    case 2: m = snprintf(dest, n, "%*.*hu",  w, r, *((uint16_t *)p)); break;
    case 4: m = snprintf(dest, n, "%*.*u",   w, r, *((uint32_t *)p)); break;
    case 8: m = snprintf(dest, n, "%*.*lu",  w, r, *((uint64_t *)p)); break;
#endif
    default: return err(-1, "invalid int size: %zu", size);
    }
    break;
  case dliteFloat:
    if (w == -1) w = 12;
    if (r == -1) r = 6;
    switch (size) {
    case 4:  m = snprintf(dest, n, "%*.*g",  w, r, *((float32_t *)p)); break;
    case 8:  m = snprintf(dest, n, "%*.*g",  w, r, *((float64_t *)p)); break;
#ifdef HAVE_FLOAT80
    case 10: m = snprintf(dest, n, "%*.*Lg", w, r, *((float80_t *)p)); break;
#endif
#ifdef HAVE_FLOAT128
    case 16: m = snprintf(dest, n, "%*.*Lg", w, r, *((float128_t *)p)); break;
#endif
    default: return err(-1, "invalid int size: %zu", size);
    }
    break;
  case dliteFixString:
    if (r > (int)size) r = size;
    m = snprintf(dest, n, "%*.*s", w, r, (char *)p);
    break;
  case dliteStringPtr:
    m = snprintf(dest, n, "%*.*s", w, r, *((char **)p));
    break;
  default:
    return err(-1, "serialising dtype \"%s\" is not supported",
	       dtype_enum_names[dtype]);
  }
  return m;
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
                                   "dtype='%s' (%d), size=%zu",
                                   dlite_type_get_dtypename(dtype), dtype,
                                   size);
  }
}

/*
  Returns the amount of padding that should be added before `type`,
  if `type` (of size `size`) is to be added to a struct at offset `offset`.
*/
size_t dlite_type_padding_at(DLiteType dtype, size_t size, size_t offset)
{
  size_t align = dlite_type_get_alignment(dtype, size);
  assert(align);
  return (align - (offset & (align - 1))) & (align - 1);
}


/*
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
