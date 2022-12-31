#ifndef _BSON_H
#define _BSON_H

/**
  @file
  @brief Functions for creating and parsing BSON documents.

  Currently, this only implements a subset of BSON.
*/
#include "boolean.h"
#include "integers.h"
#include "floats.h"


/** Supported BSON data types */
typedef enum {
  bsonDouble     = '\x01',  /* 64-bit float */
  bsonString     = '\x02',  /* UTF-8 string */
  bsonDocument   = '\x03',  /* Embedded BSON document */
  bsonArray      = '\x04',  /* Array (as an embedded BSON document */
  bsonBinary     = '\x05',  /* Binary data: size, subtype, data */
  bsonBool       = '\x08',  /* 1 byte bool */
  bsonNull       = '\x0a',  /* Null value (no additional data) */
  bsonInt32      = '\x10',  /* 32-bit integer */
#ifdef HAVE_INT64
  bsonUInt64     = '\x11',  /* 64-bit unsigned integer */
  bsonInt64      = '\x12',  /* 64-bit integer */
#endif
#ifdef HAVE_FLOAT128
  bsonDecimal128 = '\x13',  /* 128-bit float */
#endif
} BsonType;

/** Error codes (matching dlite) */
typedef enum {
  bsonTypeError=-5,              /*!< Inappropriate argument or function type */
  bsonValueError=-9,             /*!< Inappropriate argument value */
  bsonKeyError=-14,              /*!< BSON key (ename) not found */
  bsonParseError=-15,            /*!< Cannot parse input */
  bsonInconsistentDataError=-18  /*!< Inconsistent data */
} BsonError;


/**
 * @name Utility functions
 */
/** @{ */


/**
  Return name of given BSON type or NULL on error.
*/
const char *bson_typename(BsonType type);


/**
  Return size of BSON document.
 */
int bson_docsize(const unsigned char *doc);


/**
  Return the number of (non-nested) elements in BSON document.
  A negative error code is returned on error.
 */
int bson_nelements(const unsigned char *doc);


/** @} */
/**
 * @name Basic functions for creating a BSON document
 */
/** @{ */


/**
  Initialize buffer `buf` (of size `bufsize`)  to a BSON document.

  Returns number of bytes consumed or a negative error code on error.
 */
int bson_init_document(unsigned char *buf, int bufsize);


/**
  Appends an element to a BSON document.

  Arguments:
    - doc: Pointer to a BSON document to append data to.  The memory pointed
        to must have been initialised with bson_init_document().
    - bufsize: Remaining size of buffer to write to.  No more than
        `bufsize` bytes will be written.
    - ename: Element name (NUL-terminated string).  Name of data to append.
    - size: Size of data to append (in bytes).
    - data: Pointer to data to append.  If this is a BSON document, then
        the no more data must be appended to it.

  Returns:
    The number of bytes written (or would have been written) to `doc`.
    On error, a negative error code is returned.
 */
int bson_append(unsigned char *doc, int bufsize, BsonType type,
                const char *ename, int size, const void *data);


/** @} */
/**
 * @name Alternative functions for partially appending a sub-document or array.
 */
/** @{ */


/**
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
    - Number of bytes appended (or would have been appended) to `doc`.
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
                      unsigned char **subdoc);


/**
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
int bson_end_subdoc(unsigned char *doc, int bufsize, BsonType type);



/** @} */
/**
 * @name Alternative functions for partially appending a binary element.
 */
/** @{ */


/**
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
                      unsigned char **subdoc);


/**
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
                       int size, void *data);


/**
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
int bson_end_binary(unsigned char *doc, int bufsize);



/** @} */
/**
 * @name Basic parsing a BSON document
 */
/** @{ */


/**
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
 */
int bson_parse(const unsigned char *doc, char **ename, void **data,
               int *datasize, unsigned char **endptr);


/** @} */
/**
 * @name Convenient functions for scanning a BSON document
 */
/** @{ */


/**
  Scan a BSON document for an element of the given type and/or element name.

  Arguments:
    - doc: BSON document to parse.
    - ename: Element name to scan for.
    - data: The memory pointed to will be will be assigned to point to the
        data of the element that scanned for.
    - datasize: The memory pointed to will be will be assigned to the size
        of the data that is scanned for.

  Returns:
    The BSON type of the element scanned for.  If there are no more elements
    left in the document, zero is returned.
    On error is a negative error code returned.
 */
int bson_scan(const unsigned char *doc, const char *ename,
              void **data, int *datasize);


/**
  Like `bson_scan()` but takes the length of `ename` as an additional
  argument.
 */
int bson_scann(const unsigned char *doc, const char *ename, size_t len,
               void **data, int *datasize);


/**
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
int32_t bson_scan_int32(const unsigned char *doc, const char *ename,
                        BsonError *errcode);
double bson_scan_double(const unsigned char *doc, const char *ename,
                        BsonError *errcode);
#ifdef HAVE_INT64
int64_t bson_scan_int64(const unsigned char *doc, const char *ename,
                        BsonError *errcode);
uint64_t bson_scan_uint64(const unsigned char *doc, const char *ename,
                          BsonError *errcode);
#endif
#ifdef HAVE_FLOAT128
float128_t bson_scan_decimal128(const unsigned char *doc, const char *ename,
                                BsonError *errcode);
#endif
const char *bson_scan_string(const unsigned char *doc, const char *ename,
                             BsonError *errcode);
bool bson_scan_bool(const unsigned char *doc, const char *ename,
                    BsonError *errcode);

/** @} */


#endif  /* _BSON_H */
