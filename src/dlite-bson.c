#include "config.h"

#include "string.h"

#include "utils/integers.h"
#include "utils/floats.h"
#include "utils/byteorder.h"
#include "utils/err.h"
#include "utils/bson.h"

#include "dlite-entity.h"
#include "dlite-macros.h"
#include "dlite-errors.h"
#include "dlite-type.h"
#include "dlite-bson.h"



#define APPEND(buf, type, ename, size, data)                            \
  do {                                                                  \
    int m = bson_append(buf, bufsize-n, type, ename, size, data);       \
    if (m < 0) return m;                                                \
    n += m;                                                             \
  } while (0)

#define APPEND_PROPERTY(buf, p, shape, ptr)                              \
  do {                                                                  \
    int m = append_property(buf, bufsize-n, p, shape, ptr);              \
    if (m < 0) return m;                                                \
    n += m;                                                             \
  } while (0)

#define BEGIN_SUBDOC(buf, ename, subdoc)                                \
  do {                                                                  \
    int m = bson_begin_subdoc(buf, bufsize-n, ename, subdoc);           \
    if (m < 0) return m;                                                \
    n += m;                                                             \
  } while (0)

#define END_SUBDOC(buf, type)                                           \
  do {                                                                  \
    int m = bson_end_subdoc(buf, bufsize-n, type);                      \
    if (m < 0) return m;                                                \
    n += m;                                                             \
  } while (0)


/* Returns BSON type corresponding to DLite type for scalar data or
   a negative error code on error. */
static BsonType bsontype(DLiteType dtype, size_t size)
{
  switch (dtype) {
  case dliteBlob:      return bsonBinary;
  case dliteBool:      return bsonBool;
  case dliteInt:
    if (size <= 4)     return bsonInt32;
    if (size <= 8)     return bsonInt64;
    return err(dliteValueError, "unsupported integer size: %d", (int)size);
  case dliteUInt:
    if (size < 4)      return bsonInt32;
    if (size <= 8)     return bsonUInt64;
    return err(dliteValueError, "unsupported uint size: %d", (int)size);
  case dliteFloat:
    if (size <= 8)     return bsonDouble;
#ifdef HAVE_FLOAT128
    if (size <= 16)    return bsonDecimal128;
#endif
    return err(dliteValueError, "unsupported float size: %d", (int)size);
  case dliteFixString: return bsonString;
  case dliteStringPtr: return bsonString;
  case dliteRef:       return bsonString;
  case dliteDimension: return bsonDocument;
  case dliteProperty:  return bsonDocument;
  case dliteRelation:  return bsonDocument;
  }
  return err(dliteTypeError, "invalid dlite type number: %d", dtype);
}


/*
  Append DLite property to BSON document.

  Arrays are serialised as binary blobs in host byte order.

  Arguments:
    - buf: Pointer to a BSON document to append data to.  The memory pointed
        to must have been initialised with bson_init_document().
    - bufsize: Size of memory segment pointed to by `buf`.  No more than
        `bufsize` bytes will be written.
    - p: Property to append.
    - shape: Values of property dimensions.
    - ptr: Pointer to data to serialise.

  Returns:
    Number of bytes appended (or would have been appended) to `buf`.
    A negative error code is returned on error.
 */
