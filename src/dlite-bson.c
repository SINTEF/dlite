#include "config.h"

#include "string.h"

#include "utils/floats.h"

#include "dlite-entity.h"
#include "dlite-macros.h"
#include "dlite-type.h"


typedef enum {
  bsonDouble     = '\x01',  /* 64-bit float */
  bsonString     = '\x02',  /* UTF-8 string */
  bsonDocument   = '\x03',  /* Embedded BSON document */
  bsonArray      = '\x04',  /* Array (as an embedded BSON document */
  bsonBinary     = '\x05',  /* Binary data: size, subtype, data */
  bsonBool       = '\x08',  /* 1 byte bool */
  bsonNull       = '\x0a',  /* Null value (no additional data) */
  bsonInt32      = '\x10',  /* 32-bit integer */
  bsonInt64      = '\x12',  /* 64-bit integer */
  bsonDecimal128 = '\x13',  /* 128-bit float */
} BsonType;


/*
  Appends an element to a BSON document.

  Arguments
    - buf: Pointer to corrent position in BSON to data to write to.
           May be NULL, in which case only the number of bytes that would
           have been written is returned.
    - bufsize: Size of buffer.  No more than `bufsize` bytes are written.
    - type: BSON type of data to append.
    - ename: Element name (NUL-terminated string).
    - size: Size of data to append (in bytes).
    - data: Pointer to data to append.

  Returns
    - Number of bytes written (or would have been written) to `buf`.
      -1 is returned on error.
 */
static int bson_append(unsigned char *buf, int bufsize, BsonType type,
                       const char *ename, int size, const void *data)
{
  int n=0, len=strlen(ename), expected_size;

  switch (type) {
  case bsonInt32:      expected_size=4; break;
  case bsonInt64:      expected_size=8; break;
  case bsonDouble:     expected_size=8; break;
  case bsonDecimal128: expected_size=16; break;
  default: break;
  }
  if (size != expected_size)
    FAIL3("expected bson type %c to be %d bytes, got %d",
          type, expected_size, size);

  if (buf && bufsize >= len+2) {
    buf[n] = type;
    memcpy(buf+n+1, ename, len+1);
  }
  n += len+2;

  switch (type) {
  case bsonInt32:
  case bsonInt64:
  case bsonDouble:
  case bsonDecimal128:
  case bsonDocument:
  case bsonArray:
    if (size) memcpy(buf+n, data, size);
    n += size;
    break;

  case bsonString:
    len = (size) ? size : (int)strlen((const char *)data);
    if (buf && bufsize-n > 4+len+1) {
      *((int32_t *)buf+n) = len+1;
      memcpy(buf+n+4, data, len);
      buf[n+4+len] = '\0';
    }
    n += 4+len+1;
    break;

  case bsonBinary:
    if (buf && bufsize-n > 4+1+size) {
      *((int32_t *)buf+n) = size;
      buf[n+4] = '\0';
      memcpy(buf+n+4+1, data, size);
    }
    n += 4+1+size;
    break;

  case bsonBool:
    if (buf && bufsize-n > 1) {
      bool b = *((const bool *)data);
      *((unsigned char *)buf+n) = (b) ? '\x01' : '\x00';
    }
    n += 1;
    break;

  case bsonNull:
    break;
  }

  return n;
 fail:
  return -1;
}


/*
  Write beginning of BSON document to `buf` (if `buf` is not NULL).

  `size` is the number of bytes of all elements in the document (not
  including the begin and end bytes of document).

  Returns number of bytes (that would have been) written to `buf`.
*/
static int bson_begin_document(unsigned char *buf, int bufsize, int size)
{
  if (bufsize > 4) *((int32_t *)buf) = size + 5;
  return 4;
}

/*
  Write end of BSON document to `buf` (if `buf` is not NULL).

  Returns number of bytes (that would have been) written to `buf`.
*/
static int bson_end_document(unsigned char *buf, int bufsize)
{
  if (bufsize > 1) *buf = '\x00';
  return 1;
}




/* Some macros simplifying writing to `buf`. */
#define BEGIN(buf, bufsize)                             \
  n += bson_begin_document(buf+n, bufsize-n, 0);        \
  _begin = n

