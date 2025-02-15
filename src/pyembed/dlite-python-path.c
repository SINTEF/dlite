/*
  Functions for generic DLite paths objects
 */
#include "utils/fileutils.h"
#include "dlite-macros.h"
#include "dlite-pyembed.h"
#include "dlite-python-singletons.h"
#include "dlite-python-path.h"


/*
  Returns the DLite installation root directory.
 */
char *dlite_python_root(void)
{
  PyObject *dlitedict, *file=NULL;
  const char *filename;
  char *dirname=NULL;

  if (!(dlitedict = dlite_python_module_dict())) goto fail;

  if (!(file = PyMapping_GetItemString(dlitedict, "__file__")))
    FAILCODE(dlitePythonError, "cannot access `dlite.__file__`");

  if (!(filename = PyUnicode_AsUTF8(file)))  // borrowed ref
    FAILCODE(dlitePythonError, "PyUnicode_AsUTF8() failed");
  dirname = fu_dirname(filename);

 fail:
  Py_XDECREF(file);
  return dirname;
}
