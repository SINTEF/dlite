/* -*- C -*-  (not really, but good for syntax highlighting) */
/*
   This file implements a layer between CPython and dlite typemaps.
*/

%{
#include <Python.h>
#include "boolean.h"
#ifndef HAVE_STDINT_H
#include "integers.h"
#endif
#include "floats.h"
#include "dlite.h"

#define DLITE_INSTANCE_CAPSULA_NAME ((char *)"dlite.Instance")
#define DLITE_DATA_CAPSULA_NAME ((char *)"dlite.data")

#define SWIG_FILE_WITH_INIT  /* tell numpy that we initialize it in %init */
%}

/* Some cross-target language typedef's and definitions */
%inline %{
typedef PyObject obj_t;

#define DLiteSwigNone Py_None
%}

/* Forward declarations */
%{
obj_t *dlite_swig_get_scalar(DLiteType type, size_t size, void *data);
int dlite_swig_set_scalar(void *ptr, DLiteType type, size_t size, obj_t *obj);
%}


/**********************************************
 ** Module initialisation
 **********************************************/

/*
 * We use numpy for interfacing arrays.  See
 * http://docs.scipy.org/doc/numpy-1.10.0/reference/swig.interface-file.html
 * for how to use numpy.i.
 *
 * The numpy.i file itself is downloaded from
 * https://github.com/numpy/numpy/blame/master/tools/swig/numpy.i
 */

%include "numpy.i"  // slightly changed to fit out needs, search for "XXX"

%init %{
  import_array();  /* Initialize numpy */
%}

%numpy_typemaps(unsigned char, NPY_UBYTE,  size_t)
%numpy_typemaps(int32_t,       NPY_INT32,  size_t)
%numpy_typemaps(double,        NPY_DOUBLE, size_t)
%numpy_typemaps(size_t,        NPY_SIZE_T, size_t)
%numpy_typemaps(char *,        NPY_STRING, int);



/**********************************************
 ** Help functions
 **********************************************/