#define END(buf, bufsize)                       \
  *((int32_t *)buf + _begin) = n - _begin;      \
  n += bson_end_document(buf+n, bufsize-n)

#define APPEND(buf, bufsize, type, ename, size, data)           \
  _m = bson_append(buf+n, bufsize-n, type, ename, size, data);  \
  if (_m < 0) goto fail;                                        \
  n += _m

#define APPEND_DOC(buf, bufsize, add_func, obj) \
  _m = add_func(buf+n, bufsize-n, obj);         \
  if (_m < 0) goto fail;                        \
  n += _m


/*
  Write dimension to `buf` and return the number of bytes written.
*/
static int _add_dimension(unsigned char *buf, int bufsize,
                          const DLiteDimension *d)
{
  int _m, _begin, n=0;

  BEGIN(buf, bufsize);
  APPEND(buf, bufsize, bsonString, "name", 0, d->name);
  if (d->description) {
    APPEND(buf, bufsize, bsonString, "description", 0, d->description);
  }
  END(buf, bufsize);
  return n;
 fail:
  return -1;
}

/*
  Write property to `buf` and return the number of bytes written.
*/
static int _add_property(unsigned char *buf, int bufsize,
                         const DLiteProperty *p)
{
  int _m, _begin, i, n=0;
  char index[16];
  int32_t type = p->type;
  int32_t ndim = p->ndims;

  BEGIN(buf, bufsize);
  APPEND(buf, bufsize, bsonString, "name", 0, p->name);
  APPEND(buf, bufsize, bsonInt32, "type", sizeof(type), &type);
  if (p->ref) {
    APPEND(buf, bufsize, bsonString, "ref", 0, p->ref);
  }
  APPEND(buf, bufsize, bsonInt32, "ndim", sizeof(ndim), &ndim);
  if (p->ndims) {
    int save_begin = _begin;
    APPEND(buf, bufsize, bsonArray, "shape", 0, NULL);
    BEGIN(buf, bufsize);
    for (i=0; i<ndim; i++) {
      snprintf(index, sizeof(index), "%d", i);
      APPEND(buf, bufsize, bsonString, index, 0, p->dims[i]);
    }
    END(buf, bufsize);
    _begin = save_begin;
  }
  if (p->unit) {
    APPEND(buf, bufsize, bsonString, "unit", 0, p->unit);
  }
  if (p->description) {
    APPEND(buf, bufsize, bsonString, "description", 0, p->description);
  }
  END(buf, bufsize);
  return n;
 fail:
  return -1;
}


/*
  Write relation to `buf` and return the number of bytes written.
*/
static int _add_relation(unsigned char *buf, int bufsize,
                          const DLiteRelation *r)
{
  int _m, _begin, n=0;

  BEGIN(buf, bufsize);
  APPEND(buf, bufsize, bsonString, "s", 0, r->s);
  APPEND(buf, bufsize, bsonString, "p", 0, r->p);
  APPEND(buf, bufsize, bsonString, "o", 0, r->o);
  END(buf, bufsize);
  return n;
 fail:
  return -1;
}


