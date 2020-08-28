/* -*- C -*-  (not really, but good for syntax highlighting) */

/*
 * SWIG interface file for dlite bindings.
 *
 * So far only Python bindings are implemented.
 */

/* Define target-language specific docstring */
#ifdef SWIGPYTHON
%include "dlite-python-docstring.i"
%module(docstring=DOCSTRING) dlite
#else
%module dlite
#endif


/**********************************************
 ** C code included in the wrapper
 **********************************************/

%begin %{
  /* Disable some selected warnings in generated code */
#if defined __GNUC__ && __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
%}

%{
#include "dlite.h"
%}


/**********************************************
 ** Load target language-specific module
 **********************************************/

/*
 * In addition to typemaps (see end of dlite-python.i), the target
 * language-specific interface files are expected to define:
 *   - obj_t: typedef for target-language objects.
 *   - DLiteSwigNone: target-language representation for None.
 */

%include "dlite-macros.i"

/* Python
 * ------ */
#ifdef SWIGPYTHON
%include "dlite-python.i"

/* Special features */
%feature("autodoc", "0");
%feature("keyword");

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
 ** Generic typemaps
 **********************************************/
%include <typemaps.i>
%include <cstring.i>


/**********************************************
 ** Declare functions to wrap
 **********************************************/

/* Remove "dlite_" prefix from bindings */
%rename("%(strip:[dlite_])s") "";

%include "dlite-misc.i"
%include "dlite-type.i"
%include "dlite-storage.i"
%include "dlite-entity.i"
%include "dlite-collection.i"
%include "dlite-mapping.i"
