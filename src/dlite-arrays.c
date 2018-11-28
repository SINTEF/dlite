#include "config.h"

#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "utils/err.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-arrays.h"


/*
  Creates a new array object.

  `data` is a pointer to array data. No copy is done.
  `type` is the type of each element.
  `size` is the size of each element.
  `ndims` is the number of dimensions.
  `dims` is the size of each dimensions.  Length: `ndims`.

  Returns the new array or NULL on error.
 */
DLiteArray *dlite_array_create(void *data, DLiteType type, size_t size,
                               int ndims, const int *dims)
{
  DLiteArray *arr;
  int i, asize = sizeof(DLiteArray) + 2*ndims*sizeof(int);
  assert(ndims >= 0);

  /* allocate the array object (except the data) in one chunk */
  if (!(arr = calloc(1, asize))) return err(1, "allocation failure"), NULL;
  arr->dims = (char *)arr + sizeof(DLiteArray);
  arr->strides = (char *)arr->dims + ndims*sizeof(int);

  arr->data = data;
  arr->type = type;
  arr->size = size;
  arr->ndims = ndims;
  memcpy(arr->dims, dims, ndims*sizeof(int));
  for (i=ndims-1; i>=0; i--) {
    strides[i] = size;
    size *= dims[i];
  }
  return arr;
}


/*
  Free an array object, but not the associated data.
*/
void dlite_array_free(DLiteArray *arr)
{
  free(arr);
}


/*
  Returns a pointer to data at index `ind`, where `ind` is an array
  of length `arr->ndims`.
*/
void *dlite_array_index(DLiteArray *arr, int *ind)
{
  int i, offset=0;
  for (i=0; i<arr->ndims; i++) offset += ind[i] * arr->strides[i];
  return (char *)arr->data + offset;
}


#ifdef HAVE_STDARG_H
/*
  Like dlite_array_index(), but the index is provided as `arr->ndims`
  number of variable arguments of type int.
*/
void *dlite_array_vindex(DLiteArray *arr, ...)
{
  int i, offset=0;
  va_list ap;
  va_start(ap, arr);
  for (i=0; i<arr->ndims; i++) offset += va_arg(ap, int) * arr->strides[i];
  va_end(ap);
  return (char *)arr->data + offset;
}
#endif /* HAVE_STDARG_H */


/*
  Initialise array iterator object `iter`, for iteration over array `arr`.

  Returns non-zero on error.
*/
int dlite_array_iter_init(DLiteArrayIter *iter, DLiteArray *arr)
{
  memset(iter, 0, sizeof(DLiteArrayIter));
  iter->arr = arr;
  if (!(iter->ind = calloc(arr->ndims, sizeof(int))))
    return err(1, "allocation failure");
  return 0;
}

/*
  Deinitialise array iterator object `iter`.

  Returns non-zero on error.
*/
int dlite_array_iter_deinit(DLiteArrayIter *iter)
{
  free(iter->ind);
}

/*
  Returns the next element of from array iterator, or NULL if all elements
  has been visited.
*/
void *dlite_array_iter_next(DLiteArrayIter *iter)
{
  int i, n;
  DLiteArray *arr = iter->arr;
  if (iter->ind[0] < 0) return NULL;
  for (n=arr->ndims-1; n>=0; n--) {
    if (++iter->ind[n] < arr->dims[n]) break;
    iter->ind[n] = 0;
  }
  if (n < 0) {
    iter->ind[0] = -1;
    return NULL;
  }
  return dlite_array_index(arr, iter->ind);
}

/*
  Returns a new array object representing a slice of `arr`.
 */
DLiteArray *dlite_array_slice(DLiteArray *arr, int *start, int *stop, int *step)
{


}
