#include "dlite-macros.h"
#include "dlite-misc.h"
#include "dlite-pyembed.h"
#include "dlite-pyembed-utils.h"


/*
  Returns non-zero if the given Python module is available.

  A side effect of calling this function is that the module will
  be imported if it is available.

  Use PyImport_GetModule() if you don't want to import the module.
*/
int dlite_pyembed_has_module(const char *module_name)
{
  PyObject *name, *module, *type, *value, *tb;

  dlite_pyembed_initialise();

  if (!(name = PyUnicode_FromString(module_name)))
    return dlite_err(dliteValueError, "invalid string: '%s'", module_name), 0;

  PyErr_Fetch(&type, &value, &tb);
  module = PyImport_Import(name);
  PyErr_Restore(type, value, tb);

  Py_DECREF(name);
  if (!module) return 0;
  Py_DECREF(module);
  return 1;
}
