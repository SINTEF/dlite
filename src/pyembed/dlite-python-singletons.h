#ifndef _DLITE_PYTHON_SINGLETONS_H
#define _DLITE_PYTHON_SINGLETONS_H

/**
  @file
  @brief Python singletons
*/
#include <Python.h>


/**
  Returns a pointer to __main__.__dict__ of the embedded interpreater
  or NULL on error.
*/
PyObject *dlite_python_maindict(void);

/**
  Creates a new class in __main__ and returns it.  If the class
  already exists, it is returned immediately.

  The class is created as executing the following python code in the
  __main__ scope `class <classname>: pass`.

  Returns NULL on error.
 */
PyObject *dlite_python_mainclass(const char *classname);


/**
  Returns the base class for storage plugins.
*/
PyObject *dlite_python_storage_base(void);

/**
  Returns the base class for mapping plugins.
*/
PyObject *dlite_python_mapping_base(void);


#endif /* _DLITE_PYTHON_SINGLETONS_H */
