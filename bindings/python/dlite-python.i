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



/**********************************************
 ** Module initialisation
 **********************************************/


%include "numpy.i"  // slightly changed to fit out needs, search for "XXX"

%init %{
  import_array();  /* Initialize numpy */
%}


%numpy_typemaps(size_t, NPY_SIZE_T, size_t);
%numpy_typemaps(char *, NPY_STRING, int);



/**********************************************
 ** Help functions
 **********************************************/
%{

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
       it based 4 byte data points (UCS4), while the lengths of dlite
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
    /* Make space to NUL-termination */
    dtype->elsize = size - 1;
    break;
  case dliteStringPtr:
  case dliteDimension:
  case dliteProperty:
  case dliteRelation:
    assert(dtype->elsize == 0);
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
    { /* Special handling of object types */
      int n=1;
      npy_intp itemsize;
      char *itemptr;
      PyObject *item;
      PyArrayObject *arr;
      for (i=0; i<ndims; i++) n *= dims[i];
      if (!(obj = PyArray_EMPTY(ndims, d, typecode, 0)))
        FAIL("not able to create numpy array");
      arr = (PyArrayObject *)obj;
      itemsize = PyArray_ITEMSIZE(arr);
      itemptr = PyArray_DATA(arr);
      for (i=0; i<n; i++, itemptr+=itemsize) {

        if (type == dliteStringPtr) {
          char *str = *((char **)data + i);
          if (str) {
            item = PyUnicode_FromString(str);
          } else {
            item = Py_None;
            Py_INCREF(item);
          }

        } else if (type == dliteDimension) {
          DLiteDimension *dim = (DLiteDimension *)data + i;
          item = SWIG_NewPointerObj(SWIG_as_voidptr(dim),
                                    SWIGTYPE_p__DLiteDimension, 0 |  0 );

        } else if (type == dliteProperty) {
          DLiteProperty *p = (DLiteProperty *)data + i;
          item = SWIG_NewPointerObj(SWIG_as_voidptr(p),
                                    SWIGTYPE_p__DLiteProperty, 0 |  0 );

        } else if (type == dliteRelation) {
          DLiteRelation *r = (DLiteRelation *)data + i;
          item = SWIG_NewPointerObj(SWIG_as_voidptr(r),
                                    SWIGTYPE_p__Triplet, 0 |  0 );

        } else {
          assert(0);  /* should never be reached */
        }

        if (PyArray_SETITEM(arr, itemptr, item))
          FAIL1("cannot set item of type %s", dlite_type_get_dtypename(type));
      }
      break;
    }

  default:
    { /* All other types */
      //if (!(obj = PyArray_SimpleNewFromData(ndims, d, typecode, data)))
      //  FAIL("not able to create numpy array");
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
  int i, n=1, m, retval=-1;
  int typecode = npy_type(type, size);
  PyArrayObject *arr = NULL;

  if (typecode < 0) goto fail;
  for (i=0; i<ndims; i++) n *= dims[i];
  if (!(arr = (PyArrayObject *)PyArray_ContiguousFromAny(obj, typecode, 0, 0)))
    FAIL("cannot create contiguous array");
  if ((m = PyArray_SIZE(arr)) != n)
    FAIL2("expected array with total number of elements %d, got %d", n, m);
  if (type == dliteStringPtr) {
    /* Special case: handle dliteStringPtr as array of python strings */
    npy_intp itemsize = PyArray_ITEMSIZE(arr);
    char *itemptr = PyArray_DATA(arr);
    for (i=0; i<m; i++, itemptr+=itemsize) {
      char **p = *((char ***)ptr);
      PyObject *s = PyArray_GETITEM(arr, itemptr);
      assert(s);
      if (PyUnicode_READY(s)) {
        FAIL("failed preparing string");
        Py_DECREF(s);
      }
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
  } else {
    /* All other types */
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
  //int typecode = npy_type(type, size);
  PyArray_Descr *dtype = npy_dtype(type, size);
  PyArrayObject *arr = NULL;
  void *ptr=NULL;

  if (!dtype) goto fail;
  if (!(arr = (PyArrayObject *)PyArray_FromAny(obj, dtype, ndims, ndims,
                                               NPY_ARRAY_IN_ARRAY, NULL)))
    FAIL("cannot create C-contiguous array");

  switch (type) {
  case dliteFixString:
    assert(PyArray_ITEMSIZE(arr) == (int)size - 1);
    break;
  default:
    assert(PyArray_ITEMSIZE(arr) == (int)size);
  }

  assert(ndims == PyArray_NDIM(arr));
  if (!(ptr = malloc(PyArray_SIZE(arr)*size)))
    FAIL("allocation failure");
  memcpy(ptr, PyArray_DATA(arr), PyArray_SIZE(arr)*size);
  for (i=0; i<ndims; i++)
    dims[i] = PyArray_DIM(arr, i);
 fail:
  if (arr) Py_DECREF(arr);
  return ptr;
}


/* Returns a new scalar object for the target language or NULL on error.

   `type` : Type of the data.
   `size` : Size of data.
   `data` : Pointer to underlying data.
 */
obj_t *dlite_swig_get_scalar(DLiteType type, size_t size, void *data)
{
  PyObject *obj;

  switch (type) {

  case dliteBlob: {
    obj = PyByteArray_FromStringAndSize((const char *)data, size);
    break;
  }
  case dliteBool: {
    long value = *((bool *)data);
    obj = PyBool_FromLong(value);
    break;
  }
  case dliteInt: {
    long value;
    switch (size) {
    case 1: value = *((int8_t *)data);  break;
    case 2: value = *((int16_t *)data); break;
    case 4: value = *((int32_t *)data); break;
    case 8: value = *((int64_t *)data); break;
    default: FAIL1("invalid integer size: %zu", size);
    }
    obj = PyLong_FromLong(value);
    break;
  }
  case dliteUInt: {
    unsigned long value;
    switch (size) {
    case 1: value = *((uint8_t *)data);  break;
    case 2: value = *((uint16_t *)data); break;
    case 4: value = *((uint32_t *)data); break;
    case 8: value = *((uint64_t *)data); break;
    default: FAIL1("invalid unsigned integer size: %zu", size);
    }
    obj = PyLong_FromUnsignedLong(value);
    break;
  }
  case dliteFloat: {
    double value;
    switch (size) {
    case 4: value = *((float32_t *)data); break;
    case 8: value = *((float64_t *)data); break;
    //case 10: value = *((float80_t *)data); break;
    //case 16: value = *((float128_t *)data); break;
    default: FAIL1("invalid float size: %zu", size);
    }
    obj = PyFloat_FromDouble(value);
    break;
  }
  case dliteFixString: {
    obj = PyUnicode_FromStringAndSize(data, size);
    break;
  }
  case dliteStringPtr: {
    char *s;
    assert(data);
    if ((s = *(char **)data))
      obj = PyUnicode_FromString(s);
    else
      Py_RETURN_NONE;
    break;
  }
  case dliteDimension: {
    break;
  }
  case dliteProperty: {
    break;
  }
  case dliteRelation: {
    break;
  }
  }
  if (!obj) goto fail;
  return obj;
 fail:
  if (obj) Py_DECREF(obj);
  if (!dlite_errval()) dlite_err(1, "error converting %s",
                                 dlite_type_get_dtypename(type));
  return NULL;
}

/* Sets memory pointed to by `ptr` or described by `type` and `size`
   from target language object `obj`.  Returns non-zero on error.
 */
int dlite_swig_set_scalar(void *ptr, DLiteType type, size_t size, obj_t *obj)
{
  switch (type) {

  case dliteBlob: {
    size_t n;
    PyObject *bytes = PyObject_Bytes(obj);
    if (!bytes) FAIL("cannot convert object to bytes");
    n = PyByteArray_Size(bytes);
    if (n > size) {
      Py_DECREF(bytes);
      FAIL2("Length of bytearray is %zu. Exceeds size of blob: %zu", n, size);
    }
    memset(ptr, 0, size);
    memcpy(ptr, PyByteArray_AsString(obj), n);
    Py_DECREF(bytes);
    break;
  }

  case dliteBool: {
    int value = PyObject_IsTrue(obj);
    if (value < 0) FAIL("cannot convert to boolean");
    *((bool *)ptr) = value;
    break;
  }

  case dliteInt: {
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
    break;
  }

  case dliteUInt: {
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
    break;
  }

  case dliteFixString: {
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
    break;
  }

  case dliteStringPtr: {
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
    break;
  }

  case dliteDimension: {
    break;
  }

  case dliteProperty: {
    break;
  }

  case dliteRelation: {
    break;
  }

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
    /* Note that p->dims are indices into instance dimensions */
    if (!(dims = malloc(p->ndims*sizeof(int)))) FAIL("allocation failure");
    for (j=0; j<p->ndims; j++) {
      if (p->dims[j] < 0 || p->dims[j] >= (int)DLITE_NDIM(inst))
        FAIL3("dimension %d of property %d is out of range: %d",
              j, i, p->dims[j]);
      dims[j] = DLITE_DIM(inst, p->dims[j]);
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
    /* Note that p->dims are indices into instance dimensions */
    if (!(dims = malloc(p->ndims*sizeof(int)))) FAIL("allocation failure");
    for (j=0; j<p->ndims; j++) {
      if (p->dims[j] < 0 || p->dims[j] >= (int)DLITE_NDIM(inst))
        FAIL3("dimension %d of property %d is out of range: %d",
              j, i, p->dims[j]);
      dims[j] = DLITE_DIM(inst, p->dims[j]);
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
