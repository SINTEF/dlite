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
    - reshaping
    - slicing
    - transpose
    - make_continuous
    - pretty printing
 */

#include <stdio.h>

#include "dlite-type.h"


/** DLite n-dimensional arrays */
typedef struct _DLiteArray {
  void *data;      /*!< pointer to array data */
  DLiteType type;  /*!< data type of elements */
  size_t size;     /*!< size of each element in bytes */
  int ndims;       /*!< number of dimensions */
  size_t *dims;    /*!< dimension sizes [ndims] */
  int *strides;    /*!< strides, that is number of bytes between two following
                        elements along each dimension [ndims]
                        Note: strides can be negative, so we must use
                        a signed type. */
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
                               int ndims, const size_t *dims);

/**
  Like dlite_array_create(), but with argument `order`, which can have
  the values:
    'C':  row-major (C-style) order, no reordering.
    'F':  coloumn-major (Fortran-style) order, transposed order.
*/
DLiteArray *dlite_array_create_order(void *data, DLiteType type, size_t size,
                                     int ndims, const size_t *dims, int order);

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
  `stop` and `step` has the same meaning as in Python and should be
  either NULL or arrays of length `arr->ndims`.

  For `step[n] > 0` the range for dimension `n` is increasing
  (assuming `step[n]=1`):

      start[n], start[n]+1, ... stop[n]-2, stop[n]-1

  For `step[n] < 0` the range for dimension `n` is decreasing:
  (assuming `step[n]=1`):

      start[n]-1, start[n]-2, ... stop[n]+1, stop[n]

  Like Python, negative values of `start` or `stop` counts from the back.
  Hence index `-k` is equivalent to `arr->dims[n]-|k|`.

  If `start` is NULL, it will default to zero for dimensions `n` with
  positive `step` and `arr->dims[n]` for dimensions with negative
  `step`.

  If `stop` is NULL, it will default to `arr->dims[n]` for dimensions `n`
  with positive `step` and zero for dimensions with negative `step`.

  If `step` is NULL, it defaults to one.

  Returns NULL on error.

  @note
  The above behavior is not fully consistent with Python for negative
  step sizes. While the range for negative steps in dlite is given
  above, Python returns the following range:

      start[n], start[n]-1, ... stop[n]+2, stop[n]+1

  In Python, you can get the full reversed range by specifying `None`
  as the stop value.  But `None` is not a valid C integer.  In dlite
  you can get the full reversed range by setting `stop[n]` to zero.
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
                                int ndims, const size_t *dims);


/**
  Returns a new array object corresponding to the transpose of `arr`
  (that is, an array with reversed order of dimensions).

  Returns NULL on error.

  @note
  This function does not change the underlying data.  If you want to
  convert between C and Fortran array layout, you should call
  dlite_make_continuous() on the array object returned by this
  function.
 */
DLiteArray *dlite_array_transpose(DLiteArray *arr);

/**
  Creates a continuous copy of the data for `arr` (using malloc()) and
  updates `arr`.

  Returns a the new copy of the data or NULL on error.
 */
void *dlite_array_make_continuous(DLiteArray *arr);

/**
  Print array `arr` to stream `fp`.

  The `width` and `prec` arguments corresponds to the printf() minimum
  field width and precision/length modifier.  If you set them to -1, a
  suitable value will selected according to `type`.  To ignore their
  effect, set `width` to zero or `prec` to -2.

  Returns non-zero on error.
 */
int dlite_array_printf(FILE *fp, const DLiteArray *arr, int width, int prec);

#endif /* _DLITE_ARRAYS_H */