static int append_property(unsigned char *buf, int bufsize,
                           DLiteProperty *p, size_t *shape, void *ptr)
{
  int n=0;
  int32_t i32;
  int64_t i64;
  uint64_t u64;
  float64_t f64;
#ifdef HAVE_FLOAT128
  float128_t f128;
#endif

  if (p->shape) {

    /* Array - treated as binary using host byte order */
    int i, nmemb=1;
    for (i=0; i < p->ndims; i++) nmemb *= shape[i];
    switch (p->type) {
    case dliteBlob:
    case dliteBool:
    case dliteInt:
    case dliteUInt:
    case dliteFloat:
    case dliteFixString:
      APPEND(buf, bsonBinary, p->name, p->size*nmemb, ptr);
      break;

    case dliteStringPtr:
      {
        unsigned char *subdoc;
        int m;
        if ((m = bson_begin_binary(buf, bufsize-n, p->name, &subdoc)) < 0)
          return m;
        n += m;
        for (i=0; i<nmemb; i++) {
          char *s = ((char **)ptr)[i];
          if ((m = bson_append_binary(subdoc, bufsize-n, strlen(s)+1, s)) < 0)
            return m;
          n += m;
        }
        if ((m = bson_end_binary(buf, bufsize-n)) < 0) return m;
      }
      break;

    case dliteRef:
      {
        unsigned char *subdoc;
        int m;
        if ((m = bson_begin_binary(buf, bufsize-n, p->name, &subdoc)) < 0)
          return m;
        n += m;
        for (i=0; i<nmemb; i++) {
          char *uuid = ((DLiteInstance **)ptr)[i]->uuid;
          if ((m = bson_append_binary(subdoc, bufsize-n, DLITE_UUID_LENGTH+1,
                                      uuid)) < 0) return m;
          n += m;
        }
        if ((m = bson_end_binary(buf, bufsize-n)) < 0) return m;
      }
      break;

    case dliteDimension:
    case dliteProperty:
      return errx(dliteUnsupportedError, "unsupported dlite type for bson: %s",
                  dlite_type_get_dtypename(p->type));

    case dliteRelation:
      for (i=0; i<nmemb; i++) {
        unsigned char *subdoc;
        DLiteRelation rel = *((DLiteRelation *)ptr + i);
        BEGIN_SUBDOC(buf, p->name, &subdoc);
        APPEND(subdoc, bsonString, "s", -1, rel.s);
        APPEND(subdoc, bsonString, "p", -1, rel.p);
        APPEND(subdoc, bsonString, "o", -1, rel.o);
        END_SUBDOC(buf, bsonDocument);
      }
      break;
    }
  } else {

    /* Scalar - expressed in BSON */
    switch (p->type) {

    case dliteBlob:
      APPEND(buf, bsonBinary, p->name, p->size, ptr);
      break;

    case dliteBool:
      APPEND(buf, bsonBool, p->name, p->size, ptr);
      break;

    case dliteInt:
      switch (p->size) {
      case 1: i32 = *((int8_t *)ptr);  break;
      case 2: i32 = *((int16_t *)ptr); break;
      case 4: i32 = *((int32_t *)ptr); break;
      case 8: i64 = *((int64_t *)ptr); break;
      default:
        return errx(dliteValueError, "invalid integer size: %d", (int)p->size);
      }
      if (p->size <= 4)
        APPEND(buf, bsonInt32, p->name, -1, &i32);
      else
        APPEND(buf, bsonInt64, p->name, -1, &i64);
      break;

    case dliteUInt:
      switch (p->size) {
      case 1: i32 = *((uint8_t *)ptr);  break;
      case 2: i32 = *((uint16_t *)ptr); break;
      case 4: u64 = *((uint32_t *)ptr); break;
      case 8: u64 = *((uint64_t *)ptr); break;
      default:
        return errx(dliteValueError, "invalid integer size: %d", (int)p->size);
      }
      if (p->size < 4)
        APPEND(buf, bsonInt32, p->name, -1, &i32);
      else
        APPEND(buf, bsonUInt64, p->name, -1, &u64);
      break;

    case dliteFloat:
      switch (p->size) {
      case 4: f64 = *((float32_t *)ptr); break;
      case 8: f64 = *((float64_t *)ptr); break;
#ifdef HAVE_FLOAT128
#ifdef HAVE_FLOAT80
      case 10: f128 = *((float80_t *)ptr); break;
#endif
#ifdef HAVE_FLOAT96
      case 12: f128 = *((float96_t *)ptr); break;
#endif
      case 16: f128 = *((float128_t *)ptr); break;
#endif /* HAVE_FLOAT128 */
      default:
        return errx(dliteValueError, "invalid float size: %d", (int)p->size);
      }
      if (p->size <= 8)
        APPEND(buf, bsonDouble, p->name, -1, &f64);
#ifdef HAVE_FLOAT128
      else
        APPEND(buf, bsonDecimal128, p->name, -1, &f128);
#endif
      break;

    case dliteFixString:
      APPEND(buf, bsonString, p->name, p->size, ptr);
      break;

    case dliteStringPtr:
      APPEND(buf, bsonString, p->name,
             strlen(*((char **)ptr)), *((char **)ptr));
      break;

    case dliteRef:
      APPEND(buf, bsonString, p->name, p->size,
             (*((DLiteInstance **)ptr))->uuid);
      break;

    case dliteDimension:
    case dliteProperty:
      return errx(dliteUnsupportedError, "unsupported dlite type for bson: %s",
                  dlite_type_get_dtypename(p->type));

    case dliteRelation:
      {
        unsigned char *subdoc;
        DLiteRelation rel = *((DLiteRelation *)ptr);
        BEGIN_SUBDOC(buf, p->name, &subdoc);
        APPEND(subdoc, bsonString, "s", -1, rel.s);
        APPEND(subdoc, bsonString, "p", -1, rel.p);
        APPEND(subdoc, bsonString, "o", -1, rel.o);
        END_SUBDOC(buf, bsonDocument);
      }
    }
  }
  return n;
}


