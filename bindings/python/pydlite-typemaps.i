/* -*- C -*-  (not really, but good for syntax highlighting) */

/***************************************************************************
 * Input typemaps
 * --------------
 *     char **IN_STRING_LIST, size_t LEN
 *     unsigned char *IN_BYTES, size_t LEN
 *
 * Argout typemaps
 * ---------------
 *     char ***ARGOUT_STRING_LIST, size_t *LEN
 *     char ***ARGOUT_NULLTERM_STRING_LIST
 *     unsigned char **ARGOUT_BYTES, size_t *LEN
 *
 * Out typemaps
 * ------------
 *     char **
 *     const_char **
 *     bool
 *     INT_LIST
 *
 * NumPy
 * -----
 *     DATA_TYPE** ARGOUTVIEWM_ARRAY1, DIM_TYPE* DIM1
 *
 *     double **IN_ARRAY2D, size_t DIM1, size_t DIM2
 *     double ***IN_ARRAY3D, size_t DIM1, size_t DIM2, size_t DIM3
 *     double ***ARGOUT_ARRAY2D, size_t *DIM1, size_t *DIM2
 *     double ****ARGOUT_ARRAY3D, size_t *DIM1, size_t *DIM2, size_t *DIM3
 *
 ***************************************************************************/


/* Converts Python sequence of strings to (char **, size_t) */
/*
%typemap("doc") (char **IN_STRING_LIST, size_t LEN) "Sequence of strings."
%typemap(in,numinputs=1) (char **IN_STRING_LIST, size_t LEN) {
  if (PySequence_Check($input)) {
    $2 = PySequence_Length($input);
    int i = 0;
    $1 = (char **)malloc(($2)*sizeof(char *));
    for (i = 0; i < $2; i++) {
      PyObject *o = PySequence_GetItem($input, i);
      $1[i] = pystring(o);
      Py_DECREF(o);
      if (!$1[i]) {
        return NULL;
      }
    }
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}
%typemap(freearg) (char **IN_STRING_LIST, size_t LEN) {
  free((char *) $1);
}
*/

/* Converts (char ***, size_t) to a sequence of strings */
/*
%typemap("doc") (char ***ARGOUT_STRING_LIST, size_t *LEN) "List of strings."
%typemap(in,numinputs=0) (char ***ARGOUT_STRING_LIST, size_t *LEN) (char **s, size_t len) {
  $1 = &s;
  $2 = &len;
}
%typemap(argout) (char ***ARGOUT_STRING_LIST, size_t *LEN) {
  int i;
  $result = PyList_New(0);
  for (i=0; i<*$2; i++) {
    PyList_Append($result, PyString_FromString((*$1)[i]));
    free((*$1)[i]);
  }
  free(*$1);
}
*/


/* Converts (char ***) to a sequence of strings */
%typemap("doc") (char ***ARGOUT_NULLTERM_STRING_LIST) "List of strings."
%typemap(in,numinputs=0) (char ***OUT_NULLTER_STRING_LIST) (char **s) {
  $1 = &s;
}
%typemap(argout) (char ***ARGOUT_NULLTERM_STRING_LIST) {
  int i;
  $result = PyList_New(0);
  if ($1) {
    for (i=0; (*$1)[i]; i++) {
      PyList_Append($result, PyString_FromString((*$1)[i]));
      free((*$1)[i]);
    }
    free(*$1);
  }
}


/* Typemaps for input blob */
%typemap("doc") (unsigned char *IN_BYTES, size_t LEN) "Bytes."
%typemap(in,numinputs=1) (unsigned char *IN_BYTES, size_t LEN)
{
  $1 = (unsigned char *)PyByteArray_AsString($input);
  $2 = PyByteArray_Size($input);
}

/* Typemap for argout blob */
%typemap("doc") (unsigned char **ARGOUT_BYTES, size_t *LEN) "Bytes."
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


/**********************************************
 ** Out typemaps
 **********************************************/
