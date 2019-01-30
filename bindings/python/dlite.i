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

#define SWIG_FILE_WITH_INIT  // tell numpy that we initialize it in %init


  /* Initialize dlite */
  void init(void) {
  }

%}

/**********************************************
 ** Module initialisation
 **********************************************/
%init %{
  /* Initialize numpy */
  import_array();

  /* Initialize softc */
  init();
%}


/**********************************************
 ** Error handling
 **********************************************/
%include <exception.i>
%exception {
  dlite_errclr();
  $action
  if (dlite_errval())
    SWIG_exception_fail(SWIG_RuntimeError, dlite_errmsg());
}


/**********************************************
 ** Typemaps
 **********************************************/
/* Generic typemaps */
%include <typemaps.i>
%include <cstring.i>
%include "numpy.i"  // slightly changed to fit out needs, search for "XXX"

%include "dlite-typemaps.i"


/**********************************************
 ** Declare functions to wrap
 **********************************************/

/* Remove the softc_ prefix from the python bindings */
%feature("autodoc","2");
%feature("keyword");
%rename("%(strip:[dlite_])s") "";

%include "dlite-misc.i"
%include "dlite-storage.i"
%include "dlite-entity.i"
