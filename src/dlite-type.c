#include "config.h"

#include <assert.h>
#include <string.h>
#include <stddef.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "utils/err.h"
#include "utils/integers.h"
#include "utils/floats.h"
#include "utils/boolean.h"
#include "dlite-entity.h"
#include "dlite-macros.h"
#include "dlite-type.h"


#define scheck(m) \
  if ((m) < 0) return err(-1, "snprintf failed on %s:%d", __FILE__, __LINE__)


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
  {"single",   dliteFloat,     sizeof(float),         alignof(float)},
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


/*
  Writes Fortran type to `dest` corresponding to `dtype` and `size`.
  At most `n` bytes are written.

  Returns the number of bytes written or -1 on error.
*/
int dlite_type_set_ftype(DLiteType dtype, size_t size, char *dest, size_t n)
{
  int m=0;

  switch (dtype) {
  case dliteInt:
    m = snprintf(dest, n, "integer(%zu)", size);
    break;
  case dliteFloat:
    m = snprintf(dest, n, "real(%zu)", size);
    break;
  case dliteFixString:
    m = snprintf(dest, n, "character(len=%zu)", size-1);
    break;
  case dliteStringPtr:
    m = snprintf(dest, n, "character(len=1)");
    break;
  case dliteDimension:
    m = snprintf(dest, n, "!type(dliteDimension)");
    break;
  case dliteProperty:
    m = snprintf(dest, n, "!type(dliteProperty)");
    break;
  default:
    return errx(-1, "not yet supported dtype: %d", dtype);
  }
  if (m < 0)
    return err(-1, "error writing Fortran type for dtype %d", dtype);
  return m;
}

/*
  Writes Fortran pointer type (for iso_c_binding) to `dest`
  corresponding to `dtype` and `size`.  At most `n` bytes are written.

  Returns the number of bytes written or -1 on error.
*/
int dlite_type_set_fptr(DLiteType dtype, size_t size, char *dest, size_t n)
{
  int m=0;

  switch (dtype) {
  case dliteInt:
    m = snprintf(dest, n, "integer(kind=c_int%zu_t)", 8*size);
    break;
  case dliteFloat:
    m = snprintf(dest, n, "real(kind=c_%s)",
                 dlite_type_get_native_typename(dtype, size));
    break;
  case dliteFixString:
  case dliteStringPtr:
    m = snprintf(dest, n, "character(kind=c_char)");
    break;
  case dliteDimension:
    m = snprintf(dest, n, "!c_ptr");
    break;
  case dliteProperty:
    m = snprintf(dest, n, "!c_ptr");
    break;
  default:
    return errx(-1, "not yet supported dtype: %d", dtype);
  }
  if (m < 0)
    return err(-1, "error writing Fortran pointer for dtype %d", dtype);
  return m;
}

/*
  Writes Fortran type to `dest` for a variable with given `dtype`
  and `size`.  The size of the memory pointed to by `dest` must be
  at least `n` bytes.

  `name` is the name of the Fortran variable.
  `ndims` number of dimensions

  Returns the number of bytes written or -1 on error.
*/
int dlite_type_set_fdecl(DLiteType dtype, size_t size, const char *name,
                         int ndims, char *dest, size_t n)
{
  int i, m, k;
  if ((m = dlite_type_set_ftype(dtype, size, dest, n)) < 0) return -1;
  if (ndims) {
    scheck((k = snprintf(dest+m, n-m, ", allocatable")));
    m += k;
  }
  scheck((k = snprintf(dest+m, n-m, " :: %s", name)));
  m += k;
  if (ndims) {
    scheck((k = snprintf(dest+m, n-m, "(")));
    m += k;
    for (i=0; i<ndims; i++) {
      scheck((k = snprintf(dest+m, n-m, "%s:", i ? "," : "")));
      m += k;
    }
    scheck((k = snprintf(dest+m, n-m, ")")));
    m += k;
  }
  return m;
}

