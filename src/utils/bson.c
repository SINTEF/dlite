#include "config.h"

#include <string.h>

#include "err.h"
#include "byteorder.h"
#include "floats.h"
#include "boolean.h"
#include "bson.h"


/* Return name of given BSON type or NULL on error. */
const char *bson_typename(BsonType type)
{
  switch (type) {
  case bsonDouble:     return "double";
  case bsonString:     return "string";
  case bsonDocument:   return "document";
  case bsonArray:      return "array";
  case bsonBinary:     return "binary";
  case bsonBool:       return "bool";
  case bsonNull:       return "null";
  case bsonInt32:      return "int32";
#ifdef HAVE_INT64
  case bsonUInt64:     return "uint64";
  case bsonInt64:      return "int64";
#endif
#ifdef HAVE_FLOAT128
  case bsonDecimal128: return "decimal128";
#endif
  }
  return errx(bsonValueError, "invalid bson type number: %d", type), NULL;
}


/*
  Return the data size for given type or -1 if `type` require an
  explicit data type.
 */
int bson_datasize(BsonType type)
{
  switch (type) {
  case bsonDouble:     return 8;
  case bsonString:     return -1;
  case bsonDocument:   return -1;
  case bsonArray:      return -1;
  case bsonBinary:     return -1;
  case bsonBool:       return 1;
  case bsonNull:       return 0;
  case bsonInt32:      return 4;
#ifdef HAVE_INT64
  case bsonUInt64:     return 8;
  case bsonInt64:      return 8;
#endif
#ifdef HAVE_FLOAT128
  case bsonDecimal128: return 16;
#endif
  }
  abort();  /* should never be reached */
}


/*
  Return the size of element with given `type` and `ename` and whos data is
  of size `size`.  For fix-sized types (null, bool, ints, floats), size may
  be negative to indicate to use the default.

  If `ename` is NULL, it is assumed it is a 4 byte array index.

  Returns a negative error code on error.
 */
int bson_elementsize(BsonType type, const char *ename, int size)
{
  int expected_size = bson_datasize(type);
  int esize = (ename) ? strlen(ename) + 1 : 4;
  if (size < 0) size = expected_size;
  if (size < 0)
    return errx(bsonValueError,
                "positive `size` must be provided for bson type '%s'",
                bson_typename(type));
  if (expected_size >= 0 && size != expected_size)
    return errx(bsonValueError,
                "expected bson type %c to be %d bytes, got %d",
                type, expected_size, size);
  switch (type) {
  case bsonString:
  case bsonBinary:
    return 1 + esize + 4 + size + 1;
  default:
    return 1 + esize + size;
  }
}


/*
  Return size of BSON document or a negative error code on error.
 */
int bson_docsize(const unsigned char *doc)
{
  if (!doc) return 0;
  int docsize = (int)le32toh(*((const int32_t *)doc));
  if (docsize < 5)
    return errx(bsonInconsistentDataError,
                "bson document must at least be 5 bytes, got `docsize=%d`",
                docsize);
  return docsize;
}


/*
  Return the number of (non-nested) elements in BSON document.
  A negative error code is returned on error.
 */
int bson_nelements(const unsigned char *doc)
{
  int type, docsize=bson_docsize(doc), n=4, nelements=0;
  while ((type = doc[n++])) {
    n += strlen((char *)doc+n) + 1;
    switch (type) {
    case bsonDouble:     n += 8;  break;
    case bsonBool:       n++;     break;
    case bsonNull:                break;
    case bsonInt32:      n += 4;  break;
#ifdef HAVE_INT64
    case bsonUInt64:     n += 8;  break;
    case bsonInt64:      n += 8;  break;
#endif
#ifdef HAVE_FLOAT128
    case bsonDecimal128: n += 16; break;
#endif
    case bsonDocument:
    case bsonArray:
      n += le32toh(*((int32_t *)(doc+n)));
      break;
    case bsonString:
      n += le32toh(*((int32_t *)(doc+n))) + 4;
      break;
    case bsonBinary:
      n += le32toh(*((int32_t *)(doc+n))) + 5;
      break;
    }
    nelements++;
    if (n > docsize)
      return errx(bsonInconsistentDataError, "inconsistent bson document");
  }
  return nelements;
}


