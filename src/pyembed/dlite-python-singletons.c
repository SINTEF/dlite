#include "dlite-macros.h"
#include "dlite-pyembed.h"
#include "dlite-python-singletons.h"


/*
  Returns a pointer to __main__.__dict__ of the embedded interpreater
  or NULL on error.
*/
PyObject *dlite_python_maindict(void)
{
  PyObject *main_module, *main_dict=NULL;

  dlite_pyembed_initialise();

  if (!(main_module = PyImport_AddModule("__main__")))
    FAIL("cannot load the embedded Python __main__ module");
  if (!(main_dict = PyModule_GetDict(main_module)))
    FAIL("cannot access __dict__ of the embedded Python __main__ module");
 fail:
  return main_dict;
}


/*
  Creates a new empty singleton class and return it.

  The name of the new class is `classname`.

  Returns NULL on error.
 */
PyObject *dlite_python_mainclass(const char *classname)
{
  PyObject *main_dict, *class;
  char initcode[96];

  if (!(main_dict = dlite_python_maindict())) goto fail;

  /* Return newclass if it is already in __main__.__dict__ */
  if ((class = PyDict_GetItemString(main_dict, classname)))
    return class;

  /* Create and inject new class into the __main__ module */
  if (snprintf(initcode, sizeof(initcode), "class %s: pass\n", classname) < 0)
    FAIL("failure to create initialisation code for embedded Python "
         "__main__ module");
  if (PyRun_SimpleString(initcode))
    FAIL("failure when running embedded Python __main__ module "
         "initialisation code");
  if ((class = PyDict_GetItemString(main_dict, classname)))
    return class;

 fail:
  return NULL;
}


/* Returns the base class for storage plugins. */
PyObject *dlite_python_storage_base(void)
{
  return dlite_python_mainclass("DLiteStorageBase");
}


/* Returns the base class for mapping plugins. */
PyObject *dlite_python_mapping_base(void)
{
  return dlite_python_mainclass("DLiteMappingBase");
}


/*
  Returns a pointer to the dlite module dict of the embedded interpreater
  or NULL on error.
*/
PyObject *dlite_python_module_dict(void)
{
  PyObject *module, *dict=NULL, *name=NULL;

  dlite_pyembed_initialise();

  name = PyUnicode_FromString("dlite");
  assert(name);
  if (!(module = PyImport_GetModule(name)))
    FAIL("the dlite module cannot is not imported");
  if (!(dict = PyModule_GetDict(module)))
    FAIL("cannot access the dlite module dict");
 fail:
  Py_XDECREF(name);
  return dict;
}


/*
  Return a borrowed reference to singleton class `classname` in the dlite
  module.  The class is created if it doesn't already exists.

  Returns NULL on error.
*/
PyObject *dlite_python_module_class(const char *classname)
{
  PyObject *dict, *class;
  char initcode[96];

  if (!(dict = dlite_python_module_dict())) goto fail;

  /* Return newclass if it is already in module dict */
  if ((class = PyDict_GetItemString(dict, classname)))
    return class;

  /* Create and inject new class into the dlite module */
  if (snprintf(initcode, sizeof(initcode), "class %s: pass\n", classname) < 0)
    FAIL1("cannot create code for singleton class: %s", classname);
  if (PyRun_SimpleString(initcode))
    FAIL1("failure executing code for creating singleton class: '%s'",
          classname);
  if ((class = PyDict_GetItemString(dict, classname)))
    return class;

 fail:
  return NULL;
}


/*
  Help function returning builtin Python exception corresponding to
  error code or NULL, if no such built-in exception exists in Python.
 */
static PyObject *_python_exc(DLiteErrCode code)
{
  switch (code) {
  case dliteIOError:                return PyExc_IOError;
  case dliteRuntimeError:           return PyExc_RuntimeError;
  case dliteIndexError:             return PyExc_IndexError;
  case dliteTypeError:              return PyExc_TypeError;
  case dliteDivisionByZero:         return PyExc_ZeroDivisionError;
  case dliteOverflowError:          return PyExc_OverflowError;
  case dliteSyntaxError:            return PyExc_SyntaxError;
  case dliteValueError:             return PyExc_ValueError;
  case dliteSystemError:            return PyExc_SystemError;
  case dliteAttributeError:         return PyExc_AttributeError;
  case dliteMemoryError:            return PyExc_MemoryError;

  case dliteOSError:                return PyExc_OSError;
  case dliteKeyError:               return PyExc_KeyError;
  case dliteUnsupportedError:       return PyExc_NotImplementedError;
  case dliteStorageOpenError:       return PyExc_ConnectionError;

  default:                          return NULL;
  }
}


/*
  Returns a new Python singleton exception object for the given DLite
  error code.

  The singleton object is created the first time this function is called
  with a given `code`.  All following calles with the same `code` will return
  a reference to the same object.

  If code is zero, the DLite base exception `DLiteError` is returned.
  If code is positive, `DLiteUnknownError` is returned.

  Returns NULL if `code` is equal or smaller than `dliteLastError`.
*/
PyObject *dlite_python_module_error(DLiteErrCode code)
{
  PyObject *dict, *exc, *pyexc, *dliteError, *base;
  const char *errdescr;
  char errname[64], excname[64];
  int count, stat;

  if (code <= dliteLastError)
    FAILCODE1(dliteIndexError, "invalid error code: %d", code);
  if (code > 0) code = dliteUnknownError;

  if (!(dict = dlite_python_module_dict())) goto fail;

  /* Add DLiteError to module dict */
  if (!(dliteError = PyDict_GetItemString(dict, "DLiteError"))) {
    if (!(dliteError = PyErr_NewExceptionWithDoc(
      "dlite.DLiteError",                      // name
      "Base exception for the dlite module.",  // doc
      NULL,                                    // base
      NULL                                     // dict
    ))) FAILCODE(dlitePythonError, "failure creating dlite.DLiteError");
    stat = PyDict_SetItemString(dict, "DLiteError", dliteError);
    Py_DECREF(dliteError);
    if (stat)
      FAILCODE(dlitePythonError, "cannot assign DLiteError to module dict");
  }

  if (code == 0) return dliteError;

  /* Return exception if it already exists in module dict */
  count = snprintf(errname, sizeof(errname), "%sError", dlite_errname(code));
  assert(count > 0);
  if ((exc = PyDict_GetItemString(dict, errname))) return exc;

  /* Base exceptions */
  if ((pyexc = _python_exc(code))) {
    if (!(base = Py_BuildValue("(O,O)", dliteError, pyexc)))
      FAILCODE(dlitePythonError, "cannot build dlite exception base");
  } else {
    base = dliteError;
  }

  /* Create exception */
  count = snprintf(excname, sizeof(excname), "dlite.%s", errname);
  assert(count > 0 && count < (int)sizeof(excname));
  errdescr = dlite_errdescr(code);

  if (!(exc = PyErr_NewExceptionWithDoc(excname, errdescr, base, NULL)))
    FAILCODE1(dlitePythonError, "failure creating dlite.%s exception", errname);
  stat = PyDict_SetItemString(dict, errname, exc);
  if (stat)
    FAILCODE1(dlitePythonError, "cannot assign %s to module dict", errname);

  return exc;

 fail:
  return NULL;
}