/*
  Writes Fortran pointer type (for iso_c_binding) to `dest` for a
  variable with given `dtype` and `size`.  The size of the memory
  pointed to by `dest` must be at least `n` bytes.

  `name` is the name of the Fortran variable. A "_f" will be appended to it.
  `ndims` number of dimensions

  Returns the number of bytes written or -1 on error.
*/
int dlite_type_set_fptrdecl(DLiteType dtype, size_t size, const char *name,
                         int ndims, char *dest, size_t n)
{
  int i, k, m, nref = ndims;
  if (dtype == dliteFixString || dtype == dliteStringPtr) nref++;
  if ((m = dlite_type_set_fptr(dtype, size, dest, n)) < 0) return -1;
  scheck((k = snprintf(dest+m, n-m, ", pointer :: %s_f", name)));
  m += k;
  if (nref) {
    scheck((k = snprintf(dest+m, n-m, "(")));
    m += k;
    for (i=0; i<nref; i++) {
      scheck((k = snprintf(dest+m, n-m, "%s:", i ? "," : "")));
      m += k;
    }
    scheck((k = snprintf(dest+m, n-m, ")")));
    m += k;
  }
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
  namelen = strcspn(typename, "0123456789");
  typesize = strtol(typename + namelen, &endptr, 10);
  if (endptr <= typename + namelen) {
    if (strcmp(typename, "blob") == 0 ||
        strcmp(typename, "string") == 0)
      return err(1, "explicit length is expected for type name: %s", typename);
    return err(1, "unexpected type name: %s", typename);
  }
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
        int i;
        d->dims = malloc(d->ndims*sizeof(char *));
        for (i=0; i<d->ndims; i++)
          d->dims[i] = strdup(s->dims[i]);
      } else {
        d->dims = NULL;
      }
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
    if (((DLiteProperty *)p)->dims) {
      int i;
      for (i=0; i < ((DLiteProperty *)p)->ndims; i++)
        if (((DLiteProperty *)p)->dims[i])
          free(((DLiteProperty *)p)->dims[i]);
      free(((DLiteProperty *)p)->dims);
    }
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
    case 8: m = snprintf(dest, n, "%*.*lld", w, r, *((int64_t *)p)); break;
#endif
    default: return err(-1, "invalid int size: %zu", size);
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
    case 8: m = snprintf(dest, n, "%*.*llu", w, r, *((uint64_t *)p)); break;
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
  case dliteDimension:
    m = snprintf(dest, n, "{\"name\": \"%s\", \"description\": \"%s\"}",
                 ((DLiteDimension *)p)->name,
                 ((DLiteDimension *)p)->description);
    break;
  case dliteProperty:
    {
      int i;
      char typename[32];
      DLiteProperty *prop = (DLiteProperty *)p;
      dlite_type_set_typename(prop->type, prop->size, typename,
                              sizeof(typename));
      m = snprintf(dest, n, "{"
                   "\"name\": \"%s\", "
                   "\"type\": \"%s\", "
                   "\"ndims\": %d",
                   prop->name, typename, prop->ndims);
      if (prop->ndims) {
        m += snprintf(dest+m, n-m, ", [");
        for (i=0; i < prop->ndims; i++)
          m += snprintf(dest+m, n-m, "\"%s\"%s", prop->dims[i],
                        (i < prop->ndims-1) ? ", " : "");
        m += snprintf(dest+m, n-m, "]");
      }
      if (prop->unit)
        m += snprintf(dest+m, n-m, ", \"unit\": \"%s\"", prop->unit);
      if (prop->iri)
        m += snprintf(dest+m, n-m, ", \"iri\": \"%s\"", prop->iri);
      if (prop->description)
        m += snprintf(dest+m, n-m, ", \"description\": \"%s\"",
                      prop->description);
      m += snprintf(dest+m, n-m, "}");
    }
    break;
  case dliteRelation:
    {
      DLiteRelation *r = (DLiteRelation *)p;
      m = snprintf(dest, n, "[\"%s\", \"%s\", \"%s\"]", r->s, r->p, r->o);
    }
    break;
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
  default:              return err(1, "cannot determine alignment of "
                                   "dtype='%s' (%d), size=%zu",
                                   dlite_type_get_dtypename(dtype), dtype,
                                   size), 0;
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
  Returns the offset the current struct member with dtype `dtype` and
  size `size`.  The offset of the previous struct member is `prev_offset`
  and its size is `prev_size`.

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