/*
  Append instance to BSON document.

  Arguments:
    - buf: Pointer to a BSON document to append data to.  The memory pointed
        to must have been initialised with bson_init_document().
    - bufsize: Size of memory segment pointed to by `buf`.  No more than
        `bufsize` bytes will be written.
    - inst: DLite instance to append to BSON document.

  Returns:
    Number of bytes appended (or would have been appended) to `buf`.
    A negative error code is returned on error.
 */
int dlite_bson_append_instance(unsigned char *buf, int bufsize,
                               const DLiteInstance *inst)
{
  size_t i;
  int n=0, ismeta=dlite_instance_is_meta(inst);
  unsigned char *subdoc, byteordering[]="\x01\x02\x03\x04";

  APPEND(buf, bsonString, "uuid", DLITE_UUID_LENGTH, inst->uuid);
  if (inst->uri)
    APPEND(buf, bsonString, "uri", -1, inst->uri);
  APPEND(buf, bsonString, "meta", -1, inst->meta->uri);
  if (inst->_parent) {
    BEGIN_SUBDOC(buf, "parent", &subdoc);
    APPEND(subdoc, bsonString, "uuid", DLITE_UUID_LENGTH, inst->_parent->uuid);
    APPEND(subdoc, bsonBinary, "hash", DLITE_HASH_SIZE, inst->_parent->hash);
    END_SUBDOC(buf, bsonDocument);
  }

  /* Include host byte order. Since arrays are serialised in host byte order,
     this makes it possible for the reader to determine whether to byteswap
     array data. */
  APPEND(buf, bsonString, "byteorder", -1,
         (*((uint32_t *)&byteordering) == 0x04030201) ? "LE" : "BE");

  if (ismeta) {
    /* metadata */
    DLiteMeta *meta = (DLiteMeta *)inst;
    char *descr = *((char **)dlite_instance_get_property(inst, "description"));
    if (descr) APPEND(buf, bsonString, "description", -1, descr);

    BEGIN_SUBDOC(buf, "dimension_values", &subdoc);
    for (i=0; i < inst->meta->_ndimensions; i++) {
      int32_t v = DLITE_DIM(inst, i);
      APPEND(subdoc, bsonInt32, inst->meta->_dimensions[i].name, 4, &v);
    }
    END_SUBDOC(buf, bsonDocument);

    BEGIN_SUBDOC(buf, "dimensions", &subdoc);
    for (i=0; i < meta->_ndimensions; i++) {
      DLiteDimension *d = meta->_dimensions + i;
      APPEND(subdoc, bsonString, d->name, -1, d->description);
    }
    END_SUBDOC(buf, bsonDocument);

    BEGIN_SUBDOC(buf, "properties", &subdoc);
    for (i=0; i < meta->_nproperties; i++) {
      unsigned char *prop;
      DLiteProperty *p = meta->_properties + i;
      char typename[32];
      dlite_type_set_typename(p->type, p->size, typename, sizeof(typename));
      BEGIN_SUBDOC(subdoc, p->name, &prop);
      APPEND(prop, bsonString, "type", -1, typename);
      if (p->ref)
        APPEND(prop, bsonString, "$ref", -1, p->ref);
      if (p->ndims) {
        unsigned char *arr;
        BEGIN_SUBDOC(prop, "shape", &arr);
        int j;
        for (j=0; j < p->ndims; j++) {
          char index[20];
          snprintf(index, sizeof(index), "%d", j);
          APPEND(arr, bsonString, index, -1, p->shape[j]);
        }
        END_SUBDOC(prop, bsonArray);
      }
      if (p->unit && *p->unit)
        APPEND(prop, bsonString, "unit", -1, p->unit);
      if (p->description && *p->description)
        APPEND(prop, bsonString, "description", -1, p->description);
      END_SUBDOC(subdoc, bsonDocument);
    }
    END_SUBDOC(buf, bsonDocument);

  } else {
    /* data */
    BEGIN_SUBDOC(buf, "dimensions", &subdoc);
    for (i=0; i < inst->meta->_ndimensions; i++) {
      int32_t v = DLITE_DIM(inst, i);
      APPEND(subdoc, bsonInt32, inst->meta->_dimensions[i].name, 4, &v);
    }
    END_SUBDOC(buf, bsonDocument);

    BEGIN_SUBDOC(buf, "properties", &subdoc);
    for (i=0; i < inst->meta->_nproperties; i++) {
      DLiteProperty *p = inst->meta->_properties + i;
      size_t *shape = DLITE_PROP_DIMS(inst, i);
      void *ptr = dlite_instance_get_property_by_index(inst, i);
      APPEND_PROPERTY(subdoc, p, shape, ptr);
    }
    END_SUBDOC(buf, bsonDocument);
  }

  return n;
}