/*
  Initialize buffer `buf` (of size `bufsize`)  to a BSON document.

  Returns number of bytes consumed or a negative error code on error.
 */
int bson_init_document(unsigned char *buf, int bufsize)
{
  if (bufsize < 5) return 5;
  *((int32_t *)buf) = htole32(5);
  buf[4] = '\x00';
  return 5;
}


/*
  Appends an element to a BSON document.

  Arguments:
    - doc: Pointer to a BSON document to append data to.  The memory pointed
        to must have been initialised with bson_init_document().
    - bufsize: Remaining size of buffer to write to.  No more than
        `bufsize` bytes will be written.
    - type: BSON type of data to append.
    - ename: Element name (NUL-terminated string).  Name of data to append.
    - size: Size of data to append (in bytes).
    - data: Pointer to data to append.  If this is a BSON document, then
        the no more data should be appended to it.

  Returns:
    - Number of bytes appended (or would have been appended) to `doc`.
      A negative error code is returned on error.
 */
int bson_append(unsigned char *doc, int bufsize, BsonType type,
                const char *ename, int size, const void *data)
{
  int docsize, esize, n;
  if (size < 0)
    size = (type == bsonString) ? (int)strlen(data) : bson_datasize(type);
  if ((esize = bson_elementsize(type, ename, size)) < 0) return esize;
  assert(size >= 0);

  /* If buffer is too small, just return number of bytes we would have
     appended if it would have been large enough. */

  if (bufsize < esize) return esize;

  /* Consistency check */
  if ((docsize = bson_docsize(doc)) < 0) return docsize;
  if (doc[docsize-1])
    return errx(bsonInconsistentDataError,
                "bson document should always end with a NUL byte, got %c",
                doc[docsize-1]);

  /* We keep the document consistent while writing ename and data. */
  n = docsize;  /* Current position */

  /* Append ename */
  int elen = strlen(ename);
  memcpy(doc+n, ename, elen);
  n += elen;
  doc[n++] = '\x00';

  /* Append data */
  switch (type) {
  case bsonInt32:
    *((int32_t *)(doc+n)) = htole32(*((int32_t *)data));
    n += size;
    break;
#ifdef HAVE_INT64
  case bsonUInt64:
    *((uint64_t *)(doc+n)) = htole64(*((uint64_t *)data));
    n += size;
    break;
  case bsonInt64:
    *((int64_t *)(doc+n)) = htole64(*((int64_t *)data));
    n += size;
    break;
#endif
  case bsonDouble:
    {
      uint64_t v = htole64(*((uint64_t *)data));
      *((float64_t *)(doc+n)) = *((float64_t *)&v);
      n += size;
    }
    break;
#if defined(HAVE_FLOAT128) && defined(HAVE_BSWAP128)
  case bsonDecimal128:
    {
      uint128_t v = htole128(*((uint128_t *)data));
      *((float128_t *)(doc+n)) = *((float128_t *)&v);
      n += size;
    }
    break;
#endif
  case bsonDocument:
  case bsonArray:
    if (size) memcpy(doc+n, data, size);
    n += size;
    break;
  case bsonString:
    *((int32_t *)(doc+n)) = htole32(size+1);
    n += 4;
    memcpy(doc+n, data, size);
    n += size;
    doc[n++] = '\x00';
    break;
  case bsonBinary:
    *((int32_t *)(doc+n)) = htole32(size);
    n += 4;
    doc[n++] = '\x00';
    memcpy(doc+n, data, size);
    n += size;
    break;
  case bsonBool:
    doc[n++] = (*((const bool *)data)) ? '\x01' : '\x00';
    break;
  case bsonNull:
    break;
  }

  /* Terminate document */
  doc[n++] = '\x00';

  /* Update document size and element type */
  assert(n == docsize + esize);
  *((int32_t *)doc) = htole32(n);
  doc[docsize-1] = type;

  return esize;
}