%{

/* Free's array of allocated strings. */
void free_str_array(char **arr, size_t len)
{
  size_t i;
  for (i=0; i<len; i++)
    if (arr[i]) free(arr[i]);
  free(arr);
}

/* Returns the numpy type code corresponding to `type` and `size` or -1 on
   error. */
int npy_type(DLiteType type, size_t size)
{
  switch (type) {
  case dliteBlob:
    return NPY_VOID;
  case dliteBool:
    assert(size == sizeof(bool));
    switch(size) {
    case 1: return NPY_BOOL;  /* numpy bool is always 1 byte */
    case 2: return NPY_UINT16;
    case 4: return NPY_UINT32;
    default: return dlite_err(-1, "no numpy type code for bool of size %zu",
                              size);
    }
  case dliteInt:
    switch (size) {
    case 1: return NPY_INT8;
    case 2: return NPY_INT16;
    case 4: return NPY_INT32;
    case 8: return NPY_INT64;
    default: return dlite_err(-1, "no numpy type code for integer of size %zu",
                              size);
    }
  case dliteUInt:
    switch (size) {
    case 1: return NPY_UINT8;
    case 2: return NPY_UINT16;
    case 4: return NPY_UINT32;
    case 8: return NPY_UINT64;
    default: return dlite_err(-1, "no numpy type code for unsigned integer "
                              "of size %zu", size);
    }
  case dliteFloat:
    switch (size) {
    case 4: return NPY_FLOAT32;
    case 8: return NPY_FLOAT64;
    default: return dlite_err(-1, "no numpy type code for float of size %zu",
                              size);
    }
  case dliteFixString:
    /* It would have been nicer to use NPY_UNICODE, but unfortunately is
       it based on 4 byte data points (UCS4), while the lengths of dlite
       fixed strings may have any lengths.

       We therefore fall back to NPY_STRING, which is simple ASCII. */
    return NPY_STRING;
  case dliteStringPtr:
  case dliteDimension:
  case dliteProperty:
  case dliteRelation:
    return NPY_OBJECT;
  }
  abort();  /* should never be reached */
}


/* Returns a new numpy PyArray_Descr corresponding to `type` and `size`.
   Returns NULL on error. */
PyArray_Descr *npy_dtype(DLiteType type, size_t size)
{
  int typecode = npy_type(type, size);
  PyArray_Descr *dtype;
  if (typecode < 0) return NULL;
  if (!(dtype = PyArray_DescrNewFromType(typecode)))
    return dlite_err(-1, "cannot create numpy array description for %s",
                     dlite_type_get_dtypename(type)), NULL;
  switch (type) {
  case dliteBlob:
  case dliteBool:
    dtype->elsize = size;
    break;
  case dliteInt:
  case dliteUInt:
  case dliteFloat:
    assert(dtype->elsize == (int)size);
    break;
  case dliteFixString:
    dtype->elsize = size;
    break;
  case dliteStringPtr:
  case dliteDimension:
  case dliteProperty:
  case dliteRelation:
    //assert(dtype->elsize == 0);
    assert(dtype->elsize == 0 || sizeof(void *));
    break;
  }
  return dtype;
}


/* Decreases refcount to dlite instance referred to by capsula `cap`. */
void dlite_swig_capsula_instance_decref(PyObject *cap)
{
  DLiteInstance *inst = PyCapsule_GetPointer(cap, DLITE_INSTANCE_CAPSULA_NAME);
  if (inst) dlite_instance_decref(inst);
}

/* Decreases refcount to dlite instance referred to by capsula `cap`. */
void dlite_swig_capsula_free(PyObject *cap)
{
  void *data = PyCapsule_GetPointer(cap, DLITE_DATA_CAPSULA_NAME);
  if (data) free(data);
}



/**********************************************
 ** Python-specific implementations
 **********************************************/

/* Clears errors */
void dlite_swig_errclr(void)
{
  dlite_errclr();
  PyErr_Clear();
}


/* Returns a new array object for the target language or NULL on error.

   `inst` : The DLite instance that own the data. If NULL, the returned
            array takes over the ownership.
   `ndims`: Number of dimensions.
   `dims` : Length of each dimension.
   `type` : Type of the data.
   `size` : Size of each data element.
   `data` : Pointer to data, which should be C-ordered and continuous.
*/
obj_t *dlite_swig_get_array(DLiteInstance *inst, int ndims, int *dims,
                           DLiteType type, size_t size, void *data)
{
  int i;
  npy_intp *d=NULL;
  PyObject *obj, *cap, *retval=NULL;
  int typecode = npy_type(type, size);

  if (typecode < 0) goto fail;
  if (!(d = malloc(ndims*sizeof(npy_intp)))) FAIL("allocation failure");
  for (i=0; i<ndims; i++) d[i] = dims[i];

  switch (type) {

  case dliteStringPtr:
  case dliteDimension:
  case dliteProperty:
  case dliteRelation:
    { /* Special handling of object types */
      int n=1;
      npy_intp itemsize;
      char *itemptr;
      PyArrayObject *arr;
      for (i=0; i<ndims; i++) n *= dims[i];
      if (!(obj = PyArray_EMPTY(ndims, d, typecode, 0)))
        FAIL("not able to create numpy array");
      arr = (PyArrayObject *)obj;
      itemsize = PyArray_ITEMSIZE(arr);
      itemptr = PyArray_DATA(arr);
      char *ptr = data;
      for (i=0; i<n; i++, itemptr+=itemsize, ptr+=size) {
        PyObject *item;
        if (!(item = dlite_swig_get_scalar(type, size, ptr)))
          goto fail;
        if (PyArray_SETITEM(arr, itemptr, item))
          FAIL1("cannot set item of type %s", dlite_type_get_dtypename(type));
        Py_DECREF(item);
      }
      break;
    }

  default:
    { /* All other types */
      PyArray_Descr *dtype = npy_dtype(type, size);
      int flags = NPY_ARRAY_CARRAY;
      if (inst) flags |= NPY_ARRAY_OWNDATA;
      if (!(obj = PyArray_NewFromDescr(&PyArray_Type, dtype, ndims, d,
                                       NULL, data, flags, NULL)))
        FAIL("not able to create numpy array");
      break;
    }
  }

  if (inst && type != dliteStringPtr) {
    cap = PyCapsule_New(inst, DLITE_INSTANCE_CAPSULA_NAME,
                        dlite_swig_capsula_instance_decref);
    if (!cap)
      FAIL("error creating capsula");
    if (PyArray_SetBaseObject((PyArrayObject *)obj, cap) < 0)
      FAIL("error setting numpy array base");
    dlite_instance_incref(inst);
  }
  retval = obj;
 fail:
  if (d) free(d);
  return retval;
}


/* Recursive help function for setting memory from nd-array of objects. Args:
     obj : source object (array)
     ndims : number of destination dimensions
     dims : size of each destination dimension (length: ndims)
     type : type of destination data
     size : size of destination data element
     d : current dimension
     ptr : pointer to pointer to current destination memory (NB: updated!)
*/
static int dlite_swig_setitem(PyObject *obj, int ndims, int *dims,
                              DLiteType type, size_t size, int d, void **ptr)
{
  int i;
  if (d < ndims) {
    PyArrayObject *arr = (PyArrayObject *)obj;
    assert(PyArray_Check(obj));
    assert(PyArray_DIM(arr, d) == dims[d]);
    for (i=0; i<dims[d]; i++) {
      PyObject *key = PyLong_FromLong(i);
      PyObject *item = PyObject_GetItem(obj, key);
      int stat = dlite_swig_setitem(item, ndims, dims, type, size, d+1, ptr);
      Py_DECREF(item);
      Py_DECREF(key);
      if (stat) return stat;
    }
  } else {
    if (dlite_swig_set_scalar(*ptr, type, size, obj)) return 1;
    *((char **)ptr) += size;
  }
  return 0;
}


/* Sets memory pointed to by `ptr` to data from array object `obj` in the
   target language.  Returns non-zero on error.

   `ptr`  : Pointer to memory that will be written to.  Must be at least
            of size `N`, where `N` is the product of all elements in `dims`
            times `size`.
   `ndims`: Number of dimensions.
   `dims` : Length of each dimension.
   `type` : Type of the data.
   `size` : Size of each data element.
   `obj`  : Pointer to target language array object.
*/
int dlite_swig_set_array(void *ptr, int ndims, int *dims,
                         DLiteType type, size_t size, obj_t *obj)
{
  int i, n=1, retval=-1;
  int typecode = npy_type(type, size);
  PyArrayObject *arr = NULL;
  int ndim_max=ndims;

  if (typecode < 0) goto fail;
  for (i=0; i<ndims; i++) n *= dims[i];
  if (!(arr = (PyArrayObject *)PyArray_ContiguousFromAny(obj, typecode, 0, 0)))
    FAIL("cannot create contiguous array");

  /* Check dimensions */
  if (PyArray_TYPE(arr) == NPY_OBJECT || PyArray_TYPE(arr) == NPY_VOID)
    ndim_max = ndims+1;
  if (PyArray_NDIM(arr) < ndims || PyArray_NDIM(arr) > ndim_max)
    FAIL2("expected array with %d dimensions, got %d",
          ndims, PyArray_NDIM(arr));
  for (i=0; i<ndims; i++)
    if (PyArray_DIM(arr, i) != dims[i])
      FAIL3("expected length of dimension %d to be %d, got %ld",
            i, dims[i], PyArray_DIM(arr, i));

  /* Assign memory */
  switch(type) {
  case dliteFixString:  /* must be NUL-terminated */
    {
      char *itemptr = PyArray_DATA(arr);
      char *p = *((char **)ptr);
      memset(p, 0, n*size);
      for (i=0; i<n; i++, itemptr+=PyArray_ITEMSIZE(arr), p+=size) {
        strncpy(p, itemptr, PyArray_ITEMSIZE(arr));
        p[size-1] = '\0';  /* ensure NUL-termination */
      }
    }
    break;

  case dliteStringPtr:  /* array of python strings */
    {
      npy_intp itemsize = PyArray_ITEMSIZE(arr);
      char *itemptr = PyArray_DATA(arr);
      for (i=0; i<n; i++, itemptr+=itemsize) {
        char **p = *((char ***)ptr);
        PyObject *s = PyArray_GETITEM(arr, itemptr);
        assert(s);
        if (!PyUnicode_Check(s))
          FAIL("array elements should be strings");
        if (PyUnicode_READY(s))
          FAIL("failed preparing string");
        if (s == Py_None) {
          if (p[i]) free(p[i]);
        } else if (PyUnicode_Check(s)) {
          int len = PyUnicode_GET_LENGTH(s);
          p[i] = realloc(p[i], len+1);
          memcpy(p[i], PyUnicode_1BYTE_DATA(s), len);
          p[i][len] = '\0';
        } else {
          FAIL("expected None or unicode elements");
          Py_DECREF(s);
        }
        if (s) Py_DECREF(s);
      }
    }
    break;

  case dliteDimension:
  case dliteProperty:
  case dliteRelation:
    {
      void *p = *(void **)ptr;
      if (dlite_swig_setitem((PyObject *)arr, ndims, dims,
                             type, size, 0, &p)) goto fail;
    }
    break;

  default:
    memcpy(*((void **)ptr), PyArray_DATA(arr), n*size);
  }

  retval = 0;
 fail:
  if (arr) Py_DECREF(arr);
  return retval;
}


/* Copies array object from target language to newly allocated memory.
   Returns a pointer to the new allocated memory or NULL on error.

   `ndims` : Number of dimensions in `obj`.
   `dims`  : Length of each dimension will be written to this array.
             Must have at least length ndims.
   `type`  : The required type of the array.
   `size`  : The required element size for the array.
   `obj`   : Target language array object.
 */
void *dlite_swig_copy_array(int ndims, int *dims, DLiteType type,
                            size_t size, obj_t *obj)
{
  int i;
  PyArray_Descr *dtype = npy_dtype(type, size);
  PyArrayObject *arr = NULL;
  void *ptr, *retptr=NULL;

  if (!dtype) goto fail;
  if (!(arr = (PyArrayObject *)PyArray_FromAny(obj, dtype, ndims, ndims,
                                               NPY_ARRAY_DEFAULT, NULL)))
    FAIL("cannot create C-contiguous array");

  switch (type) {
  case dliteFixString:
    assert(PyArray_ITEMSIZE(arr) == (int)size - 1);
    break;
  default:
    assert(PyArray_ITEMSIZE(arr) == (int)size);
  }

  assert(ndims == PyArray_NDIM(arr));
  if (!(ptr = calloc(PyArray_SIZE(arr), size)))
    FAIL("allocation failure");

  switch (type) {
  case dliteFixString:
    for (i=0; i<PyArray_SIZE(arr); i++)
      strncpy((char *)ptr + i*size,
              (char *)(PyArray_DATA(arr)) + i*dtype->elsize,
              size);
    break;
  case dliteStringPtr:
    if (dlite_swig_set_array(&ptr, ndims, dims, type, size, (obj_t *)arr))
      goto fail;
    break;
  default:
    memcpy(ptr, PyArray_DATA(arr), PyArray_SIZE(arr)*size);
    break;
  }

  for (i=0; i<ndims; i++)
    dims[i] = PyArray_DIM(arr, i);

  retptr = ptr; /* success! */
 fail:
  if (arr) Py_DECREF(arr);
  return retptr;
}


/* Returns a new scalar object for the target language or NULL on error.

   `type` : Type of the data.
   `size` : Size of data.
   `data` : Pointer to underlying data.
 */
obj_t *dlite_swig_get_scalar(DLiteType type, size_t size, void *data)
{
  PyObject *obj=NULL;

  switch (type) {
  case dliteBlob:
    obj = PyByteArray_FromStringAndSize((const char *)data, size);
    break;

  case dliteBool:
    {
      long value = *((bool *)data);
      obj = PyBool_FromLong(value);
    }
    break;

  case dliteInt:
    {
      long value;
      switch (size) {
      case 1: value = *((int8_t *)data);  break;
      case 2: value = *((int16_t *)data); break;
      case 4: value = *((int32_t *)data); break;
      case 8: value = *((int64_t *)data); break;
      default: FAIL1("invalid integer size: %zu", size);
      }
      obj = PyLong_FromLong(value);
    }
    break;

  case dliteUInt:
    {
      unsigned long value;
      switch (size) {
      case 1: value = *((uint8_t *)data);  break;
      case 2: value = *((uint16_t *)data); break;
      case 4: value = *((uint32_t *)data); break;
      case 8: value = *((uint64_t *)data); break;
      default: FAIL1("invalid unsigned integer size: %zu", size);
      }
      obj = PyLong_FromUnsignedLong(value);
    }
    break;

  case dliteFloat:
    {
      double value;
      switch (size) {
      case 4: value = *((float32_t *)data); break;
      case 8: value = *((float64_t *)data); break;
#ifdef HAVE_FLOAT80_T
      case 10: value = *((float80_t *)data); break;
#endif
#ifdef HAVE_FLOAT128_T
      case 16: value = *((float128_t *)data); break;
#endif
      default: FAIL1("invalid float size: %zu", size);
      }
      obj = PyFloat_FromDouble(value);
    }
    break;

  case dliteFixString:
    {
      size_t len = strlen(data);
      if (len >= size) len = size-1;
      obj = PyUnicode_FromStringAndSize(data, len);
    }
    break;

  case dliteStringPtr:
    {
      char *s;
      assert(data);
      if ((s = *(char **)data))
        obj = PyUnicode_FromString(s);
      else
        Py_RETURN_NONE;
    }
    break;

  case dliteRelation:
    if (!(obj = SWIG_NewPointerObj(SWIG_as_voidptr(data),
                                   SWIGTYPE_p__Triplet, 0)))
      FAIL("cannot create relation");
    break;

  case dliteDimension:
    if (!(obj = SWIG_NewPointerObj(SWIG_as_voidptr(data),
                                   SWIGTYPE_p__DLiteDimension, 0)))
      FAIL("cannot create dimension");
    break;

  case dliteProperty:
    if (!(obj = SWIG_NewPointerObj(SWIG_as_voidptr(data),
                                   SWIGTYPE_p__DLiteProperty, 0)))
      FAIL("cannot create property");
    break;

  default:
    FAIL1("converting type \"%s\" to scalar is not yet implemented",
         dlite_type_get_dtypename(type));
    break;

  }
  if (!obj) goto fail;
  return obj;
 fail:
  if (obj) Py_DECREF(obj);
  if (!dlite_errval()) dlite_err(1, "error converting %s",
                                 dlite_type_get_dtypename(type));
  return NULL;
}


/* Sets memory pointed to by `ptr` from target language object `obj`.
   The type and size of the updated memory is described by `type` and `size`.

.  Returns non-zero on error.
 */
int dlite_swig_set_scalar(void *ptr, DLiteType type, size_t size, obj_t *obj)
{
  switch (type) {

  case dliteBlob:
    {
      size_t n;
      PyObject *bytes = PyObject_Bytes(obj);
      if (!bytes) FAIL("cannot convert object to bytes");
      assert(PyBytes_Check(bytes));
      n = PyBytes_Size(bytes);
      if (n > size) {
        Py_DECREF(bytes);
        FAIL2("Length of bytearray is %zu. Exceeds size of blob: %zu", n, size);
      }
      memset(ptr, 0, size);
      memcpy(ptr, PyBytes_AsString(bytes), n);
      Py_DECREF(bytes);
    }
    break;

  case dliteBool:
    {
      int value = PyObject_IsTrue(obj);
      if (value < 0) FAIL("cannot convert to boolean");
      *((bool *)ptr) = value;
    }
    break;

  case dliteInt:
    {
      int overflow;
#ifdef HAVE_LONG_LONG
      long long value = PyLong_AsLongLongAndOverflow(obj, &overflow);
#else
      long value = PyLong_AsLongAndOverflow(obj, &overflow);
#endif
      if (overflow) FAIL("overflow when converting to int");
      if (PyErr_Occurred()) FAIL("cannot convert to int");
      switch (size) {
      case 1: *((int8_t *)ptr) = value;  break;
      case 2: *((int16_t *)ptr) = value; break;
      case 4: *((int32_t *)ptr) = value; break;
      case 8: *((int64_t *)ptr) = value; break;
      default: FAIL1("invalid integer size: %zu", size);
      }
    }
    break;

  case dliteUInt:
    {
#ifdef HAVE_LONG_LONG
      unsigned long long value = PyLong_AsUnsignedLongLong(obj);
#else
      unsigned long value = PyLong_AsUnsignedLong(obj);
#endif
      if (PyErr_Occurred()) FAIL("cannot convert to unsigned int");
      switch (size) {
      case 1: *((uint8_t *)ptr) = value;  break;
      case 2: *((uint16_t *)ptr) = value; break;
      case 4: *((uint32_t *)ptr) = value; break;
      case 8: *((uint64_t *)ptr) = value; break;
      default: FAIL1("invalid unsigned integer size: %zu", size);
      }
      break;
    }

  case dliteFloat: {
    double value = PyFloat_AsDouble(obj);
    if (PyErr_Occurred()) FAIL("cannot convert to double");
    switch (size) {
    case 4:  *((float32_t *)ptr) = value; break;
    case 8:  *((float64_t *)ptr) = value; break;
    //case 10: *((float80_t *)ptr) = value; break;
    //case 16: *((float128_t *)ptr) = value; break;
    default: FAIL1("invalid float size: %zu", size);
    }
  }
    break;

  case dliteFixString:
    {
      size_t n;
      Py_UCS1 *s;
      PyObject *str = PyObject_Str(obj);
      if (!str) FAIL("cannot convert to string");
      if (PyUnicode_READY(str)) {
        Py_DECREF(str);
        FAIL("failed preparing string");
      }
      n = PyUnicode_GET_LENGTH(str);
      if (n > size) {
        Py_DECREF(str);
        FAIL2("Length of string is %zu. Exceeds available size: %zu", n, size);
      }
      s = PyUnicode_1BYTE_DATA(str);
      memset(ptr, 0, size);
      memcpy(ptr, s, n);
      Py_DECREF(str);
    }
    break;

  case dliteStringPtr:
    {
      size_t n;
      unsigned char *s;
      char *p;
      PyObject *str = PyObject_Str(obj);
      if (!str) FAIL("cannot convert to string");
      if (PyUnicode_READY(str)) {
        Py_DECREF(str);
        FAIL("failed preparing string");
      }
      n = PyUnicode_GET_LENGTH(str);
      s = PyUnicode_1BYTE_DATA(str);
      *(void **)ptr = realloc(*(void **)ptr, n+1);
      p = *((char **)ptr);
      memcpy(p, s, n);
      p[n] = '\0';
      Py_DECREF(str);
    }
    break;

  case dliteDimension:
    {
      DLiteDimension *dest = ptr;
      void *p;
      if (SWIG_IsOK(SWIG_ConvertPtr(obj, &p, SWIGTYPE_p__DLiteDimension, 0))) {
        DLiteDimension *src = (DLiteDimension *)p;
        if (dest->name)        free(dest->name);
        if (dest->description) free(dest->description);
        dest->name        = strdup(src->name);
        dest->description = (src->description) ? strdup(src->description) :NULL;
      } else if (PySequence_Check(obj) && PySequence_Length(obj) == 2) {
        PyObject *name = PySequence_GetItem(obj, 0);
        PyObject *descr = PySequence_GetItem(obj, 1);
        if (name && PyUnicode_Check(name)) {
          if (dest->name)        free(dest->name);
          if (dest->description) free(dest->description);
          dest->name = strdup(PyUnicode_AsUTF8(name));
          dest->description = (descr && PyUnicode_Check(descr)) ?
            strdup(PyUnicode_AsUTF8(descr)) : NULL;
        } else {
          dlite_err(1, "cannot convert Python sequence to dimension");
        }
        Py_XDECREF(name);
        Py_XDECREF(descr);
      } else {
        FAIL("cannot convert Python object to dimension");
      }
    }
    break;

  case dliteProperty:
    {
      DLiteProperty *dest = ptr;
      void *p;
      if (SWIG_IsOK(SWIG_ConvertPtr(obj, &p, SWIGTYPE_p__DLiteProperty, 0))) {
        DLiteProperty *src = (DLiteProperty *)p;
        if (dest->name)        free(dest->name);
        if (dest->dims)       free_str_array(dest->dims, dest->ndims);
        if (dest->unit)        free(dest->unit);
        if (dest->description) free(dest->description);
        dest->name  = strdup(src->name);
        dest->type  = src->type;
        dest->size  = src->size;
        dest->ndims = src->ndims;
        if (src->ndims > 0) {
          int j;
          dest->dims = malloc(src->ndims*sizeof(char *));
          for (j=0; j < src->ndims; j++)
            dest->dims[j] = strdup(src->dims[j]);
        } else
          dest->dims = NULL;
        dest->unit        = (src->unit) ? strdup(src->unit) : NULL;
        dest->description = (src->description) ? strdup(src->description) :NULL;

      } else if (PySequence_Check(obj) && PySequence_Length(obj) == 5) {
        PyObject *name = PySequence_GetItem(obj, 0);
        PyObject *type = PySequence_GetItem(obj, 1);
        PyObject *dims = PySequence_GetItem(obj, 2);
        PyObject *unit = PySequence_GetItem(obj, 3);
        PyObject *descr = PySequence_GetItem(obj, 4);
        DLiteType t;
        size_t size;
        if (name && PyUnicode_Check(name) &&
            type && PyUnicode_Check(type) &&
            dlite_type_set_dtype_and_size(PyUnicode_AsUTF8(type),
                                          &t, &size) == 0) {
          if (dest->name)        free(dest->name);
          if (dest->dims)       free_str_array(dest->dims, dest->ndims);
          if (dest->unit)        free(dest->unit);
          if (dest->description) free(dest->description);
          dest->name = strdup(PyUnicode_AsUTF8(name));
          dest->type = t;
          dest->size = size;
          if (dims && PyUnicode_Check(dims)) {
            const char *s = PyUnicode_AsUTF8(dims);
            const char *q = s;
            int j=0, ndims=(s && *s) ? 1 : 0;
            while (s[j]) if (s[j++] == ',') ndims++;
            dest->ndims = ndims;
            dest->dims = malloc(ndims*sizeof(int));
            for (j=0; j<ndims; j++) {
              if (dest->dims[j]) free(dest->dims[j]);
              dest->dims[j] = strdup(s);
              s += strcspn(q, ",") + 1;
            }
          }
          dest->unit = (unit && PyUnicode_Check(unit)) ?
            strdup(PyUnicode_AsUTF8(unit)) : NULL;
          dest->description = (descr && PyUnicode_Check(descr)) ?
            strdup(PyUnicode_AsUTF8(descr)) : NULL;
        } else
          dlite_err(1, "cannot convert Python sequence to dimension");
        Py_XDECREF(name);
        Py_XDECREF(type);
        Py_XDECREF(dims);
        Py_XDECREF(unit);
        Py_XDECREF(descr);

      } else {
        FAIL("cannot convert Python object to dimension");
      }
    }
    break;

  case dliteRelation:
    {
      void *p;
      if (SWIG_IsOK(SWIG_ConvertPtr(obj, &p, SWIGTYPE_p__Triplet, 0))) {
        DLiteRelation *src = (DLiteRelation *)p;
        triplet_reset(ptr, src->s, src->p, src->o, src->id);

      } else if (PySequence_Check(obj) || PyIter_Check(obj)) {
        int i, n;
        PyObject *lst;
        char **s = ptr;  /* cast DLiteRelation to 4 string pointers */
        assert(sizeof(DLiteRelation) == 4*sizeof(char *));

        /* Check that `obj` is a sequence of 3 or 4 strings */
        if (!(lst = PySequence_Fast(obj, "not a sequence or iterable")))
          FAIL("expected relation to be represented by a sequence or iterable");

        if ((n = PySequence_Fast_GET_SIZE(lst)) < 3 || n > 4) {
          Py_DECREF(lst);
          FAIL1("relations must be 3 or 4 strings, got %d", n);
        }
        for (i=0; i<n; i++) {
          PyObject *item = PySequence_Fast_GET_ITEM(lst, i);
          if (!PyUnicode_Check(item)) {
            Py_DECREF(lst);
            FAIL("relation subject, predicate and object must be strings");
          }
        }

        /* Free old values stored in the relation */
        for (i=0; i<4; i++) if (s[i]) free(s[i]);

        /* Assign new values */
        for (i=0; i<n; i++) {
          PyObject *item = PySequence_Fast_GET_ITEM(lst, i);
          assert(PyUnicode_Check(item));
          PyUnicode_READY(item);
          s[i] = strdup(PyUnicode_DATA(item));
        }

        /* Assign id if not already provided */
        if (n < 4)
          s[3] = triplet_get_id(NULL, s[0], s[1], s[2]);
        Py_DECREF(lst);

      } else if (PyMapping_Check(obj)) {
        char *msg=NULL;
        PyObject *s=NULL, *p=NULL, *o=NULL, *id=NULL;
        if (!(s = PyMapping_GetItemString(obj, "s")) ||
            !(p = PyMapping_GetItemString(obj, "p")) ||
            !(o = PyMapping_GetItemString(obj, "o")))
          msg = "Relations must have 's', 'p' and 'o' items";
        if (!msg && !(PyUnicode_Check(s) && PyUnicode_READY(s) == 0 &&
                      PyUnicode_Check(p) && PyUnicode_READY(p) == 0 &&
                      PyUnicode_Check(o) && PyUnicode_READY(o) == 0))
          msg = "Relation 's', 'p', 'o' items must be strings";
        if (!msg && PyMapping_HasKeyString(obj, "id") &&
            (!(id = PyMapping_GetItemString(obj, "id")) ||
             !PyUnicode_Check(id) || PyUnicode_READY(id)))
          msg = "If given, relation id must be a string";
        if (!msg)
          triplet_reset(ptr, PyUnicode_DATA(s), PyUnicode_DATA(p),
                        PyUnicode_DATA(o), (id) ? PyUnicode_DATA(id) : NULL);
        Py_XDECREF(s);
        Py_XDECREF(p);
        Py_XDECREF(o);
        Py_XDECREF(id);
        if (msg) FAIL1("%s", msg);

      } else {
        FAIL("cannot convert Python object to relation");
      }
    }
    break;
  }

  return 0;
 fail:
  return -1;
}


/* Returns a pointer to a new target language object for property `i` or
   NULL on error. */
obj_t *dlite_swig_get_property_by_index(DLiteInstance *inst, int i)
{
  int j, n=i, *dims=NULL;
  void **ptr;
  DLiteProperty *p;
  obj_t *obj=NULL;

  PyErr_Clear();
  if (n < 0) n += inst->meta->nproperties;
  if (n < 0 || n >= (int)inst->meta->nproperties)
    return dlite_err(-1, "Property index is out or range: %d", i), NULL;
  ptr = DLITE_PROP(inst, n);
  p = inst->meta->properties + n;
  if (p->ndims == 0) {
    obj = dlite_swig_get_scalar(p->type, p->size, ptr);
  } else {
    if (!(dims = malloc(p->ndims*sizeof(int)))) FAIL("allocation failure");
    for (j=0; j<p->ndims; j++) {
      if (!p->dims[j])
        FAIL2("missing dimension %d of property %d", j, i);
      dims[j] = DLITE_PROP_DIM(inst, i, j);
    }
    obj = dlite_swig_get_array(inst, p->ndims, dims, p->type, p->size, *ptr);
  }
 fail:
  if (dims) free(dims);
  return obj;
}



/* Sets property `i` to the value pointed to by the target language
   object `obj`.  Returns non-zero on error. */
int dlite_swig_set_property_by_index(DLiteInstance *inst, int i, obj_t *obj)
{
  int j, *dims=NULL, status=-1, n=i;
  void *ptr;
  DLiteProperty *p;

  PyErr_Clear();
  if (n < 0) n += inst->meta->nproperties;
  if (n < 0 || n >= (int)inst->meta->nproperties)
    FAIL1("Property index is out or range: %d", i);
  ptr = DLITE_PROP(inst, n);
  p = inst->meta->properties + n;


  if (p->ndims == 0) {
    if (dlite_swig_set_scalar(ptr, p->type, p->size, obj)) goto fail;
  } else {
    if (!(dims = malloc(p->ndims*sizeof(int)))) FAIL("allocation failure");
    for (j=0; j<p->ndims; j++) {
      if (!p->dims[j])
        FAIL2("missing dimension %d of property %d", j, i);
      dims[j] = DLITE_PROP_DIM(inst, i, j);
    }
    if (dlite_swig_set_array(ptr, p->ndims, dims, p->type, p->size, obj))
      goto fail;
  }

  status = 0;
 fail:
  if (dims) free(dims);
  return status;
}

%}