/*
  Serialise instance to BSON and return a pointer to a newly allocated
  memory region with the BSON content.

  If `size` is not NULL, it is assigned to the length of the BSON content.

  Returns NULL on error.
*/
unsigned char *dlite_bson_from_instance(const DLiteInstance *inst,
                                        size_t *size)
{
  unsigned char *doc=NULL;
  int n, m, bufsize=0;
  if ((n = bson_init_document(doc, bufsize)) < 0) goto fail;
  if ((m = dlite_bson_append_instance(doc, bufsize, inst)) < 0) goto fail;
  bufsize = n + m;
  if (!(doc = malloc(bufsize)))
    FAILCODE(dliteMemoryError, "allocation failure");
  if (bson_init_document(doc, bufsize) < 0) goto fail;
  if (dlite_bson_append_instance(doc, bufsize, inst) < 0) goto fail;
  if (size) *size = bufsize;
  return doc;
 fail:
  if (doc) free(doc);
  return NULL;
}


/*-------------------------------------------------------
 * Help functions for loading BSON
 *-------------------------------------------------------*/

#define TYPECHECK(_name, _type)                                         \
  if (type != _type)                                                    \
    return err(dliteTypeError, _name " property should be '" # _type    \
               "', got '%s'", bson_typename(type))


/* Read relations from `subdoc` and write them to `d` of length `len`.
   Returns non-zero on error. */
static int parse_relations(const unsigned char *subdoc, DLiteRelation *rel,
                            int len)
{
  unsigned char *buf, *endptr=NULL;
  char *ename;
  int type, i=0;
  while ((type = bson_parse(subdoc, &ename, (void **)&buf, NULL, &endptr))) {
    if (i++ >= len)
      return err(dliteIndexError, "too many relations in bson, expected %d",
                 len);
    if (type != bsonDocument)
      return err(dliteTypeError, "bson relations should be document, got %s",
                 bson_typename(type));
    unsigned char *ep=NULL;
    int e;
    char *s, *p, *o;
    if ((e = bson_parse(buf, NULL, (void **)&s, NULL, &ep)) < 0) return e;
    if ((e = bson_parse(buf, NULL, (void **)&p, NULL, &ep)) < 0) return e;
    if ((e = bson_parse(buf, NULL, (void **)&o, NULL, &ep)) < 0) return e;
    rel->s = strdup(s);
    rel->p = strdup(p);
    rel->o = strdup(o);
    rel++;
  }
  if (i != len) return err(dliteIndexError, "too few relations in bson, "
                           "got  %d, expected %d", i, len);
  return 0;
}