/*
  Begin appending a sub-document or array to BSON document.

  This is an alternative to `bson_append()` for appending sub-documents
  that does not require that you create the sub-document before appending
  it.

  A call to `bson_begin_subdoc()` must be followed by a matching call
  to `bson_end_subdoc()`.  The parent document is not changed before
  `bson_end_subdoc()` is called.

  Arguments:
    - doc: Pointer to a BSON document to append data to.  The memory pointed
        to must have been initialised with bson_init_document().
    - bufsize: Remaining size of buffer to write to.  No more than
        `bufsize` bytes will be written.
    - ename: Element name (NUL-terminated string).  Name of data to append.
    - subdoc: If not NULL, the memory pointed to will be set to the start of
        the sub-document, such that it can be used by subsequent calls to
        bson_append().

  Returns:
    Number of bytes appended (or would have been appended) to `doc`.
    A negative error code is returned on error.

  Example:

    unsigned char doc[1024], *subdoc;
    int bufsize=sizeof(doc), n;

    n = bson_init_document(doc, bufsize);
    n += bson_begin_subdoc(doc, bufsize-n, "subdoc", &subdoc);
    n += bson_append(subdoc, bufsize-n, bsonString, "hello", -1, "world");
    ...
    n += bson_end_subdoc(doc, bufsize-n, bsonDocument);
 */
int bson_begin_subdoc(unsigned char *doc, int bufsize, const char *ename,
                      unsigned char **subdoc)
{
  int docsize, elen=strlen(ename), esize=elen+6;
  if (bufsize < esize) return esize;
  if ((docsize = bson_docsize(doc)) < 0) return docsize;
  if (doc[docsize-1])
    return errx(bsonInconsistentDataError,
                "expect BSON document to end with NUL");

  /* If buffer is too small, just return number of bytes we would have
     appended if it would have been large enough. */
  if (bufsize < esize) return esize;

  int n = docsize;  // current position
  memcpy(doc+n, ename, elen);
  n += elen;
  doc[n++] = '\x00';
  if (subdoc) *subdoc = doc+n;
  *((int32_t *)(doc+n)) = htole32(5);
  n += 4;
  doc[n++] = '\x00';
  assert(n == docsize + esize);
  return esize;
}


/*
  End sub-document started with `bson_begin_subdoc()`.

  Arguments:
    - doc: Pointer to a BSON document to append data to.  The memory pointed
        to must have been initialised with bson_init_document().
    - bufsize: Remaining size of buffer to write to.  No more than
        `bufsize` bytes will be written.
    - type: BSON type of sub-document; `bsonDocument` or `bsonArray`.

  Returns:
    - Number of bytes appended (or would have been appended) to `doc`.
      A negative error code is returned on error.
 */
int bson_end_subdoc(unsigned char *doc, int bufsize, BsonType type)
{
  if (type != bsonDocument && type != bsonArray)
    return errx(bsonValueError, "sub-document type must be bsonDocument "
                "or bsonArray: %d", type);
  if (bufsize < 1) return 1;
  int docsize = bson_docsize(doc);
  if (docsize < 0) return docsize;

  int elen = strlen((char *)doc + docsize);
  int subsize = le32toh(*((int32_t *)(doc+docsize+elen+1)));
  int newsize = docsize + elen + 1 + subsize + 1;
  doc[newsize-1] = '\x00';
  doc[docsize-1] = type;
  *((int32_t *)(doc)) = htole32(newsize);
  return 1;
}

/*
  Functions for partially building up a binary element.
*/