/*
  Append property as a BSON element.

  Returns number of bytes written to (or would have been written to)
  `buf` or -1 on error.
*/
int dlite_bson_append_property(unsigned char *buf, int bufsize,
                               const DLiteProperty *p, const void *ptr,
                               const size_t *propdims)
{
  int n=0;
  char index[16];
  int i, _m, _begin;

  if (p->ndims && propdims) {  /* array */
    int nmemb=1;
    for (i=0; i < p->ndims; i++) nmemb *= propdims[i];

    switch (p->type) {
    case dliteBlob:
    case dliteBool:
    case dliteInt:
    case dliteUInt:
    case dliteFloat:
    case dliteFixString:
      APPEND(buf, bufsize, bsonBinary, p->name, nmemb*p->size, ptr);
      break;

    case dliteStringPtr:
      APPEND(buf, bufsize, bsonArray, p->name, 0, ptr);
      BEGIN(buf, bufsize);
      const char **q = (const char **)ptr;
      for (i=0; i<nmemb; i++, q++) {
        snprintf(index, sizeof(index), "%d", i);
        APPEND(buf, bufsize, bsonString, index, 0, *q);
      }
      END(buf, bufsize);
      break;

    case dliteRef:
      APPEND(buf, bufsize, bsonArray, p->name, 0, ptr);
      BEGIN(buf, bufsize);
      const DLiteInstance **instances = (const DLiteInstance **)ptr;
      for (i=0; i<nmemb; i++, instances++) {
        snprintf(index, sizeof(index), "%d", i);
        if (*instances) {
          APPEND(buf, bufsize, bsonString, index, 0, (*instances)->uuid);
        } else {
          APPEND(buf, bufsize, bsonNull, index, 0, NULL);
        }
      }
      END(buf, bufsize);
      break;

    case dliteDimension:
      /* We assume that dimensions have unique names */
      APPEND(buf, bufsize, bsonDocument, p->name, 0, ptr);
      BEGIN(buf, bufsize);
      for (i=0; i<nmemb; i++) {
        DLiteDimension *dim = (DLiteDimension *)ptr + i;
        APPEND(buf, bufsize, bsonDocument, dim->name, 0, ptr);
        APPEND_DOC(buf, bufsize, _add_dimension, dim);
      }
      END(buf, bufsize);
      break;

    case dliteProperty:
      /* We assume that properties have unique names */
      APPEND(buf, bufsize, bsonDocument, p->name, 0, ptr);
      BEGIN(buf, bufsize);
      for (i=0; i<nmemb; i++) {
        DLiteProperty *prop = (DLiteProperty *)ptr + i;
        APPEND(buf, bufsize, bsonDocument, prop->name, 0, ptr);
        APPEND_DOC(buf, bufsize, _add_property, prop);
      }
      END(buf, bufsize);
      break;

    case dliteRelation:
      APPEND(buf, bufsize, bsonArray, p->name, 0, ptr);
      BEGIN(buf, bufsize);
      for (i=0; i<nmemb; i++) {
        DLiteRelation *rel = (DLiteRelation *)ptr + i;
        snprintf(index, sizeof(index), "%d", i);
        APPEND(buf, bufsize, bsonDocument, index, 0, ptr);
        APPEND_DOC(buf, bufsize, _add_relation, rel);
      }
      END(buf, bufsize);
      break;
    }

  } else { /* scalar */

    switch (p->type) {
    case dliteBlob:
      APPEND(buf, bufsize, bsonBinary, p->name, p->size, ptr);
      break;

    case dliteBool:
      APPEND(buf, bufsize, bsonBool, p->name, p->size, ptr);
      break;

    case dliteInt:
      if (p->size <= 4) {
        int32_t v;
        switch (p->size) {
        case 1: v = *(int8_t *)ptr; break;
        case 2: v = *(int16_t *)ptr; break;
        case 4: v = *(int32_t *)ptr; break;
        default: FAIL1("invalid int size: %d", (int)p->size);
        }
        APPEND(buf, bufsize, bsonInt32, p->name, p->size, &v);
      } else if (p->size == 8) {
        int64_t v = *(int64_t *)ptr; break;
        APPEND(buf, bufsize, bsonInt64, p->name, p->size, &v);
      } else {
        FAIL1("unknown int size: %d", (int)p->size);
      }
      break;

    case dliteUInt:
      if (p->size <= 4) {
        int32_t v;
        switch (p->size) {
        case 1: v = *(uint8_t *)ptr; break;
        case 2: v = *(uint16_t *)ptr; break;
        case 4: v = *(uint32_t *)ptr; break;  // TODO: check for overflow
        default: FAIL1("invalid uint size: %d", (int)p->size);
        }
        APPEND(buf, bufsize, bsonInt32, p->name, p->size, &v);
      } else if (p->size == 8) {
        int64_t v = *(uint64_t *)ptr; break;  // TODO: check for overflow
        APPEND(buf, bufsize, bsonInt64, p->name, p->size, &v);
      } else {
        FAIL1("unknown uint size: %d", (int)p->size);
      }
      break;

    case dliteFloat:
      if (p->size <= 8) {
        float64_t v;
        switch (p->size) {
        case 4: v = *(float32_t *)ptr; break;
        case 8: v = *(float64_t *)ptr; break;
        default: FAIL1("invalid float size: %d", (int)p->size);
        }
        APPEND(buf, bufsize, bsonDouble, p->name, p->size, &v);
#ifdef HAVE_FLOAT128
      } else if (p->size <= 16) {
        float128_t v = *(uint64_t *)ptr; break;
        switch (p->size) {
#ifdef HAVE_FLOAT80
        case 10: v = *(float80_t *)ptr; break;
#endif
#ifdef HAVE_FLOAT96
        case 12: v = *(float96_t *)ptr; break;
#endif
        case 16: v = *(float128_t *)ptr; break;
        default: FAIL1("invalid float size: %d", (int)p->size);
        }
        APPEND(buf, bufsize, bsonDecimal128, p->name, p->size, &v);
#endif  /* HAVE_FLOAT128 */
      } else {
        FAIL1("unknown float size: %d", (int)p->size);
      }
      break;

    case dliteFixString:
      APPEND(buf, bufsize, bsonString, p->name, p->size, (const char *)ptr);
      break;

    case dliteStringPtr:
      APPEND(buf, bufsize, bsonString, p->name, p->size, *(const char **)ptr);
      break;

    case dliteRef:
      if (ptr) {
        APPEND(buf, bufsize-n, bsonString, p->name, p->size,
               ((DLiteInstance *)ptr)->uuid);
      } else {
        APPEND(buf, bufsize-n, bsonNull, p->name, 0, NULL);
      }
      break;

    case dliteDimension:
      APPEND(buf, bufsize, bsonDocument, p->name, 0, ptr);
      APPEND_DOC(buf, bufsize, _add_dimension, ptr);
      break;

    case dliteProperty:
      APPEND(buf, bufsize, bsonDocument, p->name, 0, ptr);
      APPEND_DOC(buf, bufsize, _add_property, ptr);
      break;

    case dliteRelation:
      APPEND(buf, bufsize, bsonDocument, p->name, 0, ptr);
      APPEND_DOC(buf, bufsize, _add_relation, ptr);
      break;
    }

  }
  return n;
 fail:
  return -1;
}


