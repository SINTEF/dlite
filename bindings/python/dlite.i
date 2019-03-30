/* -*- C -*-  (not really, but good for syntax highlighting) */

/*
 * SWIG interface file for the python bindings.
 *
 * We use numpy for interfacing arrays.  See
 * http://docs.scipy.org/doc/numpy-1.10.0/reference/swig.interface-file.html
 * for how to use numpy.i.
 *
 * The numpy.i file itself is downloaded from
 * https://github.com/numpy/numpy/blame/master/tools/swig/numpy.i
 */

%define DOCSTRING
"""Python bindings to DLite

"""
%enddef

%module(docstring=DOCSTRING) dlite


/**********************************************
 ** C code included in the wrapper
 **********************************************/

%begin %{
  /* Disable some selected warnings in generated code */
#pragma GCC diagnostic ignored "-Wpedantic"

#if defined __GNUC__ && __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
%}

%{
#include "dlite.h"
%}


/**********************************************
 ** Load target language-specific module
 **********************************************/
%include "dlite-macros.i"

#ifdef SWIGPYTHON
%include "dlite-python.i"
#endif


/**********************************************
 ** Error handling
 **********************************************/
%include <exception.i>
%exception {
  dlite_swig_errclr();
  $action
  if (dlite_errval()) {
    SWIG_exception_fail(SWIG_RuntimeError, dlite_errmsg());
  }
}


/**********************************************
 ** Typemaps
 **********************************************/
/* Generic typemaps */
%include <typemaps.i>
%include <cstring.i>

/* Create numpy typemaps */
%numpy_typemaps(unsigned char, NPY_UBYTE,  size_t)
%numpy_typemaps(int32_t,       NPY_INT32,  size_t)
%numpy_typemaps(double,        NPY_DOUBLE, size_t)
%numpy_typemaps(size_t,        NPY_SIZE_T, size_t)

%include "dlite-typemaps.i"



/**********************************************
 ** Declare functions to wrap
 **********************************************/

/* Special features */
%feature("autodoc", "0");
%feature("keyword");

/* Remove "dlite_" prefix from bindings */
%rename("%(strip:[dlite_])s") "";

%include "dlite-misc.i"
%include "dlite-type.i"
%include "dlite-storage.i"
%include "dlite-entity.i"
%include "dlite-collection.i"
