#ifndef _DLITE_ARRAYS_H
#define _DLITE_ARRAYS_H

/**
  @file
  @brief Simple API for working with multidimensional array properties
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




#endif /* _DLITE_ARRAYS_H */
