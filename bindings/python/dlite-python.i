/* -*- C -*-  (not really, but good for syntax highlighting) */
/*
   This file implements a layer between CPython and dlite typemaps.
*/

%{
#include <Python.h>

/* Returns a pointer to a new target language object for property `i`. */
void *dlite_swig_get_property_by_index(const DLiteInstance *inst, size_t i)
{
  void *ptr = DLITE_PROP(inst, i);
  DLiteProperty *p =
  return NULL;
}


/* Sets property `i` to the value pointed to by the target language
   object `obj`.  Returns non-zero on error. */
int dlite_swig_set_property_by_index(const DLiteInstance *inst, size_t i,
                                     const void *obj)
{

  return 0;
}




%}
