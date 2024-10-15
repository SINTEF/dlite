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
#include "strutils.h"
#include "dlite.h"
#include "dlite-python-singletons.h"
#include "dlite-pyembed.h"

#define DLITE_INSTANCE_CAPSULE_NAME ((char *)"dlite.Instance")
#define DLITE_DATA_CAPSULE_NAME ((char *)"dlite.data")

#define SWIG_FILE_WITH_INIT  /* tell numpy that we initialize it in %init */

  /* Set this to an exception object (e.g. PyExc_StopIteration) to raise
     another exceptions than DLiteError.  No need to increase the reference
     count. */
  PyObject *dlite_swig_exception = NULL;

  /* forward declarations */
  char *strndup(const char *s, size_t n);
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
  dlite_init();     /* make sure that dlite is initialised */
  Py_Initialize();  /* should already be called, but just in case... */
  import_array();   /* Initialize numpy */
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

int _get_number_of_errors(void) {
  return -dliteLastError;
}

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
    default: return dlite_err(-1, "no numpy type code for bool of size %lu",
                              (unsigned long)size);
    }
  case dliteInt:
    switch (size) {
    case 1: return NPY_INT8;
    case 2: return NPY_INT16;
    case 4: return NPY_INT32;
    case 8: return NPY_INT64;
    default: return dlite_err(-1, "no numpy type code for integer of size %lu",
                              (unsigned long)size);
    }
  case dliteUInt:
    switch (size) {
    case 1: return NPY_UINT8;
    case 2: return NPY_UINT16;
    case 4: return NPY_UINT32;
    case 8: return NPY_UINT64;
    default: return dlite_err(-1, "no numpy type code for unsigned integer "
                              "of size %lu", (unsigned long)size);
    }
  case dliteFloat:
    switch (size) {
    case 4: return NPY_FLOAT32;
    case 8: return NPY_FLOAT64;
    default: return dlite_err(-1, "no numpy type code for float of size %lu",
                              (unsigned long)size);
    }
  case dliteFixString:
    /* It would have been nicer to use NPY_UNICODE, but unfortunately is
       it based on 4 byte data points (UCS4), while the lengths of dlite
       fixed strings may have any lengths.

       We therefore fall back to NPY_STRING, which is simple ASCII. */
    return NPY_STRING;
  case dliteRef:
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
    dtype->elsize = (int)size;
    break;
  case dliteInt:
  case dliteUInt:
  case dliteFloat:
    assert(dtype->elsize == (int)size);
    break;
  case dliteFixString:
    dtype->elsize = (int)size;
    break;
  case dliteStringPtr:
  case dliteRef:
  case dliteDimension:
  case dliteProperty:
  case dliteRelation:
    assert(dtype->elsize == 0 || sizeof(void *));
    break;
  }
  return dtype;
}


/* Decreases refcount to dlite instance referred to by capsule `cap`. */
void dlite_swig_capsule_instance_decref(PyObject *cap)
{
  DLiteInstance *inst = PyCapsule_GetPointer(cap, DLITE_INSTANCE_CAPSULE_NAME);
  if (inst) dlite_instance_decref(inst);
}

/* Decreases refcount to dlite instance referred to by capsule `cap`. */
void dlite_swig_capsule_free(PyObject *cap)
{
  void *data = PyCapsule_GetPointer(cap, DLITE_DATA_CAPSULE_NAME);
  if (data) free(data);
}


/* Convert Python object `src` to a sequence of bytes (blob) and write
   the result to `dest`.  At most `n` bytes are written to `dest`.

   All Python types supporting the buffer protocol as well as hex-encoded
   strings are converted.

   Returns the number of bytes that are available in `src` or -1 on error.
 */
int dlite_swig_read_python_blob(PyObject *src, uint8_t *dest, size_t n)
{
  int retval=-1;
  if (PyUnicode_Check(src)) {
    Py_ssize_t len;
    const char *s = PyUnicode_AsUTF8AndSize(src, &len);
    if (!s) FAIL("failed representing string as UTF-8");
    if (strhex_decode(dest, n, s, (int)len) < 0)
      FAIL("cannot convert Python string to blob");
    retval = (int)len/2;
  } else if (PyObject_CheckBuffer(src)) {
    Py_buffer view;
    if (PyObject_GetBuffer(src, &view, PyBUF_SIMPLE)) goto fail;
    memcpy(dest, view.buf, n);
    retval = (int)(view.len);
    PyBuffer_Release(&view);
  } else {
    FAIL("Only Python types supporting the buffer protocol "
         "or (hex-encoded) strings can be converted to blob");
  }
 fail:
  return retval;
}