/* Read `subdoc` and assign metadata dimensions. Return non-zero on error. */
static int set_meta_dimensions(DLiteMeta *meta, unsigned char *subdoc)
{
  unsigned char *endptr=NULL;
  char *ename, *val;
  int type;
  size_t ndims=0;
  DLiteDimension *d = meta->_dimensions;
  while ((type = bson_parse(subdoc, &ename, (void **)&val, NULL, &endptr))) {
    TYPECHECK("dimension", bsonString);
    if (ndims++ >= meta->_ndimensions)
      return err(dliteIndexError, "too many dimensions in bson, expected %d",
                 (int)meta->_ndimensions);
    d->name = strdup(ename);
    d->description = strdup(val);
    d++;
  }
  if (ndims != meta->_ndimensions)
    return err(dliteIndexError, "too few dimensions in bson, got  %d, "
               "expected %d", (int)ndims, (int)meta->_ndimensions);
  return 0;
}

/* Read `subdoc` and assign metadata properties. Return non-zero on error. */
 static int set_meta_properties(DLiteMeta *meta, unsigned char *subdoc)
{
  unsigned char *buf, *endptr=NULL;
  char *ename;
  int type;
  size_t nprops=0;
  DLiteProperty *p = meta->_properties;
  while ((type = bson_parse(subdoc, &ename, (void **)&buf, NULL, &endptr))) {
    unsigned char *bufptr=NULL;
    char *value;
    TYPECHECK("property", bsonDocument);
    if (nprops++ >= meta->_nproperties)
      return err(dliteIndexError, "too many properties in bson, expected %d",
                 (int)meta->_nproperties);
    p->name = strdup(ename);
    while ((type = bson_parse(buf, &ename, (void **)&value, NULL, &bufptr))) {
      if (strcmp(ename, "type") == 0) {
        TYPECHECK("type", bsonString);
        dlite_type_set_dtype_and_size(value, &(p->type), &(p->size));
      } else if (strcmp(ename, "shape") == 0) {
        unsigned char *b=(unsigned char *)value, *ep=NULL;
        char *v;
        int ndims, i=0;
        TYPECHECK("shape", bsonArray);
        if ((ndims = bson_nelements(b)) < 0) return ndims;
        p->shape = calloc(ndims, sizeof(char *));
        while ((type = bson_parse(b, NULL, (void **)&v, NULL, &ep)))
          p->shape[i++] = strdup(v);
        p->ndims = ndims;
      } else if (strcmp(ename, "unit") == 0) {
        TYPECHECK("unit", bsonString);
        p->unit = strdup(value);
      } else if (strcmp(ename, "description") == 0) {
        TYPECHECK("description", bsonString);
        p->description = strdup(value);
      }
    }
    p++;
  }
  if (nprops != meta->_nproperties)
    return err(dliteIndexError, "too few properties in bson, got  %d, "
               "expected %d", (int)nprops, (int)meta->_nproperties);
  return 0;
}


/*
  Set array property `idx` of `inst` from `data`.
  If `byteswap` is non-zero, the data will by byteswapped.

  Returns non-zero on error.
 */