/**********************************************
 ** Typemaps
 **********************************************/
/*
 * Input typemaps
 * --------------
 * int, struct _DLiteDimension * -> numpy array
 *     Array of dimensions.
 * int, struct _DLiteProperty * -> numpy array
 *     Array of properties.
 *
 * Argout typemaps
 * ---------------
 * unsigned char **ARGOUT_BYTES, size_t *LEN
 *     Bytes.
 *
 * Out typemaps
 * ------------
 * bool -> bool
 *     Returns boolean.
 * status_t -> int
 *     Raise RuntimeError exception on non-zero return.
 * posstatus_t -> int
 *     Raise RuntimeError exception on negative return.
 * char ** -> list of strings
 *     Returns newly allocated NULL-terminated array of string pointers.
 * const_char ** -> list of strings
 *     Returns NULL-terminated array of string pointers (will not be free'ed).
 *
 **********************************************/

/* --------------
 * Input typemaps
 * -------------- */

/* Array of input dimensions */
%typemap("doc") (int ndimensions, struct _DLiteDimension *dimensions)
  "Array of input dimensions"
%typemap(in) (int ndimensions, struct _DLiteDimension *dimensions) {
  $2 = NULL;
  if (!PySequence_Check($input))
    SWIG_exception(SWIG_TypeError, "Expected a sequence");
  $1 = PySequence_Length($input);
  if (!($2 = calloc($1, sizeof(DLiteDimension))))
    SWIG_exception(SWIG_MemoryError, "Allocation failure");
  if (dlite_swig_set_array(&$2, 1, &$1, dliteDimension,
                           sizeof(DLiteDimension), $input)) SWIG_fail;
}
%typemap(freearg) (int ndimensions, struct _DLiteDimension *dimensions) {
  if ($2) {
    int i;
    for (i=0; i<$1; i++) {
      DLiteDimension *d = $2 + i;
      free(d->name);
      if (d->description) free(d->description);
    }
    free($2);
  }
}
%typemap(typecheck, precedence=SWIG_TYPECHECK_STRING_ARRAY)
  (int ndimensions, struct _DLiteDimension *dimensions) {
  PyObject *item0=NULL;
  void *vptr;
  $1 = 0;
  if (PySequence_Check($input) &&
      (PySequence_Length($input) == 0 ||
       ((item0 = PySequence_GetItem($input, 0)) &&
        SWIG_IsOK(SWIG_ConvertPtr(item0, &vptr, $2_descriptor, 0)))))
    $1 = 1;
  Py_XDECREF(item0);
 }

