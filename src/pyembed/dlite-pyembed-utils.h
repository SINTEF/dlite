#ifndef _DLITE_PYEMBED_UTILS_H
#define _DLITE_PYEMBED_UTILS_H


/**
  @file
  @brief Utility functions for embedding Python

  This header will not import Python.h, making it safe to import from
  code that also imports config.h.
 */


/**
  Returns non-zero if the given Python module is available.

  A side effect of calling this function is that the module will
  be imported if it is available.

  Use PyImport_GetModule() if you don't want to import the module.
 */
int dlite_pyembed_has_module(const char *module_name);



#endif /* _DLITE_PYEMBED_UTILS_H */