int dlite_bson_append_instance(unsigned char *buf, int bufsize,
                               const DLiteInstance *inst)
{
  int n=0;
  int i, _m, _begin, save_begin;

  BEGIN(buf, bufsize);
  APPEND(buf, bufsize, bsonString, "uuid", DLITE_UUID_LENGTH, inst->uuid);
  APPEND(buf, bufsize, bsonString, "meta", 0, inst->meta->uri);
  // TODO: add parent

  if (DLITE_NDIM(inst)) {
    APPEND(buf, bufsize, bsonDocument, "dimensions", 0, NULL);
    save_begin = _begin;
    BEGIN(buf, bufsize);
    for (i=0; i < (int)DLITE_NDIM(inst); i++) {
      int32_t size = DLITE_DIM(inst, i);
      APPEND(buf, bufsize, bsonInt32, DLITE_DIM_DESCR(inst, i)->name,
             sizeof(size), &size);
    }
    END(buf, bufsize);
    _begin = save_begin;
  }

  if (DLITE_NPROP(inst)) {
    APPEND(buf, bufsize, bsonDocument, "properties", 0, NULL);
    save_begin = _begin;
    BEGIN(buf, bufsize);
    for (i=0; i < (int)DLITE_NPROP(inst); i++) {
      void *ptr;
      if (!(ptr = dlite_instance_get_property_by_index(inst, i))) goto fail;
      if ((_m = dlite_bson_append_property(buf, bufsize, DLITE_PROP(inst, i),
                                           ptr, DLITE_PROP_DIMS(inst, n))) < 0)
        goto fail;
      n += _m;
    }
    END(buf, bufsize);
    _begin = save_begin;
  }

  if (DLITE_NRELS(inst)) {
    APPEND(buf, bufsize, bsonArray, "relations", 0, NULL);
    for (i=0; i < (int)DLITE_NPROP(inst); i++) {
      char index[12];
      DLiteRelation *rel = DLITE_REL(inst, i);
      snprintf(index, sizeof(index), "%d", i);
      APPEND(buf, bufsize, bsonDocument, index, 0, NULL);
      APPEND_DOC(buf, bufsize, _add_relation, rel);
    }
  }

  END(buf, bufsize);

  return n;
 fail:
  return -1;
}