static int set_array_property(DLiteInstance *inst, int idx, void *data,
                              int byteswap)
{
   int i, stat, nmemb=1;
  void *ptr = dlite_instance_get_property_by_index(inst, idx);
  DLiteProperty *p = DLITE_PROP_DESCR(inst, idx);
  assert(p->ndims);
 size_t *shape = DLITE_PROP_DIMS(inst, idx);
  for (i=0; i < p->ndims; i++) nmemb *= shape[i];
  switch (p->type) {
  case dliteBlob:
  case dliteBool:
  case dliteFixString:
  case dliteRef:
    dlite_instance_set_property_by_index(inst, idx, data);
    break;

  case dliteInt:
  case dliteUInt:
  case dliteFloat:
    dlite_instance_set_property_by_index(inst, idx, data);
    if (byteswap) {
      char *q = ptr;
      for (i=0; i<nmemb; i++, q += p->size) {
        switch (p->size) {
        case 1: break;
        case 2: *((uint16_t *)q) = bswap_16(*((uint16_t *)q)); break;
        case 4: *((uint32_t *)q) = bswap_32(*((uint32_t *)q)); break;
        case 8: *((uint64_t *)q) = bswap_64(*((uint64_t *)q)); break;
#ifdef HAVE_BSWAP_128
        case 16: *((uint128_t *)q) = bswap_128(*((uint128_t *)q)); break;
#endif
        default:
          warnx("cannot byteswap property '%s' with type %s and size %d",
                p->name, dlite_type_get_dtypename(p->type), (int)p->size);
        }
      }
    }
    break;

  case dliteStringPtr:
    {
      char **v = ptr;
      char *s = *((char **)data);
      for (i=0; i < nmemb; i++) {
        int n = strlen(s);
        v[i] = strdup(s);
        s += n+1;
      }
    }
    break;

  case dliteDimension:
  case dliteProperty:
    return err(dliteInconsistentDataError, "data instance should not "
               "have a property of type: %s",
               dlite_type_get_enum_name(p->type));

  case dliteRelation:
    if ((stat = parse_relations(data, ptr, nmemb))) return stat;
    break;
  }
  return 0;
}


/*
  Set scalar property `idx` of `inst` from `data` (of BSON type `btype`).
  If `byteswap` is non-zero, the data will by byteswapped.

  Returns non-zero on error.
 */
static int set_scalar_property(DLiteInstance *inst, int idx, void *data)
{
  int stat;
  int32_t i32;
  int64_t i64;
  uint64_t u64;
  float64_t f64;
#ifdef HAVE_FLOAT128
  float128_t f128;
#endif
  void *ptr = dlite_instance_get_property_by_index(inst, idx);
  DLiteProperty *p = DLITE_PROP_DESCR(inst, idx);
  int btype = bsontype(p->type, p->size);
  switch (p->type) {
  case dliteBlob:
  case dliteBool:
  case dliteFixString:
  case dliteRef:
    dlite_instance_set_property_by_index(inst, idx, data);
    break;

  case dliteInt:
    if (btype == bsonInt32)
      i32 = le32toh(*((int32_t *)data));
    else
      i64 = le64toh(*((int64_t *)data));
    switch (p->size) {
    case 1: *((int8_t *)ptr)  = i32; break;
    case 2: *((int16_t *)ptr) = i32; break;
    case 4: *((int32_t *)ptr) = i32; break;
    case 8: *((int64_t *)ptr) = i64; break;
    }
    break;

  case dliteUInt:
    if (btype == bsonInt32)
      i32 = le32toh(*((int32_t *)data));
    else
      u64 = le64toh(*((uint64_t *)data));
    switch (p->size) {
    case 1: *((uint8_t *)ptr)  = i32; break;
    case 2: *((uint16_t *)ptr) = i32; break;
    case 4: *((uint32_t *)ptr) = i32; break;
    case 8: *((uint64_t *)ptr) = u64; break;
    }
    break;

  case dliteFloat:
    if (btype == bsonDouble)
      f64 = le64toh(*((float64_t *)data));
#ifdef HAVE_FLOAT128
    else
      u64 = le64toh(*((uint64_t *)data));
#endif
    switch (p->size) {
    case 4: *((float32_t *)ptr)   = f64; break;
    case 8: *((float64_t *)ptr)   = f64; break;
#ifdef HAVE_FLOAT128
#ifdef HAVE_FLOAT80
    case 10: *((float80_t *)ptr)  = f128; break;
#endif
#ifdef HAVE_FLOAT96
    case 12: *((float96_t *)ptr)  = f128; break;
#endif
    case 16: *((float128_t *)ptr) = f128; break;
#endif  /* HAVE_FLOAT128 */
    }
    break;

  case dliteStringPtr:
    *((char **)ptr) = strdup(data);
    break;
  case dliteDimension:
  case dliteProperty:
    return err(dliteInconsistentDataError, "data instance should not "
               "have a property of type: %s",
               dlite_type_get_enum_name(p->type));
  case dliteRelation:
    if ((stat = parse_relations(data, ptr, 1))) return stat;
    break;
  }
  return 0;
}



