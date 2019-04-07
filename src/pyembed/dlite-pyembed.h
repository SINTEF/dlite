#ifndef _DLITE_PYEMBED_H
#define _DLITE_PYEMBED_H

/**
  @file
  @brief Shared code between plugins that embed Python

 */

#include <Python.h>

#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#endif
#include "utils/fileutils.h"
#include "utils/plugin.h"

/* Declarations from utils/fileutils.h - this file cannot be included because
   of conflicts with Python.h */
/*
typedef struct _FUPaths FUPaths;
typedef struct _FUIter FUIter;
typedef struct _PluginInfo PluginInfo;
FUIter *fu_startmatch(const char *pattern, const FUPaths *paths);
const char *fu_nextmatch(FUIter *iter);
int fu_endmatch(FUIter *iter);
*/



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
  Reports and restes Python error.

  If an Python error has occured, an error message is appended to `msg`,
  containing the type, value and traceback.

  Returns `eval`.
 */
int dlite_pyembed_err(int eval, const char *msg, ...);

/**
  Like dlite_pyembed_err() but takes a `va_list` as input.
 */
int dlite_pyembed_verr(int eval, const char *msg, va_list ap);


/**
  This function loads all Python modules found in `paths` and returns
  a list of plugin objects.

  A Python plugin is a subclass of `baseclassname` that implements the
  expected functionality.

  Returns NULL on error.
 */
PyObject *dlite_pyembed_load_plugins(FUPaths *paths, const char *baseclassname);


#endif /* _DLITE_PYEMBED_H */
