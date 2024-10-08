#ifndef _DLITE_PYTHON_SINGLETONS_H
#define _DLITE_PYTHON_SINGLETONS_H

/**
  @file
  @brief Python singletons
*/
#include <Python.h>


/**
  Returns a borrowed reference to `__main__.__dict__` or NULL on error.
*/
PyObject *dlite_python_maindict(void);

/**
  Creates a new empty singleton class in the dlite module and return it.

  The name of the new class is `classname`.

  Returns NULL on error.
 */
PyObject *dlite_python_dliteclass(const char *classname);


/**
  Returns the base class for storage plugins.
*/
PyObject *dlite_python_storage_base(void);

/**
  Returns the base class for mapping plugins.
*/
PyObject *dlite_python_mapping_base(void);


/**
  Returns a pointer to the dlite module dict of the embedded interpreater
  or NULL on error.
*/
PyObject *dlite_python_module_dict(void);

/**
  Return a borrowed reference to singleton class `classname` in the dlite
  module.  The class is created if it doesn't already exists.

  Returns NULL on error.
*/
PyObject *dlite_python_module_class(const char *classname);

/**
  Returns a borrowed reference to singleton Python exception object
  for the given DLite error code.

  The singleton object is created the first time this function is called
  with a given `code`.  All following calles with the same `code` will return
  a reference to the same object.

  Returns NULL if `code` is out of range.
*/
PyObject *dlite_python_module_error(DLiteErrCode code);



#endif /* _DLITE_PYTHON_SINGLETONS_H */
