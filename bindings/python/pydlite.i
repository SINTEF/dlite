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
"""pydlite -- Python bindings to DLite

"""
%enddef

%module(docstring=DOCSTRING) pydlite


/**********************************************
 ** C code included in the wrapper
 **********************************************/
%{
#include <stdint.h>
#include <stdbool.h>

#include "dlite.h"

#define SWIG_FILE_WITH_INIT  // tell numpy that we initialize it in %init

  /* Initialize pydlite */
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
 ** Typemaps
 **********************************************/
/* Generic typemaps */
%include <typemaps.i>
%include <exception.i>
%include "numpy.i"  // slightly changed to fit out needs, search for "XXX"

%include "pydlite-typemaps.i"


/**********************************************
 ** Declare functions to wrap
 **********************************************/

/* Remove the softc_ prefix from the python bindings */
%feature("autodoc","2");
%feature("keyword");
%rename("%(strip:[softc_])s") "";

%include "dlite-storage.i"