/*
  Start partially built binary element.

  Arguments:
    - doc: Pointer to a BSON document to append data to.  The memory pointed
        to must have been initialised with bson_init_document().
    - bufsize: Remaining size of buffer to write to.  No more than
        `bufsize` bytes will be written.
    - ename: Element name (NUL-terminated string).  Name of data to append.
    - subdoc: If not NULL, the memory pointed to will be set to the start of
        the sub-document, such that it can be used by subsequent calls to
        bson_append().
    - size: Size of data to append (in bytes).
    - data: Pointer to data to append.  If this is a BSON document, then
        the no more data should be appended to it.

  Returns:
    - Number of bytes appended (or would have been appended) to `doc`.
      A negative error code is returned on error.
*/
int bson_begin_binary(unsigned char *doc, int bufsize, const char *ename,
                      unsigned char **subdoc)
{
  int docsize=bson_docsize(doc), elen=strlen(ename), esize=elen+6;
  if ((docsize) < 0) return docsize;
  if (bufsize < esize) return esize;
  if (doc[docsize-1]) return errx(bsonInconsistentDataError,
                                  "expect BSON document to end with NUL");

  int n = docsize;  // current position
  memcpy(doc+n, ename, elen);
  n += elen;
  doc[n++] = '\x00';
  if (subdoc) *subdoc = doc+n;
  *((int32_t *)(doc+n)) = htole32(0);
  n += 4;
  doc[n++] = '\x00';  // subtype
  assert(n == docsize + esize);
  return esize;
}

/*
  Append data to partially built binary element.

  Arguments:
    - subdoc: Pointer to a BSON document to append data to.  The memory
        pointed to should have been initialised with bson_start_binary().
    - bufsize: Remaining size of buffer to write to.  No more than
        `bufsize` bytes will be written.
    - size: Size of data to append (in bytes).
    - data: Pointer to data to append.  If this is a BSON document, then
        the no more data should be appended to it.

  Returns:
    - Number of bytes appended (or would have been appended) to `doc`.
      A negative error code is returned on error.
*/
int bson_append_binary(unsigned char *subdoc, int bufsize,
                       int size, void *data)
{
  if (bufsize < size) return size;
  int oldsize = le32toh(*((int32_t *)subdoc));
  int newsize = oldsize + size;
  *((int32_t *)(subdoc)) = htole32(newsize);
  memcpy(subdoc+5+oldsize, data, size);
  return size;
}

/*
  Finalise partially built binary element.

  Arguments:
    - doc: Pointer to a BSON document to append data to.  The memory pointed
        to must have been initialised with bson_init_document().
    - bufsize: Remaining size of buffer to write to.  No more than
        `bufsize` bytes will be written.

  Returns:
    - Number of bytes appended (or would have been appended) to `doc`.
      A negative error code is returned on error.
*/
int bson_end_binary(unsigned char *doc, int bufsize)
{
  if (bufsize < 1) return 1;
  int docsize = bson_docsize(doc);
  if (docsize < 0) return docsize;
  size_t elen = strlen((char *)doc + docsize);
  int esize = le32toh(*((int32_t *)(doc + docsize + elen + 1)));
  int newsize = docsize + elen + 6 + esize + 1;
  doc[docsize-1] = bsonBinary;
  doc[newsize-1] = '\x00';
  *((int32_t *)(doc)) = htole32(newsize);
  return 1;
}


/*
  Parses next element in a BSON document.

  Arguments:
    - doc: BSON document to parse.
    - ename: The memory pointed to will be will be assigned to the name
        of the element that is parsed.
    - data: The memory pointed to will be will be assigned to point to the
        data of the element that is parsed.
    - datasize: The memory pointed to will be will be assigned to the size
        of the data that is parsed.
    - endptr: The pointer pointed to by `endptr` will be updated to point
        to the next element.  At the initial call, the value pointed to
        should be initialised to NULL.

  Returns:
    The BSON type of the parsed element.  If there are no more elements
    left in the document, zero is returned.
    On error is a negative error code returned.

  Note:
    For type bsonInt32, bsonUInt64, bsonInt64, bsonDouble and bsonDecimal128,
    the memory that `*data` is assigned to is little endian.  Use `le32toh()`
    family of functions to convert to host byte order or one of the convinient
    `bson_scan_<type>()` functions.
 */