/* Array of input properties */
%typemap("doc") (int nproperties, struct _DLiteProperty *properties)
  "Array of input properties"
%typemap(in) (int nproperties, struct _DLiteProperty *properties) {
  $2 = NULL;
  if (!PySequence_Check($input))
    SWIG_exception(SWIG_TypeError, "Expected a sequence");
  $1 = PySequence_Length($input);
  if (!($2 = calloc($1, sizeof(DLiteProperty))))
    SWIG_exception(SWIG_MemoryError, "Allocation failure");
  if (dlite_swig_set_array(&$2, 1, &$1, dliteProperty,
                           sizeof(DLiteProperty), $input)) SWIG_fail;
}
%typemap(freearg) (int nproperties, struct _DLiteProperty *properties) {
  if ($2) {
    int i;
    for (i=0; i<$1; i++) {
      DLiteProperty *p = $2 + i;
      free(p->name);
      if (p->dims) free_str_array(p->dims, p->ndims);
      if (p->unit) free(p->unit);
      if (p->description) free(p->description);
    }
    free($2);
  }
}
%typemap(typecheck, precedence=SWIG_TYPECHECK_STRING_ARRAY)
  (int nproperties, struct _DLiteProperty *properties) {
  PyObject *item=NULL;
  void *vptr;
  $1 = 0;
  if (PySequence_Check($input) &&
      (PySequence_Length($input) == 0 ||
       ((item = PySequence_GetItem($input, 0)) &&
        SWIG_IsOK(SWIG_ConvertPtr(item, &vptr, $2_descriptor, 0)))))
    $1 = 1;
  Py_XDECREF(item);
}


