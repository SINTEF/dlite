#ifndef _DLITE_PYTHON_STORAGE_H
#define _DLITE_PYTHON_STORAGE_H

/**
  @file
  @brief Python storages
*/
#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#endif
//#include "utils/fileutils.h"


/**
  Returns a pointer to internal Python storage search paths.
*/
FUPaths *dlite_python_storage_paths(void);

/**
  Clears Python storage search path.
*/
void dlite_python_storage_paths_clear(void);

/**
  Inserts `path` into Python storage paths before position `n`. If
  `n` is negative, it counts from the end (like Python).

  Returns the index of the newly inserted element or -1 on error.
*/
int dlite_python_storage_paths_insert(const char *path, int n);

/**
  Appends `path` to Python storage paths.
  Returns the index of the newly appended element or -1 on error.
*/
int dlite_python_storage_paths_append(const char *path);

/**
  Removes path number `index` from Python storage paths.
  Returns non-zero on error.
*/
int dlite_python_storage_paths_remove_index(int index);

/**
  Returns a pointer to the current Python storage plugin search path
  or NULL on error.
*/
const char **dlite_python_storage_paths_get(void);


/**
  Loads all Python storages (if needed).

  Returns a borrowed reference to a list of storage plugins (casted to
  void *) or NULL on error.
*/
void *dlite_python_storage_load(void);

/**
  Unloads all currently loaded storages.
*/
void dlite_python_storage_unload(void);


/**
  Returns the base class for storage plugins.
*/
PyObject *dlite_python_storage_base(void);


#endif /* _DLITE_PYTHON_STORAGE_H */