%{
  typedef bool status_t;
  typedef char const_char;
  typedef int * INT_LIST;
%}

/* Converts (char **) return value to a python list of strings */
%typemap("doc") char ** "List of strings."
%typemap(out) char ** {
  char **p;
  if (!$1) SWIG_fail;
  $result = PyList_New(0);
  for (p=$1; *p; p++) {
    PyList_Append($result, PyString_FromString(*p));
    free(*p);
  }
  free($1);
}

/* Converts (const char **) return value to a python list of strings */
%typemap("doc") const_char ** "List of strings."
%typemap(out) const_char ** {
  char **p;
  if (!$1) SWIG_fail;
  $result = PyList_New(0);
  for (p=$1; *p; p++) {
    PyList_Append($result, PyString_FromString(*p));
  }
}

/* Convert false return value to RuntimeError exception
 * Consider to replace a general bool with a ``typedef bool status_t;`` */
%typemap(out) bool {
  if (!$1) SWIG_exception(SWIG_RuntimeError,
                          "false return value in softc_$symname()");
  $result = Py_None;
  Py_INCREF(Py_None); // Py_None is a singleton so increment its reference if used.
}

/* Converts a return integer array (with size given by first element)
 * to a python list of ints */
%typemap("doc") INT_LIST "List of integers."
%typemap(out) INT_LIST {
  int i, size;
  if (!$1) SWIG_fail;
  size = $1[0];
  $result = PyList_New(0);
  for (i=1; i<=size; i++)
    PyList_Append($result, PyInt_FromLong($1[i]));
  free($1);
}


/**********************************************
 ** NumPy typemaps
 **********************************************/

/* Redefine ARGOUTVIEWM_ARRAY1 to avoid appending output reults */
%typemap(argout,
         fragment="NumPy_Backward_Compatibility,NumPy_Utilities")
  (DATA_TYPE** ARGOUTVIEWM_ARRAY1, DIM_TYPE* DIM1)
{
  npy_intp dims[1] = { *$2 };
  PyObject* obj = PyArray_SimpleNewFromData(1, dims, DATA_TYPECODE, (void*)(*$1));
  PyArrayObject* array = (PyArrayObject*) obj;
  if (!array) SWIG_fail;
%#ifdef SWIGPY_USE_CAPSULE
   PyObject* cap = PyCapsule_New((void*)(*$1), SWIGPY_CAPSULE_NAME, free_cap);
%#else
   PyObject* cap = PyCObject_FromVoidPtr((void*)(*$1), free);
%#endif
%#if NPY_API_VERSION < 0x00000007
  PyArray_BASE(array) = cap;
%#else
  PyArray_SetBaseObject(array, cap);
%#endif
  //$result = SWIG_Python_AppendOutput($result,obj);
  $result = obj;
}


/* Typemaps for input array_double_2d */
%typemap(in,numinputs=1) (double **IN_ARRAY2D, size_t DIM1, size_t DIM2)
  (PyArrayObject *arr=NULL, int is_new_object)
{
  int i;
  size_t ni, nj;
  if (!(arr = obj_to_array_allow_conversion($input, NPY_DOUBLE,
					    &is_new_object))) SWIG_fail;
  if (array_numdims(arr) != 2)
    SWIG_exception_fail(SWIG_TypeError, "requires 2d array");
  $3 = ni = array_size(arr, 0);
  $2 = nj = array_size(arr, 1);
  if (!($1 = malloc(ni * sizeof(double *))))
    SWIG_exception_fail(SWIG_MemoryError, "allocation failure");
  for (i=0; i<ni; i++)
    $1[i] = (double *)((char *)array_data(arr) + i * array_stride(arr, 0));
}
%typemap(freearg) (double **IN_ARRAY2D, size_t DIM1, size_t DIM2)
{
  if ($1) free($1);
  if (arr$argnum && is_new_object$argnum) Py_DECREF(arr$argnum);
}

