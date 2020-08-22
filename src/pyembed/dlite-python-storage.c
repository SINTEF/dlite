/*
  A generic mapping that looks up and loads Python storage plugins
 */
#include <Python.h>
#include <assert.h>
#include <stdlib.h>

/* Python pulls in a lot of defines that conflicts with utils/config.h */
#define SKIP_UTILS_CONFIG_H

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-misc.h"
#include "dlite-storage-plugins.h"
#include "dlite-pyembed.h"
#include "dlite-python-storage.h"


/* Python storage paths */
static FUPaths storage_paths;
static int storage_paths_initialised = 0;
static int storage_paths_modified = 0;

/* A cache with all loaded plugins */
static PyObject *loaded_storages = NULL;

/* Prototype for function converting `inst` to a Python object.
   Returns a new reference or NULL on error. */
typedef PyObject *(*InstanceConverter)(DLiteInstance *inst);


/*
  Returns a pointer to Python storage paths
*/
const FUPaths *dlite_python_storage_paths(void)
{
  if (!storage_paths_initialised) {
    if (fu_paths_init(&storage_paths, "DLITE_PYTHON_STORAGE_PLUGIN_DIRS") < 0)
      return dlite_err(1, "cannot initialise DLITE_PYTHON_STORAGE_PLUGIN_DIRS"
                       ), NULL;
    if (fu_paths_append(&storage_paths, DLITE_PYTHON_STORAGE_PLUGIN_DIRS) < 0)
      return dlite_err(1, "error initialising dlite python storage plugin "
                       "dirs"), NULL;
    storage_paths_initialised = 1;
    storage_paths_modified = 0;
  }
  return &storage_paths;
}

/*
  Clears Python storage search path.
*/
void dlite_python_storage_paths_clear(void)
{
  if (storage_paths_initialised) {
    fu_paths_deinit(&storage_paths);
    storage_paths_initialised = 0;
    storage_paths_modified = 0;
  }
}

/*
  Inserts `path` into Python storage paths before position `n`. If
  `n` is negative, it counts from the end (like Python).

  Returns the index of the newly inserted element or -1 on error.
*/
int dlite_python_storage_paths_insert(const char *path, int n)
{
  int stat;
  const FUPaths *paths;
  if (!(paths = dlite_python_storage_paths())) return -1;
  if ((stat = fu_paths_insert((FUPaths *)paths, path, n)))
    storage_paths_modified = 1;
  return stat;
}

/*
  Appends `path` to Python storage paths.
  Returns the index of the newly appended element or -1 on error.
*/
int dlite_python_storage_paths_append(const char *path)
{
  int stat;
  const FUPaths *paths;
  if (!(paths = dlite_python_storage_paths())) return -1;
  if ((stat = fu_paths_append((FUPaths *)paths, path)))
    storage_paths_modified = 1;
  return stat;
}

/*
  Removes path index `n` to Python storage paths.
  Returns non-zero on error.
*/
int dlite_python_storage_paths_remove(int n)
{
  int stat;
  const FUPaths *paths;
  if (!(paths = dlite_python_storage_paths())) return -1;
  if ((stat = fu_paths_remove((FUPaths *)paths, n)))
    storage_paths_modified = 1;
  return stat;
}

/*
  Returns a pointer to the current Python storage plugin search path
  or NULL on error.
*/
const char **dlite_python_storage_paths_get(void)
{
  const FUPaths *paths;
  if (!(paths = dlite_python_storage_paths())) return NULL;
  return fu_paths_get((FUPaths *)paths);
}


/*
  Loads all Python storages (if needed).

  Returns a borrowed reference to a list of storage plugins (casted to
  void *) or NULL on error.
*/
void *dlite_python_storage_load(void)
{
  if (!loaded_storages || storage_paths_modified) {
    const FUPaths *paths;
    if (loaded_storages) dlite_python_storage_unload();
    if (!(paths = dlite_python_storage_paths())) return NULL;
    loaded_storages = dlite_pyembed_load_plugins((FUPaths *)paths,
                                                 "DLiteStorageBase");
  }
  return (void *)loaded_storages;
}

/* Unloads all currently loaded storages. */
void dlite_python_storage_unload(void)
{
  if (loaded_storages)
    Py_DECREF(loaded_storages);
}
