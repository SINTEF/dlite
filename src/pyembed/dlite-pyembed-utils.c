#include <Python.h>
#include "dlite-pyembed.h"


/*
  Returns non-zero if the given Python module is available.

  A side effect of calling this function is that the module will
  be imported if it is available.
*/
int dlite_pyembed_has_module(const char *module_name)
{
  dlite_pyembed_initialise();

  // PyImport_AddModule() returns a borrowed reference, so it is safe to
  // discard its return value.
  if (PyImport_AddModule(module_name) == 0) return 1;
  PyErr_Clear();
  return 0;
}
