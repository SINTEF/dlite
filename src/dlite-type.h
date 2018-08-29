#ifndef _DLITE_TYPES_H
#define _DLITE_TYPES_H

/**
  @file
  @brief Data types for instance properties

  The types of properties is in DLite described by its `dtype` (a type
  number from enum DLiteType) and `size` (size in bytes of a single
  data point).

  Note that the type (`dtype` and `size`) concerns a single data
  point.  The dimensionality (`ndims` and `dims`) of a property comes
  in addition and is not treated by the functions in this file.

  The properties can have most of the basic types found in C, with
  some additions, as summarised in the table below:

  type      | dtype          | sizes          | description                      | examples
  ----      | -----          | -----          | -----------                      | --------
  blob      | dliteBlob      | any            | binary blob, sequence of bytes   | blob32, blob128
  bool      | dliteBool      | sizeof(bool)   | boolean                          | bool
  int       | dliteInt       | 1, 2, 4, {8}   | signed integer                   | (int), int8, int16, int32, {int64}
  uint      | dliteUInt      | 1, 2, 4, {8}   | unsigned integer                 | (uint), uint8, uint16, uint32, {uint64}
  float     | dliteFloat     | 4, 8, {10, 16} | floating point                   | (float), (double), float32, float64, {float80, float128}
  fixstring | dliteFixString | any            | fix-sized NUL-terminated string  | string20
  string    | dliteStringPtr | sizeof(char *) | pointer to NUL-terminated string | string

  The column "examples" shows examples of how these types whould be
  written when specifying the type of a property.

  The types in parenthesis are included for portability with SOFT5,
  but not encouraged because their may vary between platforms.

  The types in curly brackets may not be defined on all platforms.
  The headers "integers.h" and "floats.h" provides macros like
  `HAVE_INT64_T`, `HAVE_FLOAT128_T`... that can be used to check for
  availability.

  Some additional notes:
    - *blob*: is a sequence of bytes of length `size`.  When writing
      the type name, you should always append the size as shown in
      the examples in table.
    - *bool*: corresponds to the bool type as defined in <stdbool.h>.
      To support systems lacking <stdbool.h> you can use "boolean.h"
      provided by dlite.
    - *float*: currently `long double` in C is not supported.  If
      needed, it can easily be added.
    - *fixstring*: corresponds to `char fixstring[size]` in C. The
      size includes the terminating NUL.
    - *string*: corresponds to `char *string` in C, pointing to memory
      allocated with malloc().  If you free a string, you should always
      set the pointer to NULL, since functions like dlite_entity_free()
      otherwise will try to free it again, causing memory corruption.
*/

#include <stdlib.h>

#include "boolean.h"
//#include "triplestore.h"
//#include "triplestore-private.h"


/**
  A subject-predicate-object triplet used to represent a relation.

  Triplets are only exposed as a type to make the implementation of
  Collections sane.  Normal Entity instances are supposed to be
  independent and should not define relations.
*/
typedef struct _XTriplet DLiteRelation;

typedef struct _DLiteProperty  DLiteProperty;
typedef struct _DLiteDimension DLiteDimension;



/** Basic data types */
typedef enum _DLiteType {
  dliteBlob,             /*!< Binary blob, sequence of bytes */
  dliteBool,             /*!< Boolean */
  dliteInt,              /*!< Signed integer */
  dliteUInt,             /*!< Unigned integer */
  dliteFloat,            /*!< Floating point */
  dliteFixString,        /*!< Fix-sized NUL-terminated string */
  dliteStringPtr,        /*!< Pointer to NUL-terminated string */

  dliteDimension,        /*!< Dimension, for entities */
  dliteProperty,         /*!< Property, for entities */
  dliteRelation,         /*!< Subject-predicate-object relation,
			      for collections */

  dliteSchemaDimension,  /*!< Schema dimension, for generic metadata */
  dliteSchemaProperty,   /*!< Schema property, for generic metadata */
  dliteSchemaRelation    /*!< Schema relation, for generic metadata */
} DLiteType;


/**
  Returns descriptive name for \a dtype or NULL on error.
*/
const char *dlite_type_get_dtypename(DLiteType dtype);

/**
  Returns the dtype corresponding to \a dtypename or -1 on error.
*/
DLiteType dlite_type_get_dtype(const char *dtypename);

/**
  Writes the type name corresponding to \a dtype and \a size to \a typename,
  which must be of size \a n.  Returns non-zero on error.
*/
int dlite_type_set_typename(DLiteType dtype, size_t size,
                            char *typename, size_t n);

/**
  Returns true if name is a DLiteType, otherwise false.
 */
bool dlite_is_type(const char *name);

/**
  Assigns \a dtype and \a size from \a typename.  Returns non-zero on error.
*/
int dlite_type_set_dtype_and_size(const char *typename,
                                  DLiteType *dtype, size_t *size);


/**
  Returns the struct alignment of the given type or 0 on error.
 */
size_t dlite_type_get_alignment(DLiteType dtype, size_t size);


/**
  Returns the offset the current struct member with dtype \a dtype and
  size \a size.  The offset of the previous struct member is \a prev_offset
  and its size is \a prev_size.

  Returns -1 on error.
 */
int dlite_type_get_member_offset(size_t prev_offset, size_t prev_size,
                                 DLiteType dtype, size_t size);

#endif /* _DLITE_TYPES_H */