/* ---------------
 * Argout typemaps
 * --------------- */

/* Argout bytes */
%typemap("doc") (unsigned char **ARGOUT_BYTES, size_t *LEN) "Bytes"
%typemap(in,numinputs=0) (unsigned char **ARGOUT_BYTES, size_t *LEN)
  (unsigned char *tmp, size_t n)
{
  $1 = &tmp;
  $2 = &n;
}
%typemap(argout) (unsigned char **ARGOUT_BYTES, size_t *LEN)
{
  $result = PyByteArray_FromStringAndSize((char *)tmp$argnum, n$argnum);
}



/* ---------------
 * Output typemaps
 * --------------- */

/* Boolean return. */
%typemap("doc") bool "Boolean"
%typemap(out) bool {
  $result = PyBool_FromLong($1);
}

%{
  typedef int status_t;     // error if non-zero, no output
  typedef int posstatus_t;  // error if negative
%}

/* Convert non-zero return value to RuntimeError exception */
%typemap("doc") status_t "Returns non-zero on error"
%typemap(out) status_t {
  if ($1) SWIG_exception_fail(SWIG_RuntimeError,
			      "non-zero return value in $symname()");
  $result = Py_None;
  Py_INCREF(Py_None); // Py_None is a singleton so increment its refcount
}

/* Raise RuntimeError exception on negative return, otherwise return int */
%typemap("doc") posstatus_t "Returns less than zero on error"
%typemap(out) posstatus_t {
  if ($1 < 0) SWIG_exception_fail(SWIG_RuntimeError,
				  "negative return value in $symname()");
  $result = PyLong_FromLong($1);
}


/* Return of newly allocated NULL-terminated array of string pointers. */
%typemap("doc") char ** "List of strings"
%typemap(out) char ** {
  $result = PyList_New(0);
  if ($1) {
    char **p;
    for (p=$1; *p; p++) {
      PyList_Append($result, PyString_FromString(*p));
      free(*p);
    }
    free($1);
  }
}

/* Converts (const char **) return value to a python list of strings */
%inline %{
  typedef char const_char;
%}
%typemap("doc") const_char ** "List of strings"
%typemap(out) const_char ** {
  $result = PyList_New(0);
  if ($1) {
    char **p;
    for (p=$1; *p; p++)
      PyList_Append($result, PyString_FromString(*p));
  }
}