/* Returns the length of sequence-like object.  On error, -1 is returned. */
size_t dlite_swig_length(obj_t *obj)
{
  return (size_t)PyObject_Length(obj);
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
   `shape`: Length of each dimension.
   `type` : Type of the data.
   `size` : Size of each data element.
   `data` : Pointer to data, which should be C-ordered and continuous.
*/
obj_t *dlite_swig_get_array(DLiteInstance *inst, int ndims, int *shape,
                           DLiteType type, size_t size, void *data)
{
  int i;
  npy_intp *d=NULL;
  PyObject *obj, *cap, *retval=NULL;
  int typecode = npy_type(type, size);

  if (typecode < 0) goto fail;
  if (!(d = malloc(ndims*sizeof(npy_intp)))) FAIL("allocation failure");
  for (i=0; i<ndims; i++) d[i] = shape[i];

  switch (type) {

  case dliteStringPtr:
  case dliteRef:
  case dliteDimension:
  case dliteProperty:
  case dliteRelation:
    { /* Special handling of object types */
      int n=1;
      npy_intp itemsize;
      char *itemptr;
      PyArrayObject *arr;
      for (i=0; i<ndims; i++) n *= shape[i];
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
    cap = PyCapsule_New(inst, DLITE_INSTANCE_CAPSULE_NAME,
                        dlite_swig_capsule_instance_decref);
    if (!cap)
      FAIL("error creating capsule");
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
     shape : size of each destination dimension (length: ndims)
     type : type of destination data
     size : size of destination data element
     d : current dimension
     ptr : pointer to pointer to current destination memory (NB: updated!)
*/
static int dlite_swig_setitem(PyObject *obj, int ndims, int *shape,
                              DLiteType type, size_t size, int d, void **ptr)
{
  int i;
  if (d < ndims) {
    PyArrayObject *arr = (PyArrayObject *)obj;
    assert(!PyArray_Check(obj) || PyArray_DIM(arr, d) == shape[d]);
    for (i=0; i<shape[d]; i++) {
      int stat;
      PyObject *key = PyLong_FromLong(i);
      PyObject *item = PyObject_GetItem(obj, key);
      Py_DECREF(key);

      if (!item) return dlite_err(1, "dimension %d had no index %d", d, i);
      stat = dlite_swig_setitem(item, ndims, shape, type, size, d+1, ptr);
      Py_DECREF(item);
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
            of size `N`, where `N` is the product of all elements in `shape`
            times `size`.
   `ndims`: Number of dimensions.
   `shape` : Length of each dimension.
   `type` : Type of the data.
   `size` : Size of each data element.
   `obj`  : Pointer to target language array object.
*/
int dlite_swig_set_array(void *ptr, int ndims, int *shape,
                         DLiteType type, size_t size, obj_t *obj)
{
  int i, n=1, retval=-1;
  int typecode = npy_type(type, size);
  PyArrayObject *arr = NULL;
  int ndim_max=ndims;

  if (typecode < 0) goto fail;
  for (i=0; i<ndims; i++) n *= shape[i];
  if (!(arr = (PyArrayObject *)PyArray_ContiguousFromAny(obj, typecode,
                                                         ndims, ndims))) {
    /* Clear the error and try to convert object as-is */
    void *p = *(void **)ptr;
    PyErr_Clear();
    return dlite_swig_setitem((PyObject *)obj, ndims, shape, type, size, 0, &p);
  }

  /* Check dimensions */
  if (PyArray_TYPE(arr) == NPY_OBJECT || PyArray_TYPE(arr) == NPY_VOID)
    ndim_max = ndims+1;
  if (PyArray_NDIM(arr) < ndims || PyArray_NDIM(arr) > ndim_max)
    FAIL2("expected array with %d dimensions, got %d",
          ndims, PyArray_NDIM(arr));
  for (i=0; i<ndims; i++)
    if (PyArray_DIM(arr, i) != shape[i])
      FAIL3("expected length of dimension %d to be %d, got %ld",
            i, shape[i], (long)PyArray_DIM(arr, i));

  /* Assign memory */
  switch(type) {
  case dliteFixString:  /* must be NUL-terminated */
    {
      char *itemptr = PyArray_DATA(arr);
      char *p = *((char **)ptr);
      size_t len = ((size_t)PyArray_ITEMSIZE(arr) < size) ?
        (size_t)PyArray_ITEMSIZE(arr) : size;
      memset(p, 0, n*size);
      for (i=0; i<n; i++, itemptr+=PyArray_ITEMSIZE(arr), p+=size) {
        strncpy(p, itemptr, len);
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
        if (s == Py_None) {
          if (p[i]) free(p[i]);
        } else if (PyUnicode_Check(s)) {
          assert(s);
          Py_ssize_t len;
          const char *str = PyUnicode_AsUTF8AndSize(s, &len);
          if (!str) FAIL("failed representing string as UTF-8");
          p[i] = realloc(p[i], len+1);
          memcpy(p[i], str, len+1);
        } else {
          FAIL("expected None or unicode elements");
          Py_DECREF(s);
        }
        if (s) Py_DECREF(s);
      }
    }
    break;

  case dliteRef:
  case dliteDimension:
  case dliteProperty:
  case dliteRelation:
    {
      void *p = *(void **)ptr;
      if (dlite_swig_setitem((PyObject *)arr, ndims, shape,
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
   `shape`  : Length of each dimension will be written to this array.
             Must have at least length ndims.
   `type`  : The required type of the array.
   `size`  : The required element size for the array.
   `obj`   : Target language array object.
 */
void *dlite_swig_copy_array(int ndims, int *shape, DLiteType type,
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
  case dliteRef:
  case dliteDimension:
  case dliteProperty:
  case dliteRelation:
    if (dlite_swig_set_array(&ptr, ndims, shape, type, size, (obj_t *)arr))
      goto fail;
    break;
  default:
    memcpy(ptr, PyArray_DATA(arr), PyArray_SIZE(arr)*size);
    break;
  }

  for (i=0; i<ndims; i++)
    shape[i] = (int)PyArray_DIM(arr, i);

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
      case 8: value = (long)*((int64_t *)data); break;
      default: FAIL1("invalid integer size: %lu", (unsigned long)size);
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
      case 8: value = (unsigned long)*((uint64_t *)data); break;
      default: FAIL1("invalid unsigned integer size: %lu", (unsigned long)size);
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
      // TODO: check for overflow
      case 10: value = *((float80_t *)data); break;
#endif
#ifdef HAVE_FLOAT96_T
      case 12: value = *((float96_t *)data); break;
#endif
#ifdef HAVE_FLOAT128_T
      case 16: value = *((float128_t *)data); break;
#endif
      default: FAIL1("invalid float size: %lu", (unsigned long)size);
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

  case dliteRef:
    {
      DLiteInstance *inst = *(DLiteInstance **)data;
      if (!(obj = dlite_pyembed_from_instance(inst->uuid))) goto fail;
    }
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

  case dliteRelation:
    if (!(obj = SWIG_NewPointerObj(SWIG_as_voidptr(data),
                                   SWIGTYPE_p__Triple, 0)))
      FAIL("cannot create relation");
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
      int n = dlite_swig_read_python_blob(obj, ptr, size);
      if (n < 0) goto fail;
      if (n != (int)size)
        FAIL2("cannot read Python blob of size %d into buffer of size %d",
              n, (int)size);
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
      case 1: *((int8_t *)ptr)  =  (int8_t)value; break;
      case 2: *((int16_t *)ptr) = (int16_t)value; break;
      case 4: *((int32_t *)ptr) = (int32_t)value; break;
      case 8: *((int64_t *)ptr) = (int64_t)value; break;
      default: FAIL1("invalid integer size: %lu", (unsigned long)size);
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
      case 1: *((uint8_t *)ptr)  =  (uint8_t)value;  break;
      case 2: *((uint16_t *)ptr) = (uint16_t)value; break;
      case 4: *((uint32_t *)ptr) = (uint32_t)value; break;
      case 8: *((uint64_t *)ptr) = (uint64_t)value; break;
      default: FAIL1("invalid unsigned integer size: %lu", (unsigned long)size);
      }
      break;
    }

  case dliteFloat: {
    double value = PyFloat_AsDouble(obj);
    if (PyErr_Occurred()) FAIL("cannot convert to double");
    switch (size) {
    case 4:  *((float32_t *)ptr)  =  (float32_t)value; break;
    case 8:  *((float64_t *)ptr)  =  (float64_t)value; break;
#ifdef HAVE_FLOAT80
    case 10: *((float80_t *)ptr)  =  (float80_t)value; break;
#endif
#ifdef HAVE_FLOAT96
    case 12: *((float96_t *)ptr)  =  (float96_t)value; break;
#endif
#ifdef HAVE_FLOAT128
    case 16: *((float128_t *)ptr) = (float128_t)value; break;
#endif
    default: FAIL1("invalid float size: %lu", (unsigned long)size);
    }
  }
    break;

  case dliteFixString:
    {
      Py_ssize_t n;
      PyObject *str = PyObject_Str(obj);
      const char *s = PyUnicode_AsUTF8AndSize(str, &n);
      if (!s) {
        Py_DECREF(str);
        FAIL("cannot represent string as UTF-8");
      }
      if (n >= (Py_ssize_t)size) {
        Py_DECREF(str);
        FAIL2("Length of string is %lu. Exceeds available size: %lu",
              (unsigned long)n, (unsigned long)size);
      }
      memcpy(ptr, s, n+1);
      Py_DECREF(str);
    }
    break;

  case dliteStringPtr:
    {
      char *p;
      Py_ssize_t n;
      PyObject *str = PyObject_Str(obj);
      const char *s = PyUnicode_AsUTF8AndSize(str, &n);
      if (!s) {
        Py_DECREF(str);
        FAIL("cannot represent string as UTF-8");
      }
      if (!(p = realloc(*(void **)ptr, n+1))) {
        Py_DECREF(str);
        FAIL("allocation failure");
      }
      *(char **)ptr = p;
      if (p)
        memcpy(p, s, n+1);
      Py_DECREF(str);
    }
    break;

  case dliteRef:
    {
      DLiteInstance *inst=NULL;
      if (obj == Py_None) {
        // do nothing here, handled below...
      } else if (PyUnicode_Check(obj)) {
        const char *s = PyUnicode_AsUTF8(obj);
        if (!s) FAIL("cannot represent string as UTF-8");
        inst = dlite_instance_get(s);
      } else {
        inst = dlite_pyembed_get_instance(obj);
      }
      if (inst || obj == Py_None) {
        DLiteInstance **q = ptr;
        if (*q) dlite_instance_decref(*q);
        *q = inst;
      }
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
        if (dest->ref)         free(dest->ref);
        if (dest->shape)       free_str_array(dest->shape, dest->ndims);
        if (dest->unit)        free(dest->unit);
        if (dest->description) free(dest->description);
        dest->name  = strdup(src->name);
        dest->type  = src->type;
        dest->size  = src->size;
        if (src->ref) dest->ref = strdup(src->ref);
        dest->ndims = src->ndims;
        if (src->ndims > 0) {
          int j;
          dest->shape = malloc(src->ndims*sizeof(char *));
          for (j=0; j < src->ndims; j++)
            dest->shape[j] = strdup(src->shape[j]);
        } else
          dest->shape = NULL;
        dest->unit        = (src->unit) ? strdup(src->unit) : NULL;
        dest->description = (src->description) ? strdup(src->description) :NULL;

      } else if (PySequence_Check(obj) && PySequence_Length(obj) == 6) {
        /* We should deprecate this - either remove or change to dict... */
        PyObject *name  = PySequence_GetItem(obj, 0);
        PyObject *type  = PySequence_GetItem(obj, 1);
        PyObject *ref   = PySequence_GetItem(obj, 2);
        PyObject *shape = PySequence_GetItem(obj, 3);
        PyObject *unit  = PySequence_GetItem(obj, 4);
        PyObject *descr = PySequence_GetItem(obj, 5);
        DLiteType t;
        size_t size;
        if (name && PyUnicode_Check(name) &&
            type && PyUnicode_Check(type) &&
            dlite_type_set_dtype_and_size(PyUnicode_AsUTF8(type),
                                          &t, &size) == 0) {
          if (dest->name)        free(dest->name);
          if (dest->ref)         free(dest->ref);
          if (dest->shape)       free_str_array(dest->shape, dest->ndims);
          if (dest->unit)        free(dest->unit);
          if (dest->description) free(dest->description);
          dest->name = strdup(PyUnicode_AsUTF8(name));
          dest->type = t;
          dest->size = size;
          if (ref != Py_None) dest->ref = strdup(PyUnicode_AsUTF8(ref));
          if (shape && PyUnicode_Check(shape)) {
            const char *s = PyUnicode_AsUTF8(shape);
            int j=0, ndims=(s && *s) ? 1 : 0;
            while (s[j]) if (s[j++] == ',') ndims++;
            dest->ndims = ndims;
            dest->shape = malloc(ndims*sizeof(char *));
            s += strspn(s, " \t\n[");
            for (j=0; j<ndims; j++) {
              size_t len;
              s += strspn(s, " \t\n");
              len = strcspn(s, ",] \t\n");
              dest->shape[j] = strndup(s, len);
              s += len + 1;
            }
          }
          dest->unit = (unit && PyUnicode_Check(unit)) ?
            strdup(PyUnicode_AsUTF8(unit)) : NULL;
          dest->description = (descr && PyUnicode_Check(descr)) ?
            strdup(PyUnicode_AsUTF8(descr)) : NULL;
        } else {
          dlite_err(1, "cannot convert Python sequence to property");
        }
        Py_XDECREF(name);
        Py_XDECREF(type);
        Py_XDECREF(shape);
        Py_XDECREF(unit);
        Py_XDECREF(descr);

      } else {
        FAIL("cannot convert Python object to property");
      }
    }
    break;

  case dliteRelation:
    {
      void *p;
      if (SWIG_IsOK(SWIG_ConvertPtr(obj, &p, SWIGTYPE_p__Triple, 0))) {
        DLiteRelation *src = (DLiteRelation *)p;
        triple_reset(ptr, src->s, src->p, src->o, src->d, src->id);

      } else if (PySequence_Check(obj) || PyIter_Check(obj)) {
        int i;
        Py_ssize_t n;
        PyObject *lst;
        char **s = ptr;  /* cast DLiteRelation to 5 string pointers */
        assert(sizeof(DLiteRelation) == 5*sizeof(char *));

        /* Check that `obj` is a sequence of 3 to 5 strings */
        if (!(lst = PySequence_Fast(obj, "not a sequence or iterable")))
          FAIL("expected relation to be represented by a sequence or iterable");

        if ((n = PySequence_Fast_GET_SIZE(lst)) < 3 || n > 5) {
          Py_DECREF(lst);
          FAIL1("relations must be 3 to 5 strings, got %ld", (long)n);
        }
        for (i=0; i<n; i++) {
          PyObject *item = PySequence_Fast_GET_ITEM(lst, i);
          if (!PyUnicode_Check(item)) {
            Py_DECREF(lst);
            FAIL("all elements in a (subject(s), predicate(p), object(o)[, "
                 "datatype(d)[, id]]) relation provided as a sequence must "
                 "be strings");
          }
        }

        /* Free old values stored in the relation */
        for (i=0; i<5; i++)
          if (s[i]) {
            free(s[i]);
            s[i] = NULL;
          }

        /* Assign new values */
        for (i=0; i<n; i++) {
          PyObject *item = PySequence_Fast_GET_ITEM(lst, i);
          assert(PyUnicode_Check(item));
          const char *q = PyUnicode_AsUTF8(item);
          if (!q) FAIL("failed representing string as UTF-8");
          s[i] = strdup(q);
        }

        /* Assign id if not already provided */
        if (n < 5)
          s[4] = triple_get_id(NULL, s[0], s[1], s[2], s[3]);
        Py_DECREF(lst);

      } else if (PyMapping_Check(obj)) {
        char *msg=NULL;
        PyObject *s=NULL, *p=NULL, *o=NULL, *d=NULL, *id=NULL;
        const char *ss, *sp, *so, *sd, *sid;
        if (!(s = PyMapping_GetItemString(obj, "s")) ||
            !(p = PyMapping_GetItemString(obj, "p")) ||
            !(o = PyMapping_GetItemString(obj, "o")))
          msg = "Relations must have 's', 'p' and 'o' items";
        if (!msg && !(PyUnicode_Check(s) && (ss = PyUnicode_AsUTF8(s)) &&
                      PyUnicode_Check(p) && (sp = PyUnicode_AsUTF8(p)) &&
                      PyUnicode_Check(o) && (so = PyUnicode_AsUTF8(o))))
          msg = "Relation 's', 'p', 'o' items must be strings";
        if (!msg && PyMapping_HasKeyString(obj, "d") &&
            (!(d = PyMapping_GetItemString(obj, "d")) ||
             !PyUnicode_Check(d) || !(sd = PyUnicode_AsUTF8(d))))
          msg = "If given, relation d (datatype) must be a string";
        if (!msg && PyMapping_HasKeyString(obj, "id") &&
            (!(id = PyMapping_GetItemString(obj, "id")) ||
             !PyUnicode_Check(id) || !(sid = PyUnicode_AsUTF8(id))))
          msg = "If given, relation id must be a string";
        if (!msg)
          triple_reset(ptr, ss, sp, so,
                       (d) ? sd : NULL, (id) ? sid : NULL);
        Py_XDECREF(s);
        Py_XDECREF(p);
        Py_XDECREF(o);
        Py_XDECREF(d);
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
  int j, n=i, *shape=NULL;
  void **ptr;
  DLiteProperty *p;
  obj_t *obj=NULL;

  /* Properly raise StopIteration */
  if (i == (int)inst->meta->_nproperties) {
    dlite_swig_exception = PyExc_StopIteration;
    return NULL;
  }

  PyErr_Clear();
  if (n < 0) n += (int)inst->meta->_nproperties;
  if (n < 0 || n >= (int)inst->meta->_nproperties)
    return dlite_err(-1, "Property index %d is out or range: %s",
                     i, inst->meta->uri), NULL;
  dlite_instance_sync_to_properties(inst);
  ptr = DLITE_PROP(inst, n);
  p = inst->meta->_properties + n;
  if (p->ndims == 0) {
    obj = dlite_swig_get_scalar(p->type, p->size, ptr);
  } else {
    if (!(shape = malloc(p->ndims*sizeof(int)))) FAIL("allocation failure");
    for (j=0; j<p->ndims; j++) {
      if (!p->shape[j])
        FAIL2("missing dimension %d of property %d", j, i);
      shape[j] = (int)DLITE_PROP_DIM(inst, i, j);
    }
    obj = dlite_swig_get_array(inst, p->ndims, shape, p->type, p->size, *ptr);
  }
 fail:
  if (shape) free(shape);
  return obj;
}



/* Sets property `i` to the value pointed to by the target language
   object `obj`.  Returns non-zero on error. */
int dlite_swig_set_property_by_index(DLiteInstance *inst, int i, obj_t *obj)
{
  int j, *shape=NULL, status=-1, n=i;
  void *ptr;
  DLiteProperty *p;

  PyErr_Clear();
  if (n < 0) n += (int)inst->meta->_nproperties;
  if (n < 0 || n >= (int)inst->meta->_nproperties)
    FAIL2("Property index %d is out or range: %s", i, inst->meta->uri);
  ptr = DLITE_PROP(inst, n);
  p = inst->meta->_properties + n;

  if (p->ndims == 0) {
    if (dlite_swig_set_scalar(ptr, p->type, p->size, obj)) goto fail;
  } else {
    if (!(shape = malloc(p->ndims*sizeof(int)))) FAIL("allocation failure");
    for (j=0; j<p->ndims; j++) {
      if (!p->shape[j])
        FAIL2("missing dimension %d of property %d", j, i);
      shape[j] = (int)DLITE_PROP_DIM(inst, i, j);
    }
    if (dlite_swig_set_array(ptr, p->ndims, shape, p->type, p->size, obj))
      goto fail;
  }
  if (dlite_instance_sync_from_properties(inst)) goto fail;

  status = 0;
 fail:
  if (shape) free(shape);
  return status;
}


/* Expose PyRun_File() to python. Returns NULL on error. */
PyObject *dlite_run_file(const char *path, PyObject *globals, PyObject *locals)
{
  char *basename=NULL;
  FILE *fp=NULL;
  PyObject *result=NULL;
  if (!(basename = fu_basename(path)))
    FAILCODE(dliteMemoryError, "cannot allocate path base name");
  if (!(fp = fopen(path, "rt")))
    FAILCODE1(dliteIOError, "cannot open python file: %s", path);
  if (!(result = PyRun_File(fp, basename, Py_file_input, globals, locals)))
    dlite_err(dlitePythonError, "cannot run python file: %s", path);

 fail:
  if (fp) fclose(fp);
  if (basename) free(basename);
  return result;
}

%}



/**********************************************
 ** Typemaps
 **********************************************/
/*
 * Input typemaps
 * --------------
 * const char *INPUT, size_t LEN             <- string
 *     String (with possible NUL-bytes)
 * int, struct _DLiteDimension *             <- numpy array
 *     Array of dimensions.
 * int, struct _DLiteProperty *              <- numpy array
 *     Array of properties.
 * struct _DLiteInstance **, int             <- numpy array
 *     Array of DLiteInstance's.
 * unsigned char *INPUT_BYTES, size_t LEN    <- bytes
 *     Bytes object (with explicit size)
 * unsigned char *INPUT_BYTES                <- bytes
 *     Bytes object (without explicit size)
 *
 * Argout typemaps
 * ---------------
 * char **ARGOUT, size_t *LENGTH              -> string
 *     This assumes that the wrapped function assignes *ARGOUT to
 *     an malloc'ed buffer.
 * char **ARGOUT_STRING, size_t *LENGTH       -> string
 *     Assumes that *ARGOUT_STRING is malloc()'ed by the wrapped function.
 * unsigned char **ARGOUT_BYTES, size_t *LEN  -> bytes
 *     This assumes that the wrapped function assignes *ARGOUT_BYTES to
 *     an malloc'ed buffer.
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

/* String (with possible NUL-bytes) */
%typemap("doc") (const char *INPUT, size_t LEN)
  "string"
%typemap(in, numinputs=1) (const char *INPUT, size_t LEN) (Py_ssize_t tmp) {
  $1 = (char *)PyUnicode_AsUTF8AndSize($input, &tmp);
  $2 = tmp;
}

/* Array of input dimensions */
%typemap("doc") (struct _DLiteDimension *dimensions, int ndimensions)
  "Array of input dimensions"
%typemap(in) (struct _DLiteDimension *dimensions, int ndimensions) {
  $1 = NULL;
  if (!PySequence_Check($input))
    SWIG_exception(SWIG_TypeError, "Expected a sequence");
  $2 = (int)PySequence_Length($input);
  if (!($1 = calloc($2, sizeof(DLiteDimension))))
    SWIG_exception(SWIG_MemoryError, "Allocation failure");
  if (dlite_swig_set_array(&$1, 1, &$2, dliteDimension,
                           sizeof(DLiteDimension), $input)) SWIG_fail;
}
%typemap(freearg) (struct _DLiteDimension *dimensions, int ndimensions) {
  if ($1) {
    int i;
    for (i=0; i<$2; i++) {
      DLiteDimension *d = $1 + i;
      free(d->name);
      if (d->description) free(d->description);
    }
    free($1);
  }
}
%typemap(typecheck, precedence=SWIG_TYPECHECK_STRING_ARRAY)
  (struct _DLiteDimension *dimensions, int ndimensions) {
  PyObject *item0=NULL;
  void *vptr;
  $2 = 0;
  if (PySequence_Check($input) &&
      (PySequence_Length($input) == 0 ||
       ((item0 = PySequence_GetItem($input, 0)) &&
        SWIG_IsOK(SWIG_ConvertPtr(item0, &vptr, $1_descriptor, 0)))))
    $2 = 1;
  Py_XDECREF(item0);
 }

/* Array of input properties */
%typemap("doc") (struct _DLiteProperty *properties, int nproperties)
  "Array of input properties"
%typemap(in) (struct _DLiteProperty *properties, int nproperties) {
  $1 = NULL;
  if (!PySequence_Check($input))
    SWIG_exception(SWIG_TypeError, "Expected a sequence");
  $2 = (int)PySequence_Length($input);
  if (!($1 = calloc($2, sizeof(DLiteProperty))))
    SWIG_exception(SWIG_MemoryError, "Allocation failure");
  if (dlite_swig_set_array(&$1, 1, &$2, dliteProperty,
                           sizeof(DLiteProperty), $input)) SWIG_fail;
}
%typemap(freearg) (struct _DLiteProperty *properties, int nproperties) {
  if ($1) {
    int i;
    for (i=0; i<$2; i++) {
      DLiteProperty *p = $1 + i;
      free(p->name);
      if (p->shape) free_str_array(p->shape, p->ndims);
      if (p->unit) free(p->unit);
      if (p->description) free(p->description);
    }
    free($1);
  }
}
%typemap(typecheck, precedence=SWIG_TYPECHECK_STRING_ARRAY)
  (struct _DLiteProperty *properties, int nproperties) {
  PyObject *item=NULL;
  void *vptr;
  $2 = 0;
  if (PySequence_Check($input) &&
      (PySequence_Length($input) == 0 ||
       ((item = PySequence_GetItem($input, 0)) &&
        SWIG_IsOK(SWIG_ConvertPtr(item, &vptr, $1_descriptor, 0)))))
    $2 = 1;
  Py_XDECREF(item);
}

/* Array of input instances */
%typemap("doc") (struct _DLiteInstance **instances, int ninstances)
  "Array of dlite instances."
%typemap(in) (struct _DLiteInstance **instances, int ninstances) {
  int i;
  if (!PySequence_Check($input))
    SWIG_exception(SWIG_TypeError, "Expected a sequence");
  $2 = (int)PySequence_Length($input);
  if (!($1 = calloc($2, sizeof(DLiteInstance **))))
    SWIG_exception(SWIG_MemoryError, "Allocation failure");
  for (i=0; i<$2; i++) {
    void *p;
    PyObject *item = PySequence_GetItem($input, i);
    if (SWIG_IsOK(SWIG_ConvertPtr(item, &p, SWIGTYPE_p__DLiteInstance, 0))) {
      $1[i] = (DLiteInstance *)p;
      dlite_instance_incref($1[i]);
    }
    Py_XDECREF(item);
  }
}
%typemap(freearg) (struct _DLiteInstance **instances, int ninstances) {
  if ($1) {
    int i;
    for (i=0; i<$2; i++)
      if ($1[i]) dlite_instance_decref($1[i]);
    free($1);
  }
}
%typemap(typecheck, precedence=SWIG_TYPECHECK_STRING_ARRAY)
  (struct _DLiteInstance **instances, int ninstances) {
  PyObject *item=NULL;
  void *vptr;
  $2 = 0;
  if (PySequence_Check($input) &&
      (PySequence_Length($input) == 0 ||
       ((item = PySequence_GetItem($input, 0)) &&
        SWIG_IsOK(SWIG_ConvertPtr(item, &vptr, $1_descriptor, 0)))))
    $2 = 1;
  Py_XDECREF(item);
}

/* Bytes object (with explicit length) */
%typemap("doc") (unsigned char *INPUT_BYTES, size_t LEN) "bytes"
%typemap(in) (unsigned char *INPUT_BYTES, size_t LEN) {
  char *tmp=NULL;
  Py_ssize_t n=0;
  if (PyBytes_Check($input)) {
    if (PyBytes_AsStringAndSize($input, &tmp, &n)) SWIG_fail;
  } else if (PyByteArray_Check($input)) {
    if ((n = PyByteArray_Size($input)) < 0) SWIG_fail;
    if (!(tmp = PyByteArray_AsString($input))) SWIG_fail;
  } else {
    SWIG_Error(SWIG_TypeError, "argument must be bytes or bytearray");
  }
  $1 = (unsigned char *)tmp;
  $2 = n;
}

/* Bytes object (without explicit length) */
%typemap("doc") unsigned char *INPUT_BYTES "bytes"
%typemap(in) unsigned char *INPUT_BYTES {
  $1 = (unsigned char *)PyBytes_AsString($input);
}

/* ---------------
 * Argout typemaps
 * --------------- */

/* Argout string */
/* Assumes that *ARGOUT_STRING is malloc()'ed by the wrapped function */
%typemap("doc") (char **ARGOUT_STRING, size_t *LENGTH) "string"
%typemap(in,numinputs=0) (char **ARGOUT_STRING, size_t *LENGTH)
  (char *tmp=NULL, Py_ssize_t n) {
  $1 = &tmp;
  $2 = (size_t *)&n;
}
%typemap(argout) (char **ARGOUT_STRING, size_t *LENGTH) {
  $result = PyUnicode_FromStringAndSize((char *)tmp$argnum, n$argnum);
}
%typemap(freearg) (char **ARGOUT_STRING, size_t *LENGTH) {
  if ($1 && *$1) free(*$1);
}

/* Argout bytes */
/* Assumes that *ARGOUT_BYTES is malloc()'ed by the wrapped function */
%typemap("doc") (unsigned char **ARGOUT_BYTES, size_t *LEN) "bytes"
%typemap(in,numinputs=0) (unsigned char **ARGOUT_BYTES, size_t *LEN)
  (unsigned char *tmp=NULL, size_t n) {
  $1 = &tmp;
  $2 = &n;
}
%typemap(argout) (unsigned char **ARGOUT_BYTES, size_t *LEN) {
  $result = PyByteArray_FromStringAndSize((char *)tmp$argnum, n$argnum);
}
%typemap(freearg) (unsigned char **ARGOUT_BYTES, size_t *LEN) {
  if ($1 && *$1) free(*$1);
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
      PyList_Append($result, PyUnicode_FromString(*p));
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
      PyList_Append($result, PyUnicode_FromString(*p));
  }
}

/* Wrap storage and mapping base */
%rename(_get_storage_base) dlite_python_storage_base;
%rename(_get_mapping_base) dlite_python_mapping_base;
PyObject *dlite_python_storage_base(void);
PyObject *dlite_python_mapping_base(void);


/* ------------------
 * Expose generic api
 * ------------------ */

%rename(errgetexc) dlite_python_module_error;
%feature("docstring", "Returns DLite exception corresponding to error code.") dlite_python_module_error;
PyObject *dlite_python_module_error(int code);

int _get_number_of_errors(void);

%rename(run_file) dlite_run_file;
%feature("docstring", "Exposing PyRun_File from the Python C API.") dlite_run_file;
PyObject *dlite_run_file(const char *path, PyObject *globals=NULL, PyObject *locals=NULL);


%pythoncode %{
for n in range(_dlite._get_number_of_errors()):
    exc = errgetexc(-n)
    setattr(_dlite, exc.__name__, exc)
DLiteStorageBase = _dlite._get_storage_base()
DLiteMappingBase = _dlite._get_mapping_base()
del n, exc


class DLiteProtocolBase:
    """Base class for Python protocol plugins."""


def instance_cast(inst, newtype=None):
    """Return instance converted to a new instance subclass.

    By default the convertion is done to the type of the underlying
    instance object.

    Any subclass of `dlite.Instance` can be provided as `newtype`.
    Only downcasting to subclasses of `inst` are permitted.  For
    casting to other types, `dlite.DLiteTypeError` is raised.
    """
    if newtype:
        subclasses = getattr(newtype, "__subclasses__")
        if type(inst) in subclasses():
            inst.__class__ = newtype
        else:
            raise _dlite.DLiteTypeError(
                f"cannot upcast {type(inst)} to {newtype}"
            )
    elif inst.meta.uri == COLLECTION_ENTITY:
        inst.__class__ = Collection
    elif inst.is_meta:
        inst.__class__ = Metadata
    return inst


# Deprecated exceptions
class DLiteSearchError(_dlite.DLiteLookupError):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        deprecation_warning(
            "0.7.0",
            "DLiteSearchError has been renamed to DLiteLookupError."
        )

%}