/* Typemap for argout array_double_2d */
%typemap(in,numinputs=0) (double ***ARGOUT_ARRAY2D, size_t *DIM1, size_t *DIM2)
  (double **tmp, size_t ni, size_t nj)
{
  $1 = &tmp;
  $3 = &ni;
  $2 = &nj;
}
%typemap(argout) (double ***ARGOUT_ARRAY2D, size_t *DIM1, size_t *DIM2)
{
  int i, j;
  size_t ni=*$3, nj=*$2;
  npy_intp dims[] = {ni, nj};
  PyObject *arr = PyArray_SimpleNew(2, dims, NPY_DOUBLE);
  for (i=0; i<ni; i++) {
    for (j=0; j<nj; j++) {
      double v = (*$1)[i][j];
      *((double *)((char *)array_data(arr) +
                   i*array_stride(arr, 0) + j*array_stride(arr, 1))) = v;
    }
  }
  $result = arr;
}


/* Typemaps for input array_double_3d */
%typemap(in,numinputs=1) (double ***IN_ARRAY3D, size_t DIM1, size_t DIM2, size_t DIM3)
  (PyArrayObject *arr=NULL, double **ptr=NULL, int is_new_object)
{
  int i, j;
  size_t ni, nj, nk;
  if (!(arr = obj_to_array_allow_conversion($input, NPY_DOUBLE,
					    &is_new_object))) return NULL;
  if (array_numdims(arr) != 3)
    SWIG_exception_fail(SWIG_TypeError, "requires 3d array");
  $4 = ni = array_size(arr, 0);
  $3 = nj = array_size(arr, 1);
  $2 = nk = array_size(arr, 2);
  if (!($1 = malloc(ni * sizeof(double *))))
    SWIG_exception_fail(SWIG_MemoryError, "error allocating $1");
  if (!(ptr = malloc(ni * nj * sizeof(double *))))
    SWIG_exception_fail(SWIG_MemoryError, "error allocating ptr$argnum");
  for (i=0; i<ni; i++) {
    for (j=0; j<nj; j++)
      ptr[i*nj+j] = (double *)((char *)array_data(arr) +
			       i*array_stride(arr, 0) +
			       j*array_stride(arr, 1));
    $1[i] = &ptr$argnum[i*nj];
  }
}
%typemap(freearg) (double ***IN_ARRAY3D, size_t DIM1, size_t DIM2, size_t DIM3)
{
  if ($1) free($1);
  if (ptr$argnum) free(ptr$argnum);
  if (arr$argnum && is_new_object$argnum) Py_DECREF(arr$argnum);
}

/* Typemap for argout array_double_3d */
%typemap(in,numinputs=0) (double ****ARGOUT_ARRAY3D,
                          size_t *DIM1, size_t *DIM2, size_t *DIM3)
  (double ***tmp, size_t ni, size_t nj, size_t nk)
{
  $1 = &tmp;
  $4 = &ni;
  $3 = &nj;
  $2 = &nk;
}
%typemap(argout) (double ****ARGOUT_ARRAY3D,
                  size_t *DIM1, size_t *DIM2, size_t *DIM3)
{
  int i, j, k;
  size_t ni=*$4, nj=*$3, nk=*$2;
  npy_intp dims[] = {ni, nj, nk};
  PyObject *arr = PyArray_SimpleNew(3, dims, NPY_DOUBLE);
  for (i=0; i<ni; i++) {
    for (j=0; j<nj; j++) {
      for (k=0; k<nk; k++) {
        double v = (*$1)[i][j][k];
        *((double *)((char *)array_data(arr) +
                     i*array_stride(arr, 0) +
                     j*array_stride(arr, 1) +
                     k*array_stride(arr, 2))) = v;
      }
    }
  }
  $result = arr;
}


/* Create numpy typemaps */
%numpy_typemaps(unsigned char, NPY_UBYTE,  size_t)
%numpy_typemaps(int32_t,       NPY_INT32,  size_t)
%numpy_typemaps(double,        NPY_DOUBLE, size_t)
