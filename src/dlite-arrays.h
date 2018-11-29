#ifndef _DLITE_ARRAYS_H
#define _DLITE_ARRAYS_H

/**
  @file
  @brief Simple API for accessing the data of multidimensional array properties

  The DLiteArray structure adds some basic functionality for accessing
  multidimensional array data.  It it not a complete array library and
  do no memory management.  It is neither optimised for speed, so don't
  use it for writing optimised solvers.

  Included features:
    - indexing
    - iteration
    - comparisons
    - slicing

  Planned features:
    - transpose
    - pretty printing
 */

#include "dlite-type.h"


/** DLite n-dimensional arrays */
typedef struct _DLiteArray {
  void *data;      /*!< pointer to array data */
  DLiteType type;  /*!< data type of elements */
  size_t size;     /*!< size of each element in bytes */
  int ndims;       /*!< number of dimensions */
  int *dims;       /*!< dimension sizes [ndims] */
  int *strides;    /*!< strides, that is number of bytes between two following
                        elements along each dimension [ndims] */
} DLiteArray;


/** Array iterator object. */
typedef struct _DLiteArrayIter {
  const DLiteArray *arr;  /*!< pointer to the array we are iterating over */
  int *ind;               /*!< the current index */
} DLiteArrayIter;


/**
  Creates a new array object.

  `data` is a pointer to array data. No copy is done.
  `type` is the type of each element.
  `size` is the size of each element.
  `ndims` is the number of dimensions.
  `dims` is the size of each dimensions.  Length: `ndims`.

  Returns the new array or NULL on error.
 */
DLiteArray *dlite_array_create(void *data, DLiteType type, size_t size,
                               int ndims, const int *dims);

/**
  Free an array object, but not the associated data.
*/
void dlite_array_free(DLiteArray *arr);


/**
  Returns the memory size in bytes of array `arr`.
 */
size_t dlite_array_size(const DLiteArray *arr);

/**
  Returns non-zero if array `arr` describes a C-continuous memory layout.
 */
int dlite_array_is_continuous(const DLiteArray *arr);

/**
  Returns a pointer to data at index `ind`, where `ind` is an array
  of length `arr->ndims`.
*/
void *dlite_array_index(const DLiteArray *arr, int *ind);


#ifdef HAVE_STDARG_H
/**
  Like dlite_array_index(), but the index is provided as `arr->ndims`
  number of variable arguments of type int.
*/
void *dlite_array_vindex(const DLiteArray *arr, ...);
#endif /* HAVE_STDARG_H */


/**
  Initialise array iterator object `iter`, for iteration over array `arr`.

  Returns non-zero on error.
*/
int dlite_array_iter_init(DLiteArrayIter *iter, const DLiteArray *arr);

/**
  Deinitialise array iterator object `iter`.
*/
void dlite_array_iter_deinit(DLiteArrayIter *iter);

/**
  Returns the next element of from array iterator, or NULL if all elements
  has been visited.
*/
void *dlite_array_iter_next(DLiteArrayIter *iter);

/**
  Returns 1 is arrays `a` and `b` are equal, zero otherwise.
 */
int dlite_array_compare(const DLiteArray *a, const DLiteArray *b);

/**
  Returns a new array object representing a slice of `arr`.  `start`,
  `stop` and `step` should be NULL or arrays of length `arr->ndims`.

  If `start` is NULL

 */
DLiteArray *dlite_array_slice(const DLiteArray *arr,
			      int *start, int *stop, int *step);

/**
  Returns a new array object representing `arr` with a new shape specified
  with `ndims` and `dims`.  `dims` should be compatible with the old shape.
  The current implementation also requires that `arr` is C-continuous.

  Returns NULL on error.
 */
DLiteArray *dlite_array_reshape(const DLiteArray *arr,
                                int ndims, const int *dims);

/**
  Print array `arr` to stream `fp`.  Returns non-zero on error.
 */
int dlite_array_printf(FILE *fp, const DLiteArray *arr, int width, int prec);

#endif /* _DLITE_ARRAYS_H */
