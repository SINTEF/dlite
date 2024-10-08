#ifndef _DLITE_PYEMBED_H
#define _DLITE_PYEMBED_H

/**
  @file
  @brief Shared code between plugins that embed Python

 */

#include <Python.h>

/* Python pulls in a lot of defines that conflicts with utils/config.h */
#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#endif

#include "utils/fileutils.h"
#include "utils/plugin.h"
#include "dlite.h"

/* Remove __attribute__ when we are not compiling with gcc */
#ifndef __GNUC__
# define __attribute__(x)
#endif


/**
  Initialises the embedded Python environment.
*/
void dlite_pyembed_initialise(void);

/**
  Finalises the embedded Python environment.
*/
int dlite_pyembed_finalise(void);

/**
  Returns a static pointer to the class name of python object cls or
  NULL on error.
*/
const char *dlite_pyembed_classname(PyObject *cls);

/**
  Return DLite error code given Python exception type.
 */
DLiteErrCode dlite_pyembed_errcode(PyObject *type);

/**
  Return Python exception class corresponding to given DLite error code.
  Returns NULL if `code` is zero.
 */
PyObject *dlite_pyembed_exception(DLiteErrCode code);

/**
  Writes Python error message to `errmsg` (of length `len`) if an
  Python error has occured.

  On return the The Python error indicator is reset.

  Returns 0 if no error has occured, otherwise return the number of
  bytes written to `errmsg`. On error -1 is returned.
*/
int dlite_pyembed_errmsg(char *errmsg, size_t errlen);


/**
  Reports and restes Python error.

  If an Python error has occured, an error message is appended to `msg`,
  containing the type, value and traceback.

  Returns `eval`.
 */
int dlite_pyembed_err(int eval, const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));

/**
  Like dlite_pyembed_err() but takes a `va_list` as input.
 */
int dlite_pyembed_verr(int eval, const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 2, 0)));

/**
  Checks if an Python error has occured.  Returns zero if no error has
  occured.  Otherwise dlite_pyembed_err() is called and non-zero is
  returned.
 */
int dlite_pyembed_err_check(const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));

/**
  Like dlite_pyembed_err_check() but takes a `va_list` as input.
 */
int dlite_pyembed_verr_check(const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 1, 0)));


/**
  Loads the Python C extension module "_dlite" and returns the address
  of `symbol`, within this module.  Returns NULL on error or if
  `symbol` cannot be found.
*/
void *dlite_pyembed_get_address(const char *symbol);


/**
  Returns a Python representation of dlite instance with given id or NULL
  on error.
*/
PyObject *dlite_pyembed_from_instance(const char *id);

/**
  Returns a new reference to DLite instance from Python representation
  or NULL on error.
*/
DLiteInstance *dlite_pyembed_get_instance(PyObject *pyinst);


/**
  This function loads all Python modules found in `paths` and returns
  a list of plugin objects.

  A Python plugin is a subclass of `baseclassname` that implements the
  expected functionality.

  If `failed_paths` is given, it should be a pointer to a
  NULL-terminated array of pointers to paths to plugins that failed to
  load.  In case a plugin fails to load, this array will be updated.

  If `failed_paths` is given, `failed_len` must also be given. It
  should be a pointer to the allocated length of `*failed_paths`.

  Returns NULL on error.
 */
PyObject *dlite_pyembed_load_plugins(FUPaths *paths, PyObject *baseclass,
                                     char ***failed_paths, size_t *failed_len);


/**
  Return borrowed reference to the `__dict__` object in the dlite
  module or NULL on error.
 */
PyObject *dlite_python_dlitedict(void);


/**
  Return borrowed reference to a dict serving as a namespace for the
  given plugin.

  The returned dict is accessable from Python as
  `dlite._plugindict[plugin_name]`.  The dict will be created if it
  doesn't already exists.

  Returns NULL on error.
 */
PyObject *dlite_python_plugindict(const char *plugin_name);


#endif /* _DLITE_PYEMBED_H */
