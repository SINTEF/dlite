#include <assert.h>
#include <string.h>
#include <stddef.h>

#include "utils/err.h"
#include "integers.h"
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
    snprintf(typename, n, "blob%zd", size);
    break;
  case dliteBool:
    if (size != sizeof(bool))
      return errx(1, "bool should have size %zd, but %zd was provided",
                 sizeof(bool), size);
    snprintf(typename, n, "bool");
    break;
  case dliteInt:
    snprintf(typename, n, "int%zd", size*8);
    break;
  case dliteUInt:
    snprintf(typename, n, "uint%zd", size*8);
    break;
  case dliteFloat:
    snprintf(typename, n, "float%zd", size*8);
    break;
  case dliteFixString:
    snprintf(typename, n, "string%zd", size);
    break;
  case dliteStringPtr:
    if (size != sizeof(char *))
      return errx(1, "string should have size %zd, but %zd was provided",
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
  Writes C declaration to `cdecl` of a C variable with given `dtype` and `size`.
  The size of the memory pointed to by `cdecl` must be at least `n` bytes.

  `name` is the name of the C variable.

  `nref` is the number of extra * to add in front of `name`.

  Returns the number of bytes written or -1 on error.
*/
int dlite_type_set_cdecl(DLiteType dtype, size_t size, const char *name,
                         size_t nref, char *pcdecl, size_t n)
{
  int m;
  char ref[32];

  if (nref >= sizeof(ref))
    return errx(-1, "too many dereferences to write: %zd", nref);
  memset(ref, '*', sizeof(ref));
  ref[nref] = '\0';

  switch (dtype) {
  case dliteBlob:
    m = snprintf(pcdecl, n, "uint8_t %s%s[%zd]", ref, name, size);
    break;
  case dliteBool:
    if (size != sizeof(bool))
      return errx(-1, "bool should have size %zd, but %zd was provided",
                 sizeof(bool), size);
    m = snprintf(pcdecl, n, "bool %s%s", ref, name);
    break;
  case dliteInt:
    m = snprintf(pcdecl, n, "int%zd_t %s%s", size*8, ref, name);
    break;
  case dliteUInt:
    m = snprintf(pcdecl, n, "uint%zd_t %s%s", size*8, ref, name);
    break;
  case dliteFloat:
    m = snprintf(pcdecl, n, "float%zd_t %s%s", size*8, ref, name);
    break;
  case dliteFixString:
    m = snprintf(pcdecl, n, "char %s%s[%zd]", ref, name, size);
    break;
  case dliteStringPtr:
    if (size != sizeof(char *))
      return errx(-1, "string should have size %zd, but %zd was provided",
                 sizeof(char *), size);
    m = snprintf(pcdecl, n, "char *%s%s", ref, name);
    break;
  case dliteDimension:
    if (size != sizeof(DLiteDimension))
      return errx(-1, "DLiteDimension must have size %zd, got %zd",
                  sizeof(DLiteDimension), size);
    m = snprintf(pcdecl, n, "DLiteDimension %s%s", ref, name);
    break;
  case dliteProperty:
    if (size != sizeof(DLiteProperty))
      return errx(-1, "DLiteProperty must have size %zd, got %zd",
                  sizeof(DLiteProperty), size);
    m = snprintf(pcdecl, n, "DLiteProperty %s%s", ref, name);
    break;
  case dliteRelation:
    if (size != sizeof(DLiteRelation))
      return errx(-1, "DLiteRelation must have size %zd, got %zd",
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
      size_t len = strlen(s) + 1;
      *((void **)dest) = realloc(*((void **)dest), len);
      memcpy(*((void **)dest), s, len);
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
                                   "dtype='%s' (%d), size=%zd",
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
