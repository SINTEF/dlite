#ifndef _DLITE_PYTHON_PATH_H
#define _DLITE_PYTHON_PATH_H

/**
  @file
  @brief Functions for generic DLite paths objects
*/


/**
  Returns the newly allocated string with the Python site prefix or
  NULL on error.

  This correspond to returning `site.PREFIXES[0]` from Python.
 */
char *dlite_python_site_prefix(void);


#endif /* _DLITE_PYTHON_PATH_H */
