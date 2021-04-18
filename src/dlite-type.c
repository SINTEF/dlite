#include "config.h"

#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "utils/err.h"
#include "utils/integers.h"
#include "utils/floats.h"
#include "utils/boolean.h"
#include "utils/strtob.h"
#include "utils/strutils.h"
#define JSMN_HEADER
#include "utils/jsmn.h"
#include "dlite-entity.h"
#include "dlite-macros.h"
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
    snprintf(typename, n, "string%zu", size-1);
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
  Writes the fortran type name corresponding to `dtype` and `size` to
  `typename`, which must be of size `n`.  Returns non-zero on error.
*/
int dlite_type_set_ftype(DLiteType dtype, size_t size,
                         char *ftype, size_t n)
{
  switch (dtype) {
  case dliteBlob:
    snprintf(ftype, n, "blob");
    break;
  case dliteBool:
    if (size != sizeof(bool))
      return errx(1, "bool should have size %zu, but %zu was provided",
                 sizeof(bool), size);
    snprintf(ftype, n, "logical");
    break;
  case dliteInt:
    snprintf(ftype, n, "integer(%zu)", size);
    break;
  case dliteUInt:
    snprintf(ftype, n, "integer(%zu)", size);
    break;
  case dliteFloat:
    snprintf(ftype, n, "real(%zu)", size);
    break;
  case dliteFixString:
    snprintf(ftype, n, "character(len=%zu)", size-1);
    break;
  case dliteStringPtr:
    snprintf(ftype, n, "character(*)");
    break;
  case dliteDimension:
    snprintf(ftype, n, "type(DLiteDimension)");
    break;
  case dliteProperty:
    snprintf(ftype, n, "type(DLiteProperty)");
    break;
  case dliteRelation:
    snprintf(ftype, n, "type(DLiteRelation)");
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
  Writes the Fortran ISO_C_BINDING type name corresponding to `dtype` and
  `size` to `isoctype`, which must be of size `n`.  Returns non-zero on error.
*/
int dlite_type_set_isoctype(DLiteType dtype, size_t size,
                            char *isoctype, size_t n)
{
  const char* native = dlite_type_get_native_typename(dtype, size);
  switch (dtype) {
  case dliteBlob:
    snprintf(isoctype, n, "blob");
    break;
  case dliteBool:
    if (size != sizeof(bool))
      return errx(1, "bool should have size %zu, but %zu was provided",
                 sizeof(bool), size);
    snprintf(isoctype, n, "logical(c_bool)");
    break;
  case dliteInt:
    snprintf(isoctype, n, "integer(c_%s)", native);
    break;
  case dliteUInt:
    snprintf(isoctype, n, "integer(c_%s)", native);
    break;
  case dliteFloat:
    snprintf(isoctype, n, "real(c_%s)", native);
    break;
  case dliteFixString:
    snprintf(isoctype, n, "character(kind=c_char)");
    break;
  case dliteStringPtr:
    snprintf(isoctype, n, "character(kind=c_char)");
    break;
  case dliteDimension:
    snprintf(isoctype, n, "type(c_ptr)");
    break;
  case dliteProperty:
    snprintf(isoctype, n, "type(c_ptr)");
    break;
  case dliteRelation:
    snprintf(isoctype, n, "type(c_ptr)");
    break;
  default:
    return errx(1, "unknown dtype number: %d", dtype);
  }
  return 0;
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
  Assigns `dtype` and `size` from `typename`.

  Characters other than alphanumerics or underscore may follow the
  type name.

  Returns non-zero on error.
*/
int dlite_type_set_dtype_and_size(const char *typename,
                                  DLiteType *dtype, size_t *size)
{
  int i;
  size_t len=0, namelen, typesize;
  char *endptr;

  while (isalpha(typename[len])) len++;
  namelen = len;
  while (isdigit(typename[len])) len++;
  if (isalpha(typename[len]) || typename[len] == '_')
    return errx(1, "alphabetic characters or underscore cannot follow digits "
                "in type name: %s", typename);


  /* Check if typename is a fixed-sized type listed in the type table */
  for (i=0; type_table[i].typename; i++) {
    if (strncmp(typename, type_table[i].typename, len) == 0) {
      *dtype = type_table[i].dtype;
      *size = type_table[i].size;
      return 0;
    }
  }

  /* Type is not in the type table - it must have a explicit size */
  if (len == namelen)
    return errx(1, "explicit length is expected for type name: %s", typename);

  /* extract size from `typename` */
  typesize = strtol(typename + namelen, &endptr, 10);
  assert(endptr == typename + len);
  if (strncmp(typename, "blob", namelen) == 0) {
    *dtype = dliteBlob;
    *size = typesize;
  } else if (strncmp(typename, "string", namelen) == 0) {
    *dtype = dliteFixString;
    *size = typesize+1;
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
        void *p = realloc(*((void **)dest), len);
        if (!p) return err(1, "allocation failure"), NULL;
        *((void **)dest) = p;
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
        if (!(d->dims = malloc(d->ndims*sizeof(char *))))
          return err(1, "allocation failure"), NULL;
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
  Return a StrquoteFlags corresponding to `flags`.
 */
static StrquoteFlags as_qflags(DLiteTypeFlag flags)
{
  switch (flags) {
  case dliteFlagDefault: return strquoteRaw;
  case dliteFlagRaw: return strquoteRaw;
  case dliteFlagQuoted: return 0;
  case dliteFlagStrip: return strquoteNoQuote | strquoteNoEscape;
  }
  abort();
}


/* Expands to `a - b` if `a > b` else to `0`. */
#define PDIFF(a, b) (((size_t)(a) > (size_t)(b)) ? (a) - (b) : 0)

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
int dlite_type_print(char *dest, size_t n, const void *p, DLiteType dtype,
                     size_t size, int width, int prec,  DLiteTypeFlag flags)
{
  int m=0, w=width, r=prec;
  size_t i;
  StrquoteFlags qflags = as_qflags(flags);
  switch (dtype) {

  case dliteBlob:
    if (!(qflags & strquoteNoQuote)) {
        int v = snprintf(dest+m, PDIFF(n, m), "\"");
        if (v < 0) return err(-1, "error printing initial quote for blob");
        m += v;
      }
    for (i=0; i<size; i++) {
      int v = snprintf(dest+m, PDIFF(n, m), "%02hhx",
                       *((unsigned char *)p+i));
      if (v < 0) return err(-1, "error printing blob");
      m += v;
    }
    if (!(qflags & strquoteNoQuote)) {
        int v = snprintf(dest+m, PDIFF(n, m), "\"");
        if (v < 0) return err(-1, "error printing final quote for blob");
        m += v;
      }
    break;

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
    if (prec > 0 && prec < (int)size) size = prec;
    m = strnquote(dest, n, (char *)p, size, qflags);
    break;

  case dliteStringPtr:
    if (*((char **)p)) {
      size_t len = strlen(*((char **)p));
      if (prec > 0 && prec < (int)len) len = prec;
      m = strnquote(dest, n, *((char **)p), len, qflags);
    } else {
      m = snprintf(dest, n, "%*.*s", w, r, "null");
    }
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
        m += snprintf(dest+m, PDIFF(n, m), ", \"dims\": [");
        for (i=0; i < prop->ndims; i++)
          m += snprintf(dest+m, PDIFF(n, m), "\"%s\"%s", prop->dims[i],
                        (i < prop->ndims-1) ? ", " : "");
        m += snprintf(dest+m, PDIFF(n, m), "]");
      }
      if (prop->unit)
        m += snprintf(dest+m, PDIFF(n, m), ", \"unit\": \"%s\"", prop->unit);
      if (prop->iri)
        m += snprintf(dest+m, PDIFF(n, m), ", \"iri\": \"%s\"", prop->iri);
      if (prop->description)
        m += snprintf(dest+m, PDIFF(n, m), ", \"description\": \"%s\"",
                      prop->description);
      m += snprintf(dest+m, PDIFF(n, m), "}");
    }
    break;

  case dliteRelation:
    {
      DLiteRelation *r = (DLiteRelation *)p;
      m = snprintf(dest, n, "[\"%s\", \"%s\", \"%s\"]", r->s, r->p, r->o);
    }
    break;
  }
  if (m < 0) {
    char buf[32];
    dlite_type_set_typename(dtype, size, buf, sizeof(buf));
    return errx(-1, "error printing type %s", buf);
  }
  return m;
}

/*
  Like dlite_type_print(), but prints to allocated buffer.

  Prints to position `pos` in `*dest`, which should point to a buffer
  of size `*n`.  `*dest` is reallocated if needed.

  Returns number or bytes written or a negative number on error.
 */
int dlite_type_aprint(char **dest, size_t *n, size_t pos, const void *p,
                      DLiteType dtype, size_t size, int width, int prec,
                      DLiteTypeFlag flags)
{
  int m;
  void *ptr;
  size_t newsize;
  assert(dest);
  if (!*dest) *n = 0;
  if (!n) *dest = NULL;
  m = dlite_type_print(*dest + pos, PDIFF(*n, pos), p, dtype, size,
                       width, prec, flags);
  if (m < 0) return m;  /* failure */
  if (m < (int)PDIFF(*n, pos)) return m;  // success, buffer is large enough

  /* Reallocate buffer to required size. */
  newsize = m + pos + 1;
  if (!(ptr = realloc(*dest, newsize))) return -1;
  *dest = ptr;
  *n = newsize;
  m = dlite_type_print(*dest + pos, PDIFF(*n, pos), p, dtype, size,
                       width, prec, flags);
  assert(0 <= m && m < (int)*n);
  return m;
}


/*
  Scans a value from `src` and write it to memory pointed to by `p`.

  If `len` is non-negative, at most `len` bytes are read from `src`.

  The type and size of the scanned data is described by `dtype` and `size`,
  respectively.

  Returns number of characters consumed or -1 on error.
*/
int dlite_type_scan(const char *src, int len, void *p, DLiteType dtype,
                    size_t size, DLiteTypeFlag flags)
{
  size_t i;
  int m=0, v;
  char *endptr;
  StrquoteFlags qflags = as_qflags(flags);
  switch(dtype) {

  case dliteBlob:
    if (!(flags & dliteFlagStrip)) while (isblank(src[m])) m++;
    if (!(flags & dliteFlagQuoted) && src[m++] != '"')
      return errx(-1, "expected initial double quote around blob");
    for (i=0; i<2*size; i++)
      if (!isxdigit(src[i+m]))
        return errx(-1, "invalid character in blob: %c", src[i+m]);
    for (i=0; i<size; i++)
      if (sscanf(src + 2*i+m, "%2hhx", (unsigned char *)p + i) != 1)
        return errx(-1, "error scanning blob: '%.*s'", (int)size, src);
    m += 2*size;
    if (!(flags & dliteFlagQuoted) && src[m++] != '"')
      return errx(-1, "expected final double quote around blob");
    break;

  case dliteBool:
    if ((v = strtob(src, &endptr)) < 0)
      return errx(-1, "invalid bool: '%s'", src);
    *((bool *)p) = v;
    m = endptr - src;
    break;

  case dliteInt:
    errno = 0;
    switch (size) {
#ifdef HAVE_INTTYPES_H
    case 1: v = sscanf(src, "%"SCNi8"%n",  ((int8_t  *)p), &m); break;
    case 2: v = sscanf(src, "%"SCNi16"%n", ((int16_t *)p), &m); break;
    case 4: v = sscanf(src, "%"SCNi32"%n", ((int32_t *)p), &m); break;
    case 8: v = sscanf(src, "%"SCNi64"%n", ((int64_t *)p), &m); break;
#else
    case 1: v = sscanf(src, "%hhi%n", ((int8_t  *)p), &m); break;
    case 2: v = sscanf(src, "%hi%n",  ((int16_t *)p), &m); break;
    case 4: v = sscanf(src, "%i%n",   ((int32_t *)p), &m); break;
    case 8: v = sscanf(src, "%lli%n", ((int64_t *)p), &m); break;
#endif
    default: return err(-1, "invalid int size: %zu", size);
    }
    if (v != 1) return err(-1, "invalid int: '%s'", src);
    break;

  case dliteUInt:
    v = 0;
    while (isblank(src[v])) m++;
    if (src[v] == '0' && (src[v+1] == 'x' || src[v+1] == 'X')) {
      switch (size) {
#ifdef HAVE_INTTYPES_H
      case 1: v = sscanf(src, "%"SCNx8"%n",  ((uint8_t  *)p), &m); break;
      case 2: v = sscanf(src, "%"SCNx16"%n", ((uint16_t *)p), &m); break;
      case 4: v = sscanf(src, "%"SCNx32"%n", ((uint32_t *)p), &m); break;
      case 8: v = sscanf(src, "%"SCNx64"%n", ((uint64_t *)p), &m); break;
#else
      case 1: v = sscanf(src, "%hhx%n", ((uint8_t  *)p), &m); break;
      case 2: v = sscanf(src, "%hx%n",  ((uint16_t *)p), &m); break;
      case 4: v = sscanf(src, "%x%n",   ((uint32_t *)p), &m); break;
      case 8: v = sscanf(src, "%llx%n", ((uint64_t *)p), &m); break;
#endif
      default: return err(-1, "invalid int size: %zu", size);
      }
    } else {
      switch (size) {
#ifdef HAVE_INTTYPES_H
      case 1: v = sscanf(src, "%"SCNu8"%n",  ((uint8_t  *)p), &m); break;
      case 2: v = sscanf(src, "%"SCNu16"%n", ((uint16_t *)p), &m); break;
      case 4: v = sscanf(src, "%"SCNu32"%n", ((uint32_t *)p), &m); break;
      case 8: v = sscanf(src, "%"SCNu64"%n", ((uint64_t *)p), &m); break;
#else
      case 1: v = sscanf(src, "%hhu%n", ((uint8_t  *)p), &m); break;
      case 2: v = sscanf(src, "%hu%n",  ((uint16_t *)p), &m); break;
      case 4: v = sscanf(src, "%u%n",   ((uint32_t *)p), &m); break;
      case 8: v = sscanf(src, "%llu%n", ((uint64_t *)p), &m); break;
#endif
      default: return err(-1, "invalid uint size: %zu", size);
      }
    }
    if (v != 1) return err(-1, "invalid uint: '%s'", src);
    break;

  case dliteFloat:
    switch (size) {
    case 4:  v = sscanf(src, "%f%n",  ((float32_t *)p), &m); break;
    case 8:  v = sscanf(src, "%lf%n", ((float64_t *)p), &m); break;
#ifdef HAVE_FLOAT80
    case 10: v = sscanf(src, "%Lf%n", ((float80_t *)p), &m); break;
#endif
#ifdef HAVE_FLOAT128
    case 16: v = sscanf(src, "%Lf%n", ((float128_t *)p), &m); break;
#endif
    default: return err(-1, "invalid int size: %zu", size);
    }
    if (v != 1) return err(-1, "invalid float: '%s'", src);
    break;

  case dliteFixString:
    switch (strnunquote((char *)p, size, src, len, &m, qflags)) {
    case -1: return errx(-1, "expected initial double quote around string");
    case -2: return errx(-1, "expected final double quote around string");
    }
    ((char *)p)[size-1] = '\0';
    break;

  case dliteStringPtr:
    {
      char *q=NULL;
      int n;
      switch((n = strnunquote(NULL, 0, src, len, &m, qflags))) {
      case -1: return errx(-1, "expected initial double quote around string");
      case -2: return errx(-1, "expected final double quote around string");
      }
      assert(n >= 0);
      if (!(q = realloc(*((char **)p), n+1)))
        return err(-1, "allocation failure");
      n = strunquote(q, n+1, src, NULL, qflags);
      assert(n >= 0);
      *(char **)p = q;
    }
    break;

  case dliteDimension:
    {
      DLiteDimension *dim = p;
      jsmn_parser parser;
      jsmntok_t tokens[5], *t;
      int r;

      if (dim->name) free(dim->name);
      if (dim->description) free(dim->description);
      memset(dim, 0, sizeof(DLiteDimension));

      if (len < 0) len = strlen(src);
      jsmn_init(&parser);
      if ((r = jsmn_parse(&parser, src, len, tokens, 5)) < 0)
        return err(-1, "cannot parse dimension: %s: '%s'",
                   jsmn_strerror(r), src);
      if (tokens->type != JSMN_OBJECT)
        return errx(-1, "dimension should be a JSON object");
      m = tokens->end - tokens->start;

      if (!(t = jsmn_item(src, tokens, "name")))
        return err(-1, "missing dimension name: '%s'", src);
      dim->name = strndup(src + t->start, t->end - t->start);
      if ((t = jsmn_item(src, tokens, "description")))
        dim->description = strndup(src + t->start, t->end - t->start);
    }
    break;

  case dliteProperty:
    {
      DLiteProperty *prop = p;
      jsmn_parser parser;
      jsmntok_t tokens[32], *t, *d;
      int r, i;

      if (prop->name) free(prop->name);
      if (prop->dims) free(prop->dims);
      if (prop->unit) free(prop->unit);
      if (prop->description) free(prop->description);
      memset(prop, 0, sizeof(DLiteProperty));

      if (len < 0) len = strlen(src);
      jsmn_init(&parser);
      if ((r = jsmn_parse(&parser, src, len, tokens, 32)) < 0)
        return err(-1, "cannot parse property: %s: '%s'",
                   jsmn_strerror(r), src);
      if (tokens->type != JSMN_OBJECT)
        return errx(-1, "property should be a JSON object");
      m = tokens->end - tokens->start;

      if (!(t = jsmn_item(src, tokens, "name")))
        return errx(-1, "missing property name: '%s'", src);
      prop->name = strndup(src + t->start, t->end - t->start);

      if (!(t = jsmn_item(src, tokens, "type")))
        return errx(-1, "missing property type: '%s'", src);
      if (dlite_type_set_dtype_and_size(src + t->start,
                                        &prop->type, &prop->size))
        return -1;

      if ((t = jsmn_item(src, tokens, "dims"))) {
        if (t->type != JSMN_ARRAY)
          return errx(-1, "property dims should be an array");
        prop->ndims = t->size;
        prop->dims = calloc(prop->ndims, sizeof(char *));
        for (i=0; i < prop->ndims; i++) {
          if (!(d = jsmn_element(src, t, i)))
            return err(-1, "error parsing property dimensions: %.*s",
                       t->end - t->start, src + t->start);
          prop->dims[i] = strndup(src + d->start, d->end - d->start);
        }
      }

      if ((t = jsmn_item(src, tokens, "unit")))
        prop->unit = strndup(src + t->start, t->end - t->start);

      if ((t = jsmn_item(src, tokens, "description")))
        prop->description = strndup(src + t->start, t->end - t->start);
    }
    break;

  case dliteRelation:
    {
      DLiteRelation *rel = p;
      jsmn_parser parser;
      jsmntok_t tokens[5], *t;
      int r;

      if (rel->s) free(rel->s);
      if (rel->p) free(rel->p);
      if (rel->o) free(rel->o);
      if (rel->id) free(rel->id);
      memset(rel, 0, sizeof(DLiteRelation));

      if (len < 0) len = strlen(src);
      jsmn_init(&parser);
      if ((r = jsmn_parse(&parser, src, len, tokens, 5)) < 0)
        return err(-1, "cannot parse relation: %s: '%s'",
                   jsmn_strerror(r), src);
      if (tokens->type != JSMN_ARRAY)
        return errx(-1, "relation should be a JSON array");
      if (tokens->size < 3 || tokens->size > 4)
        return errx(-1, "relation should have 3 (optionally 4) elements");
      m = tokens->end - tokens->start;

      if (!(t = jsmn_element(src, tokens, 0))) return -1;
      rel->s = strndup(src + t->start, t->end - t->start);
      if (!(t = jsmn_element(src, tokens, 1))) return -1;
      rel->p = strndup(src + t->start, t->end - t->start);
      if (!(t = jsmn_element(src, tokens, 2))) return -1;
      rel->o = strndup(src + t->start, t->end - t->start);
      if (tokens->size > 3 && (t = jsmn_element(src, tokens, 3)))
        rel->id = strndup(src + t->start, t->end - t->start);
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
