#ifndef _DLITE_PYTHON_MAPPING_H
#define _DLITE_PYTHON_MAPPING_H

/**
  @file
  @brief Python mappings
*/

#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#endif


/**
  Returns a pointer to internal Python mapping search paths.
*/
FUPaths *dlite_python_mapping_paths(void);

/**
  Clears Python mapping search path.
*/
void dlite_python_mapping_paths_clear(void);

/**
  Inserts `path` into Python mapping paths before position `n`. If
  `n` is negative, it counts from the end (like Python).

  Returns the index of the newly inserted element or -1 on error.
*/
int dlite_python_mapping_paths_insert(const char *path, int n);

/**
  Appends `path` to Python mapping paths.
  Returns the index of the newly appended element or -1 on error.
*/
int dlite_python_mapping_paths_append(const char *path);

/**
  Removes path number `index` from the Python mapping paths.
  Returns non-zero on error.
*/
int dlite_python_mapping_paths_remove_index(int index);

/**
  Returns a pointer to the current Python mapping plugin search path
  or NULL on error.
*/
const char **dlite_python_mapping_paths_get(void);


/**
  Loads all Python mappings (if needed).

  Returns a borrowed reference to a list of mapping plugins (casted to
  void *) or NULL on error.
*/
void *dlite_python_mapping_load(void);

/**
  Unloads all currently loaded mappings.
*/
void dlite_python_mapping_unload(void);


/**
  Returns pointer to next Python mapping plugin (casted to void *) or
  NULL on error.

  `state` should be a pointer to the global state.

  At the first call to this function, `*iter` should be initialised to zero.
  If there are more APIs, `*iter` will be increased by one.
*/
const void *dlite_python_mapping_next(void *state, int *iter);

/**
  Returns Python mapping plugin (casted to void *) with given name or
  NULL if no matches can be found.
 */
const void *dlite_python_mapping_get_api(const char *name);


#endif /* _DLITE_PYTHON_MAPPING_H */
