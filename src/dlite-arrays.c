#include "config.h"

#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "utils/err.h"
#include "utils/tgen.h"
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
  arr->dims = (int *)((char *)arr + sizeof(DLiteArray));
  arr->strides = (int *)((char *)arr->dims + ndims*sizeof(int));

  arr->data = data;
  arr->type = type;
  arr->size = size;
  arr->ndims = ndims;
  memcpy(arr->dims, dims, ndims*sizeof(int));
  for (i=ndims-1; i>=0; i--) {
    arr->strides[i] = size;
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
  Returns the memory size in bytes of array `arr`.
 */
size_t dlite_array_size(const DLiteArray *arr)
{
  int n, size, maxsize=0;
  for (n=0; n < arr->ndims; n++)
    if ((size = arr->strides[n]*arr->dims[n]) > maxsize) maxsize = size;
  return maxsize;
}

/*
  Returns non-zero if array `arr` describes a C-continuous memory layout.
 */
int dlite_array_is_continuous(const DLiteArray *arr)
{
  int n, size = arr->size;
  for (n=arr->ndims-1; n >= 0; n--) {
    if (arr->strides[n] != size) return 0;
    size *= arr->dims[n];
  }
  return 1;
}

/*
  Returns a pointer to data at index `ind`, where `ind` is an array
  of length `arr->ndims`.
*/
void *dlite_array_index(const DLiteArray *arr, int *ind)
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
void *dlite_array_vindex(const DLiteArray *arr, ...)
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
int dlite_array_iter_init(DLiteArrayIter *iter, const DLiteArray *arr)
{
  memset(iter, 0, sizeof(DLiteArrayIter));
  iter->arr = arr;
  if (!(iter->ind = calloc(arr->ndims, sizeof(int))))
    return err(1, "allocation failure");
  iter->ind[arr->ndims-1]--;
  return 0;
}

/*
  Deinitialise array iterator object `iter`.
*/
void dlite_array_iter_deinit(DLiteArrayIter *iter)
{
  free(iter->ind);
}

/*
  Returns the next element of from array iterator, or NULL if all elements
  has been visited.
*/
void *dlite_array_iter_next(DLiteArrayIter *iter)
{
  int n;
  DLiteArray *arr = (DLiteArray *)iter->arr;
  if (iter->ind[0] < 0) return NULL;  /* check stop indicator */
  for (n=arr->ndims-1; n>=0; n--)
    if (arr->dims[n] <= 0) return NULL;  /* check that all dimensions has
					    positive length */
  for (n=arr->ndims-1; n>=0; n--) {
    if (++iter->ind[n] < arr->dims[n]) break;
    iter->ind[n] = 0;
  }
  if (n < 0) {
    iter->ind[0] = -1;  /* stop indicator */
    return NULL;
  }
  return dlite_array_index(arr, iter->ind);
}


/*
  Returns 1 is arrays `a` and `b` are equal, zero otherwise.
 */
int dlite_array_compare(const DLiteArray *a, const DLiteArray *b)
{
  int i;
  /* check whether the array structures are equal */
  if (a->type != b->type) return 0;
  if (a->size != b->size) return 0;
  if (a->ndims != b->ndims) return 0;
  for (i=0; i < a->ndims; i++) {
    if (a->dims[i] != b->dims[i]) return 0;
    if (a->strides[i] != b->strides[i]) return 0;
  }
  /* check whether the array data are equal */
  if (memcmp(a->data, b->data, dlite_array_size(a))) return 0;
  return 1;
}


#define abs(x) (((x) > 0) ? (x) : -(x))

/*
  Returns a new array object representing a slice of `arr`.  `start`,
  `stop` and `step` has the same meaning as in Python and should be
  either NULL or arrays of length `arr->ndims`.

  For `step[n] > 0` the range for dimension `n` is increasing:

      start[n], start[n]+1, ... stop[n]-2, stop[n]-1

  For `step[n] < 0` the range for dimension `n` is decreasing:

      start[n]-1, start[n]-2, ... stop[n]+1, stop[n]

  Like Python, negative values of `start` or `stop` from the back.
  Hence index `-k` is equivalent to `arr->dims[n]-k`.

  If `start` is NULL, it will default to zero for dimensions `n` with
  positive `step` and `arr->dims[n]` for dimensions with negative
  `step`.

  If `stop` is NULL, it will default to `arr->dims[n]` for dimensions `n`
  with positive `step` and zero for dimensions with negative `step`.

  If `step` is NULL, it defaults to one.

  Returns NULL on error.

  Note
  The above behavior is not fully consistent with Python for negative
  step sizes. While the range for negative steps in dlite is given
  above, Python returns the following range:

      start[n], start[n]-1, ... stop[n]+2, stop[n]+1

  In Python, you can get the full reversed range by specifying `None`
  as the stop value.  But `None` is not a valid C integer.  If dlite
  you can get the full reversed range by setting `stop[n]` to zero.
 */
DLiteArray *dlite_array_slice(const DLiteArray *arr,
			      int *start, int *stop, int *step)
{
  int n, offset=0;
  DLiteArray *new;
  if (!(new = dlite_array_create(arr->data, arr->type, arr->size,
				 arr->ndims, arr->dims))) return NULL;
  for (n = arr->ndims-1; n >= 0; n--) {
    int s1, s2, m;
    int d = (step) ? step[n] : 1;
    if (d == 0) return err(1, "dim %d: slice step cannot be zero", n), NULL;
    if (d > 0) {
      s1 = (start) ? start[n] % arr->dims[n] : 0;
      s2 = (stop) ? (stop[n]-1) % arr->dims[n] + 1 : arr->dims[n];
      if (s1 < 0) s1 += arr->dims[n];
      if (s2 < 0) s2 += arr->dims[n];
      offset += s1 * arr->strides[n];
    } else {
      s1 = (start) ? (start[n]-1) % arr->dims[n] + 1 : arr->dims[n];
      s2 = (stop) ? (stop[n]-1) % arr->dims[n] + 1 : 0;
      if (s1 < 0) s1 += arr->dims[n];
      if (s2 < 0) s2 += arr->dims[n];
      offset += (s1 - 1) * arr->strides[n];
    }
    m = (abs(s2 - s1) + d/2) / abs(d);
    new->dims[n] = m;
    new->strides[n] *= d;
  }
  new->data = ((char *)arr->data) + offset;
  return new;
}


/*
  Returns a new array object representing `arr` with a new shape specified
  with `ndims` and `dims`.  `dims` should be compatible with the old shape.
  The current implementation also requires that `arr` is C-continuous.

  Returns NULL on error.
 */
DLiteArray *dlite_array_reshape(const DLiteArray *arr,
                                int ndims, const int *dims)
{
  int i, prod1=1, prod2=1;;
  if (!dlite_array_is_continuous(arr))
    return err(1, "can only reshape C-continuous arrays"), NULL;
  for (i=0; i < arr->ndims; i++) prod1 *= arr->dims[i];
  for (i=0; i < ndims; i++) prod2 *= dims[i];
  if (prod1 != prod2)
    return err(1, "cannot reshape to an incompatible shape"), NULL;
  return dlite_array_create(arr->data, arr->type, arr->size, ndims, dims);
}


/*
  Returns a new array object corresponding to the transpose of `arr`
  (that is, an array with reversed order of dimensions).

  Returns NULL on error.
 */
DLiteArray *dlite_array_transpose(DLiteArray *arr)
{
  int i;
  DLiteArray *new;
  if (!(new = dlite_array_create(arr->data, arr->type, arr->size,
				 arr->ndims, arr->dims))) return NULL;
  for (i=0; i < arr->ndims; i++) {
    int j = arr->ndims - 1 - i;
    new->dims[i] = arr->dims[j];
    new->strides[i] = arr->strides[j];
  }
  return new;
}


/*
  Creates a continuous copy of the data for `arr` (using malloc()) and
  updates `arr`.

  Returns a the new copy of the data or NULL on error.
 */
void *dlite_array_make_continuous(DLiteArray *arr)
{
  int n, size=arr->size;
  void *data, *p;
  char *q;
  DLiteArrayIter iter;
  for (n=0; n < arr->ndims; n++) size *= arr->dims[n];
  if (!(data = malloc(size))) return err(1, "allocation failure"), NULL;
  if (dlite_array_is_continuous(arr)) return memcpy(data, arr->data, size);

  q = data;
  dlite_array_iter_init(&iter, arr);
  while ((p = dlite_array_iter_next(&iter))) {
    memcpy(q, p, arr->size);
    q += arr->size;
  }
  dlite_array_iter_deinit(&iter);

  /* Update `arr` */
  arr->data = data;
  size = arr->size;
  for (n=arr->ndims-1; n>=0; n--) {
    arr->strides[n] = size;
    size *= arr->dims[n];
  }
  return data;
}


/*
  Print array `arr` to stream `fp`.  Returns non-zero on error.
 */
int dlite_array_printf(FILE *fp, const DLiteArray *arr, int width, int prec)
{
  void *p;
  int i, N=arr->ndims-1, NN=arr->dims[N]-1;
  DLiteArrayIter iter;
  char buf[80];
  dlite_array_iter_init(&iter, arr);
  while ((p = dlite_array_iter_next(&iter))) {
    char *sep = (iter.ind[N] < NN) ? " " : "";
    int m=0;
    for (i=arr->ndims-1; i >= 0 && iter.ind[i] == 0; i--) m++;
    if (iter.ind[N] == 0)
      for (; i >= 0; i--) fprintf(fp, " ");
    for (i=0; i<m; i++) fprintf(fp, "[");
    dlite_type_snprintf(p, arr->type, arr->size, width, prec, buf, sizeof(buf));
    fprintf(fp, "%s%s", buf, sep);
    for (i=N; i >= 0 && iter.ind[i] == arr->dims[i]-1; i--) fprintf(fp, "]");
    if (iter.ind[N] == NN) fprintf(fp, "\n");
  }
  dlite_array_iter_deinit(&iter);
  return 0;
}