/*
  Copies n-dimensional array `src` to `dest` by calling `castfun` on
  each element.  `dest` must have sufficient size to hold the result.

  If either `dest_strides` or `src_strides`  are NULL, the memory is
  assumed to be C-contiguous.

  This function is rather general function that allows `src` and
  `dest` to have different type and memory layout.  By e.g. inverting
  the order of `dest_dims` and `dest_strides` you can copy an
  n-dimensional array from C to Fortran order.

  Arguments:
    - ndims: Number of dimensions for both source and destination.
          Zero means scalar.
    - dest: Pointer to destination memory.  It must be large enough.
    - dest_type: Destination data type
    - dest_size: Size of each element in destination
    - dest_dims: Destination dimensions.  Length: `ndims`, required if ndims > 0
    - dest_strides: Destination strides.  Length: `ndims`, optional
    - src: Pointer to source memory.
    - src_type: Source data type.
    - src_size: Size of each element in source.
    - src_dims: Source dimensions.  Length: `ndims`, required if ndims > 0
    - src_strides: Source strides.  Length: `ndims`, optional
    - castfun: Function that is doing the actually casting. Called on each
          element.

  Returns non-zero on error.

  TODO
  Allow different number of dimensions in `src` and `dest` as long as
  the total number of elements are equal.
*/
int dlite_type_ndcast(int ndims,
                      void *dest, DLiteType dest_type, size_t dest_size,
                      const size_t *dest_dims, const int *dest_strides,
                      const void *src, DLiteType src_type, size_t src_size,
                      const size_t *src_dims, const int *src_strides,
                      DLiteTypeCast castfun)
{
  int i, retval=1, samelayout=1, *sstrides=NULL, *dstrides=NULL;
  size_t *sidx=NULL, *didx=NULL;
  size_t j, n, N=1;

  assert(src);
  assert(dest);
  if (!castfun) castfun = dlite_type_copy_cast;

  /* Scalar */
  if (ndims == 0)
    return castfun(dest, dest_type, dest_size, src, src_type, src_size);

  assert(src_dims);
  assert(dest_dims);

  /* Total number of elements: N */
  for (i=0, n=1; i<ndims; i++) {
    N *= src_dims[i];
    n *= dest_dims[i];
  }
  if (n != N)
    return err(1, "incompatible sizes of source (%lu) and dest (%lu)", N, n);

  /* Default source strides */
  if (!src_strides) {
    size_t size = src_size;
    if (!(sstrides = calloc(ndims, sizeof(size_t)))) FAIL("allocation failure");
    for (i=ndims-1; i >= 0; i--) {
      sstrides[i] = size;
      size *= src_dims[i];
    }
    src_strides = sstrides;
  }

  /* Default dest strides */
  if (!dest_strides) {
    size_t size = dest_size;
    if (!(dstrides = calloc(ndims, sizeof(size_t)))) FAIL("allocation failure");
    for (i=ndims-1; i >= 0; i--) {
      dstrides[i] = size;
      size *= dest_dims[i];
    }
    dest_strides = dstrides;
  }

  /* -- check that source and dest have same layout */
  if (dest_type != src_type || dest_size != src_size) samelayout = 0;
  if (samelayout) {
    for (i=0; i<ndims; i++)
      if (dest_dims[i] != src_dims[i] || dest_strides[i] != src_strides[i]) {
        samelayout = 0;
        break;
      }
  }
  if (samelayout) {
    /* -- check that source is contiguous */
    int size = src_size;
    for (i=0; i<ndims; i++) {
      int iscont=0;
      for (j=0; j < src_dims[j]; j++)
        if (src_strides[j] == size) { iscont = 1; break; }
      if (!iscont) { samelayout = 0; break; }
      size *= src_dims[i];
    }
  }

  if (samelayout) {
    /* Special case: if source and dest have same layout and are
       contiguous, copy all data in one chunck */
    memcpy(dest, src, N * src_size);

  } else {
    /* General case: copy all elements individually using castfun()

       We make a single loop over the total number of elements.  The
       current index in each dimension in `src` and `dest` are stored
       in `sidx` and `didx`, respectively.
    */
    size_t M=ndims-1;
    const char *sp = src;  /* pointer to current element in `src` */
    char *dp = dest;       /* pointer to current element in `dest` */
    if (!(sidx = calloc(ndims, sizeof(size_t)))) FAIL("allocation failure");
    if (!(didx = calloc(ndims, sizeof(size_t)))) FAIL("allocation failure");

    n = 0;
    while (1) {
      if (castfun(dp, dest_type, dest_size, sp, src_type, src_size)) goto fail;

      if (++n >= N) break;

      /* update src pointer and index */
      if (++sidx[M] < src_dims[M]) {
        sp += src_strides[M];
      } else {
        sidx[M] = 0;
        for (i=M-1; i>=0; i--) {
          if (++sidx[i] < src_dims[i]) break;
          sidx[i] = 0;
        }
        for (j=0, sp=src; j<M; j++)
          sp += src_strides[j] * sidx[j];
      }

      /* update dest pointer and index */
      if (++didx[M] < dest_dims[M]) {
        dp += dest_strides[M];
      } else {
        didx[M] = 0;
        for (i=M-1; i>=0; i--) {
          if (++didx[i] < dest_dims[i]) break;
          didx[i] = 0;
        }
        for (j=0, dp=dest; j<M; j++)
          dp += dest_strides[j] * didx[j];
      }
    }
  }
  retval = 0;
 fail:
  if (sidx) free(sidx);
  if (didx) free(didx);
  if (sstrides) free(sstrides);
  if (dstrides) free(dstrides);
  return retval;
}
