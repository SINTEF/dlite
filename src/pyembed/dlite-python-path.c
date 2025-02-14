/*
  Functions for generic DLite paths objects
 */
#include "dlite-macros.h"
#include "dlite-pyembed.h"
#include "dlite-python-path.h"


/*
  Returns the newly allocated string with the Python site prefix or
  NULL on error.

  This correspond to returning `site.PREFIXES[0]` from Python.
 */
char *dlite_python_site_prefix(void)
{
  PyObject *site=NULL, *prefixes=NULL, *prefix0;
  const char *prefix;

  dlite_pyembed_initialise();

  if (!(site = PyImport_ImportModule("site")))
    FAILCODE(dlitePythonError, "cannot import `site`");

  if (!(prefixes = PyObject_GetAttrString(site, "PREFIXES")))
    FAILCODE(dlitePythonError, "cannot access `site.PREFIXES`");

  if (!(prefix0 = PyList_GetItem(prefixes, 0)))  // borrowed ref
    FAILCODE(dlitePythonError, "cannot access `site.PREFIXES[0]`");

  if (!(prefix = PyUnicode_AsUTF8(prefix0)))
    FAILCODE(dlitePythonError, "PyUnicode_AsUTF8() failed");

  return strdup(prefix);

 fail:
  Py_XDECREF(site);
  Py_XDECREF(prefixes);
  return NULL;
}
