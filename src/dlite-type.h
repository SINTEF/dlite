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

  type      | dtype          | sizes                  | pointer                       | description                      | examples names
  ----      | -----          | -----                  | -------                       | -----------                      | --------------
  blob      | dliteBlob      | any                    | uint8_t *                     | binary blob, sequence of bytes   | blob32, blob128
  bool      | dliteBool      | sizeof(bool)           | bool *                        | boolean                          | bool
  int       | dliteInt       | 1, 2, 4, {8}           | int8_t *, int16_t *, ...      | signed integer                   | (int), int8, int16, int32, {int64}
  uint      | dliteUInt      | 1, 2, 4, {8}           | uint8_t *, uint16_t *, ...    | unsigned integer                 | (uint), uint8, uint16, uint32, {uint64}
  float     | dliteFloat     | 4, 8, {10, 16}         | float32_t *, float64_t *, ... | floating point                   | (float), (double), float32, float64, {float80, float128}
  fixstring | dliteFixString | any                    | char *                        | fix-sized NUL-terminated string  | string20
  string    | dliteStringPtr | sizeof(char *)         | char **                       | pointer to NUL-terminated string | string
  relation  | dliteRelation  | sizeof(DLiteRelation)  | DLiteRelation *               | subject-predicate-object triple  | relation
  dimension | dliteDimension | sizeof(DLiteDimension) | DLiteDimension *              | only intended for metadata       | dimension
  property  | dliteProperty  | sizeof(DLiteProperty)  | DLiteProperty *               | only intended for metadata       | property

  The column "pointer" shows the C type of the `ptr` argument for
  functions like dlite_instance_get_property() and
  dlite_instance_set_property().  Note that this pointer type is the
  same regardless we are referring to a scalar or an array.  For arrays
  the pointer points to the first element.

  The column "examples names" shows examples of how these types whould
  be written when specifying the type of a property.
    - The type names in parenthesis are included for portability with
      SOFT5, but not encouraged because their may vary between platforms.
    - The type names in curly brackets may not be defined on all
      platforms.  The headers "integers.h" and "floats.h" provides
      macros like `HAVE_INT64_T`, `HAVE_FLOAT128_T`... that can be
      used to check for availability.

  Some additional notes:
    - *blob*: is a sequence of bytes of length `size`.  When writing
      the type name, you should always append the size as shown in
      the examples in table.
    - *bool*: corresponds to the bool type as defined in <stdbool.h>.
      To support systems lacking <stdbool.h> you can use "boolean.h"
      provided by dlite.
    - *fixstring*: corresponds to `char fixstring[size]` in C. The
      size includes the terminating NUL.
    - *string*: corresponds to `char *string` in C, pointing to memory
      allocated with malloc().  If you free a string, you should always
      set the pointer to NULL, since functions like dlite_entity_free()
      otherwise will try to free it again, causing memory corruption.
    - *relation*: a subject-predicate-object triple defined in
      `triplestore.h`.  In addition have all triples an id, allowing
      a relation to refer to another relation.
    - *dimension*: Name and description of a dimension.  Only intended
      for metadata.
    - *property*: Name and full description of a property.  Only intended
      for metadata.
*/

#include <stdlib.h>

#include "utils/boolean.h"
#include "triplestore.h"


/** Expands to the struct alignment of type */
#define alignof(type) ((size_t)&((struct { char c; type d; } *)0)->d)

/** Expands to the amount of padding that should be added before `type`
    if `type` is to be added to a struct at offset `offset`. */
#define padding_at(type, offset)                                        \
  ((alignof(type) - ((offset) & (alignof(type) - 1))) & (alignof(type) - 1))


typedef struct _DLiteProperty  DLiteProperty;
typedef struct _DLiteDimension DLiteDimension;
typedef struct _Triple         DLiteRelation;


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
  dliteRelation          /*!< Subject-predicate-object relation */
} DLiteType;


/** Some flags for printing or scanning dlite types */
typedef enum _DLiteTypeFlag {
  dliteFlagDefault = 0,  /*!< Default */
  dliteFlagRaw=1,        /*!< Raw unquoted input/output */
  dliteFlagQuoted=2,     /*!< Quoted input/output */
  dliteFlagStrip=4       /*!< Strip off initial and final spaces */
} DLiteTypeFlag;


/** Function prototype that copies value from `src` to `dest`.  If
    `dest_type` and `dest_size` differs from `src_type` and `src_size`
    the value will be casted, if possible.

    If `dest_type` contains allocated data, new memory should be
    allocated for `dest`.  Information may get lost in this case.

    Returns non-zero on error or if the cast is not supported. */
typedef int
(*DLiteTypeCast)(void *dest, DLiteType dest_type, size_t dest_size,
                 const void *src, DLiteType src_type, size_t src_size);


#include "dlite-type-cast.h"


/**
  Returns descriptive name for `dtype` or NULL on error.
*/
const char *dlite_type_get_dtypename(DLiteType dtype);