int bson_parse(const unsigned char *doc, char **ename, void **data,
               int *datasize, unsigned char **endptr)
{
  const unsigned char *p = (endptr && *endptr) ? *endptr : doc + 4;
  BsonType type = *(p++);
  char *_ename;
  void *_data;
  int _datasize;
  if (!type) return 0;

  _ename = (char *)p;
  p += strlen(_ename) + 1;

  switch (type) {
  case bsonDouble:
  case bsonBool:
  case bsonNull:
  case bsonInt32:
#ifdef HAVE_INT64
  case bsonUInt64:
  case bsonInt64:
#endif
#ifdef HAVE_FLOAT128
  case bsonDecimal128:
#endif
    _datasize = bson_datasize(type);
    assert(_datasize >= 0);
    _data = (void *)p;
    p += _datasize;
    break;
  case bsonString:
    _datasize = le32toh(*((int32_t *)p)) - 1;
    p += 4;
    _data = (void *)p;
    p += _datasize + 1;
    break;
  case bsonDocument:
  case bsonArray:
    _datasize = le32toh(*((int32_t *)p));
    _data = (void *)p;
    p += _datasize;
    break;
  case bsonBinary:
    _datasize = le32toh(*((int32_t *)p));
    p += 4;
    if (*p != '\x00')
      return errx(bsonParseError,
                  "unsupported binary bson subtype: %02x", *p);
    p++;
    _data = (void *)p;
    p += _datasize;
    break;
  }

  if (endptr) *endptr = (unsigned char *)p;
  if (ename) *ename = _ename;
  if (data) *data = _data;
  if (datasize) *datasize = _datasize;
  return type;
}


/*
  Scan a BSON document for an element of the given type and/or element name.

  Arguments:
    - doc: BSON document to parse.
    - ename: Element name to scan for.
    - data: The memory pointed to will be will be assigned to point to the
        data of the element that is scanned for.
    - datasize: The memory pointed to will be will be assigned to the size
        of the data that is scanned for.

  Returns:
    The BSON type of the element scanned for.  If there are no more elements
    left in the document, zero is returned.
    On error is a negative error code returned.

  Note:
    For type bsonInt32, bsonUInt64, bsonInt64, bsonDouble and bsonDecimal128,
    the memory that `*data` is assigned to is little endian.  Use `le32toh()`
    family of functions to convert to host byte order or one of the convinient
    `bson_scan_<type>()` functions.
 */
int bson_scan(const unsigned char *doc, const char *ename,
              void **data, int *datasize)
{
  return bson_scann(doc, ename, strlen(ename), data, datasize);
}


/*
  Like `bson_scan()` but takes the length of `ename` as an additional
  argument.
 */
int bson_scann(const unsigned char *doc, const char *ename, size_t len,
               void **data, int *datasize)
{
  void *_data;
  int type, _datasize;
  char *_ename;
  unsigned char *endptr = NULL;
  while ((type = bson_parse(doc, &_ename, &_data, &_datasize, &endptr)) > 0) {
    if (len == strlen(_ename) && strncmp(ename, _ename, len) == 0) {
      if (data) *data = _data;
      if (datasize) *datasize = _datasize;
      break;
    }
  }
  return type;
}


/*
  Family of convenient functions that scans a BSON document for a
  numerical value and return it (in host byte order).

  Arguments:
    - doc: BSON document to parse.
    - ename: Element name to scan for.
    - errcode: If not NULL, it will be set to zero on sucess or a
        negative error code on failure.
        If `errcode` is non-zero on input, errx() is called with
        bsonKeyError if `ename` cannot be found.

  Returns:
    The value scanned for or zero on error.
 */