/*
  Create a new instance from bson document and return it.
  Returns NULL on error.
*/
DLiteInstance *dlite_bson_load_instance(const unsigned char *doc)
{
  const char *metaid, *uuid, *uri, *id, *byteorder;
  int i, type, idx, ndims, datasize, byteswap=0;
  unsigned char *subdoc, *endptr;
  char *ename;
  void *data;
  size_t *shape=NULL;
  DLiteInstance *inst=NULL;
  if (!(metaid = bson_scan_string(doc, "meta", NULL))) goto fail;
  uuid = bson_scan_string(doc, "uuid", NULL);
  uri = bson_scan_string(doc, "uri", NULL);

  /* Check whether arrays should be byteswapped */
  if ((byteorder = bson_scan_string(doc, "byteorder", NULL))) {
    unsigned char x[]="\x01\x02\x03\x04";
    char *host_byteorder = (*((uint32_t *)&x) == 0x04030201) ? "LE" : "BE";
    if (strcmp(byteorder, host_byteorder) != 0) byteswap=1;
  }

  /* Get dimensions */
  if ((type = bson_scan(doc, "dimension_values", (void **)&subdoc, NULL)) < 0)
    goto fail;
  if (type == 0) {
    if ((type = bson_scan(doc, "dimensions", (void **)&subdoc, NULL)) < 0)
      goto fail;
  }
  if (type == 0)
    FAILCODE(dliteKeyError, "missing dimension values");
  if (type != bsonDocument)
    FAILCODE1(dliteKeyError, "expected dimension values to be a bson "
              "document, got %s", bson_typename(type));
  if ((ndims = bson_nelements(subdoc)) < 0) goto fail;
  if (!(shape = calloc(ndims, sizeof(size_t))))
    FAILCODE(dliteMemoryError, "allocation failure");
  endptr = NULL;
  i = 0;
  while ((type = bson_parse(subdoc, &ename, &data, NULL, &endptr))) {
    if (type != bsonInt32)
      FAILCODE1(dliteTypeError, "expected dimension values to be bsonInt32, "
                "got %s", bson_typename(type));
    shape[i++] = *((int32_t *)data);
  }
  if (i != ndims)
    FAILCODE2(dliteInconsistentDataError, "expected %d dimensions, got %d",
              ndims, i);

  /* Create instance */
  if (!(id = (uri) ? uri : (uuid) ? uuid : NULL))
    FAILCODE(dliteKeyError, "bson data is missing uri and/or uuid");
  if (!(inst = dlite_instance_create_from_id(metaid, shape, id))) goto fail;

  if (dlite_instance_is_meta(inst)) {
    /* Metadata */
    if ((type = bson_scan(doc, "dimensions", (void **)&subdoc, NULL)) < 0 ||
        set_meta_dimensions((DLiteMeta *)inst, subdoc)) goto fail;

    if ((type = bson_scan(doc, "properties", (void **)&subdoc, NULL)) < 0 ||
        set_meta_properties((DLiteMeta *)inst, subdoc)) goto fail;
  } else {
    /* Data */
    if ((type = bson_scan(doc, "properties", (void **)&subdoc, NULL)) < 0)
      goto fail;
    if (type != bsonDocument)
      FAILCODE1(dliteTypeError, "expected properties to be a bson document, "
                "got %s", bson_typename(type));
    endptr = NULL;
    while ((type = bson_parse(subdoc, &ename, &data, &datasize, &endptr))) {
      if ((idx = dlite_meta_get_property_index(inst->meta, ename)) < 0)
        goto fail;
      if (type == bsonNull) continue;
      DLiteProperty *p = DLITE_PROP_DESCR(inst, idx);
      int btype = bsontype(p->type, p->size);
      if (p->ndims) {
        if (set_array_property(inst, idx, data, byteswap)) goto fail;
      } else {
        if (type != btype)
          FAILCODE3(dliteInconsistentDataError, "expected bson type '%s', "
                    "got '%s' for property: %s", bson_typename(btype),
                    bson_typename(type), ename);
        if (set_scalar_property(inst, idx, data)) goto fail;
      }
    }
  }

  if (shape) free(shape);
  return inst;
 fail:
  if (inst) dlite_instance_decref(inst);
  if (shape) free(shape);
  return NULL;
}