/**
  Returns enum name for `dtype` or NULL on error.
 */
const char *dlite_type_get_enum_name(DLiteType dtype);

/**
  Returns the dtype corresponding to `dtypename` or -1 on error.
*/
DLiteType dlite_type_get_dtype(const char *dtypename);

/**
  Writes the type name corresponding to `dtype` and `size` to `typename`,
  which must be of size `n`.  Returns non-zero on error.
*/
int dlite_type_set_typename(DLiteType dtype, size_t size,
                            char *typename, size_t n);

/*
  Writes the fortran type name corresponding to `dtype` and `size` to
  `typename`, which must be of size `n`.  Returns non-zero on error.
*/
int dlite_type_set_ftype(DLiteType dtype, size_t size,
                         char *ftype, size_t n);

/*
  Writes the Fortran ISO_C_BINDING type name corresponding to `dtype` and
  `size` to `isoctype`, which must be of size `n`.  Returns non-zero on error.
*/
int dlite_type_set_isoctype(DLiteType dtype, size_t size,
                            char *isoctype, size_t n);

/**
  Writes C declaration to `cdecl` of a C variable with given `dtype` and `size`.
  The size of the memory pointed to by `cdecl` must be at least `n` bytes.

  If `native` is non-zero, the native typename will be written to `pcdecl`
  (e.g. "double") instead of the portable typename (e.g. "float64_t").

  `name` is the name of the C variable.

  `nref` is the number of extra * to add in front of `name`.

  Returns the number of bytes written or -1 on error.
*/
int dlite_type_set_cdecl(DLiteType dtype, size_t size, const char *name,
                         size_t nref, char *pcdecl, size_t n, int native);

/**
  Returns true if name is a DLiteType, otherwise false.
 */
bool dlite_is_type(const char *name);

/**
  Assigns `dtype` and `size` from `typename`.

  Characters other than alphanumerics or underscore may follow the
  type name.

  Returns non-zero on error.
*/
int dlite_type_set_dtype_and_size(const char *typename,
                                  DLiteType *dtype, size_t *size);


/**
  Returns non-zero if `dtype` contains allocated data, like dliteStringPtr.
 */
int dlite_type_is_allocated(DLiteType dtype);

/**
  Copies value of given dtype from `src` to `dest`.  If the dtype contains
  allocated data, new memory will be allocated for `dest`.

  Returns a pointer to the memory area `dest` or NULL on error.
*/
void *dlite_type_copy(void *dest, const void *src,
                      DLiteType dtype, size_t size);

/**
  Clears the memory pointed to by `p`.  Its type is gived by `dtype`
  and `size`.

  Returns a pointer to the memory area `p` or NULL on error.
*/
void *dlite_type_clear(void *p, DLiteType dtype, size_t size);

/**
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
                     size_t size, int width, int prec, DLiteTypeFlag flags);

/**
  Like dlite_type_print(), but prints to allocated buffer.

  Prints to position `pos` in `*dest`, which should point to a buffer
  of size `*n`.  `*dest` is reallocated if needed.

  Returns number or bytes written or a negative number on error.
 */
int dlite_type_aprint(char **dest, size_t *n, size_t pos, const void *p,
                      DLiteType dtype, size_t size, int width, int prec,
                      DLiteTypeFlag flags);

/**
  Scans a value from `src` and write it to memory pointed to by `p`.

  If `len` is non-negative, at most `len` bytes are read from `src`.

  The type and size of the scanned data is described by `dtype` and `size`,
  respectively.

  For allocated types, the memory pointed to by `p` should either be
  initialized to zero or contain valid data.

  Returns number of characters consumed or -1 on error.
 */
int dlite_type_scan(const char *src, int len, void *p, DLiteType dtype,
                    size_t size, DLiteTypeFlag flags);

/**
  Returns the struct alignment of the given type or 0 on error.
 */
size_t dlite_type_get_alignment(DLiteType dtype, size_t size);

/**
  Returns the amount of padding that should be added before `type`,
  if `type` (of size `size`) is to be added to a struct at offset `offset`.
*/
size_t dlite_type_padding_at(DLiteType dtype, size_t size, size_t offset);


/**
  Returns the offset the current struct member with dtype `dtype` and
  size `size`.  The offset of the previous struct member is `prev_offset`
  and its size is `prev_size`.

  Returns -1 on error.
 */
int dlite_type_get_member_offset(size_t prev_offset, size_t prev_size,
                                 DLiteType dtype, size_t size);


/**
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
*/
int dlite_type_ndcast(int ndims,
                      void *dest, DLiteType dest_type, size_t dest_size,
                      const size_t *dest_dims, const int *dest_strides,
                      const void *src, DLiteType src_type, size_t src_size,
                      const size_t *src_dims, const int *src_strides,
                      DLiteTypeCast castfun);

#endif /* _DLITE_TYPES_H */