#define SCAN(doc, ename, type, errcode, datatype, bits)         \
  datatype *data;                                               \
  uint ## bits ## _t v;                                         \
  int _type = bson_scan(doc, ename, (void **)&data, NULL);      \
  if (_type < 0) {                                              \
    if (errcode) *errcode = _type;                              \
    return 0;                                                   \
  } else if (_type == 0) {                                      \
    if (errcode) {                                              \
      if (*errcode)                                             \
        errx(bsonKeyError, "no such element: '%s'", ename);     \
      *errcode = bsonKeyError;                                  \
    }                                                           \
    return 0;                                                   \
  } else if (_type != type) {                                   \
    errx(bsonTypeError,                                         \
         "expected type of element '%s' to be %s, got %s",      \
         ename, bson_typename(type), bson_typename(_type));     \
    if (errcode) *errcode = bsonTypeError;                      \
    return 0;                                                   \
  }                                                             \
  if (errcode) *errcode = 0;                                    \
  v = le ## bits ## toh(*((uint ## bits ## _t *)data));         \
  return *((datatype *)&v)


int32_t bson_scan_int32(const unsigned char *doc, const char *ename,
                        BsonError *errcode)
{
  SCAN(doc, ename, bsonInt32, errcode, int32_t, 32);
}

double bson_scan_double(const unsigned char *doc, const char *ename,
                        BsonError *errcode)
{
#if SIZEOF_DOUBLE == 4
  SCAN(doc, ename, bsonDouble, errcode, double, 32);
#elif SIZEOF_DOUBLE == 8
  SCAN(doc, ename, bsonDouble, errcode, double, 64);
#elif SIZEOF_DOUBLE == 16
  SCAN(doc, ename, bsonDecimal128, errcode, double, 128);
#else
# error "Non-standard size of double" # SIZEOF_DOUBLE
#endif
}

#ifdef HAVE_INT64
int64_t bson_scan_int64(const unsigned char *doc, const char *ename,
                        BsonError *errcode)
{
  SCAN(doc, ename, bsonInt64, errcode, int64_t, 64);
}

uint64_t bson_scan_uint64(const unsigned char *doc, const char *ename,
                          BsonError *errcode)
{
  SCAN(doc, ename, bsonUInt64, errcode, uint64_t, 64);
}
#endif

#ifdef HAVE_FLOAT128
float128_t bson_scan_decimal128(const unsigned char *doc, const char *ename,
                                BsonError *errcode)
{
  SCAN(doc, ename, bsonDecimal128, errcode, float128_t, 128);
}
#endif

const char *bson_scan_string(const unsigned char *doc, const char *ename,
                             BsonError *errcode)
{
  void *data;
  int type;
  if ((type = bson_scan(doc, ename, &data, NULL)) < 0) {
    if (errcode) *errcode = type;
    return NULL;
  } else if (type == 0) {
    if (errcode) {
      if (*errcode)
        errx(bsonKeyError, "no such element: '%s'", ename);
      *errcode = bsonKeyError;
    }
    return NULL;
  } else if (type != bsonString) {
    errx(bsonTypeError,
         "expected element '%s' to be string, got %s",
         ename, bson_typename(type));
    if (errcode) *errcode = bsonTypeError;
    return NULL;
  }
  if (errcode) *errcode = 0;
  return (const char *)data;
}

bool bson_scan_bool(const unsigned char *doc, const char *ename,
                             BsonError *errcode)
{
  void *data;
  int type;
  if ((type = bson_scan(doc, ename, &data, NULL)) < 0) {
    if (errcode) *errcode = type;
    return 0;
  } else if (type == 0) {
    if (errcode) {
      if (*errcode)
        errx(bsonKeyError, "no such element: '%s'", ename);
      *errcode = bsonKeyError;
    }
    return 0;
  } else if (type != bsonString) {
    errx(bsonTypeError,
         "expected element '%s' to be boolean, got %s",
         ename, bson_typename(type));
    if (errcode) *errcode = bsonTypeError;
    return 0;
  }
  if (errcode) *errcode = 0;
  return *((bool *)data);
}
