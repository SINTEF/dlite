/*
  A generic mapping that looks up and loads Python storage plugins
 */
#include <Python.h>
#include <assert.h>
#include <stdlib.h>

/* Python pulls in a lot of defines that conflicts with utils/config.h */
#define SKIP_UTILS_CONFIG_H

#include "config-paths.h"

#include "utils/strutils.h"
#include "pathshash.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-misc.h"
#include "dlite-storage-plugins.h"
#include "dlite-pyembed.h"
#include "dlite-python-singletons.h"
#include "dlite-python-storage.h"

#define GLOBALS_ID "dlite-python-storage-globals"


/* Prototype for function converting `inst` to a Python object.
   Returns a new reference or NULL on error. */
typedef PyObject *(*InstanceConverter)(DLiteInstance *inst);

/* Global state for this module */
typedef struct {
  FUPaths paths;                 /* Python storage paths */
  int initialised;               /* Whether `paths` is initiated */
  unsigned char paths_hash[32];  /* Sha3 hash of plugin paths */
  PyObject *loaded_storages;     /* Cache with all loaded python storage plugins */
  char **failed_paths;           /* NULL-terminated array of paths to storages
                                    that fail to load. */
  size_t failed_len;             /* Allocated length of `failed_paths`. */
} PythonStorageGlobals;


/* Free global state for this module */
static void free_globals(void *globals)
{
  PythonStorageGlobals *g = (PythonStorageGlobals *)globals;
  if (g->initialised) fu_paths_deinit(&g->paths);

  /* Do not call Py_DECREF if we are in an atexit handler */
  if (!dlite_globals_in_atexit()) {
    Py_XDECREF(g->loaded_storages);
    g->loaded_storages = NULL;
  }

  if (g->failed_paths) strlst_free(g->failed_paths);
  g->failed_paths = NULL;
  g->failed_len = 0;
  free(g);
}

/* Return a pointer to global state for this module */
static PythonStorageGlobals *get_globals(void)
{
  PythonStorageGlobals *g = dlite_globals_get_state(GLOBALS_ID);
  if (!g) {
    if (!(g = calloc(1, sizeof(PythonStorageGlobals))))
      return dlite_err(dliteMemoryError, "allocation failure"), NULL;
    dlite_globals_add_state(GLOBALS_ID, g, free_globals);
  }
  return g;
}


/*
  Returns a static pointer to a NULL-terminated list of pointers to
  storages that failed to load.
*/
const char **dlite_python_storage_failed_paths()
{
  PythonStorageGlobals *g = get_globals();
  return (const char **)g->failed_paths;
}


/*
  Returns a pointer to Python storage paths
*/
FUPaths *dlite_python_storage_paths(void)
{
  PythonStorageGlobals *g = get_globals();
  if (!g->initialised) {
    int s;
    if (fu_paths_init(&g->paths, "DLITE_PYTHON_STORAGE_PLUGIN_DIRS") < 0)
      return dlite_err(1, "cannot initialise "
                       "DLITE_PYTHON_STORAGE_PLUGIN_DIRS"), NULL;

    fu_paths_set_platform(&g->paths, dlite_get_platform());

    if (dlite_use_build_root())
      s = fu_paths_extend(&g->paths, dlite_PYTHON_STORAGE_PLUGINS, NULL);
    else
      s = fu_paths_extend_prefix(&g->paths, dlite_pkg_root_get(),
                                 DLITE_PYTHON_STORAGE_PLUGIN_DIRS, NULL);
    if (s < 0) return dlite_err(1, "error initialising dlite python storage "
                                "plugin dirs"), NULL;
    g->initialised = 1;

    /* Make sure that dlite DLLs are added to the library search path */
    dlite_add_dll_path();
  }
  return &g->paths;
}

/*
  Clears Python storage search path.
*/
void dlite_python_storage_paths_clear(void)
{
  PythonStorageGlobals *g = get_globals();
  if (g->initialised) {
    fu_paths_deinit(&g->paths);
    g->initialised = 0;
  }
}

/*
  Inserts `path` into Python storage paths before position `n`. If
  `n` is negative, it counts from the end (like Python).

  Returns the index of the newly inserted element or -1 on error.
*/
int dlite_python_storage_paths_insert(const char *path, int n)
{
  const FUPaths *paths;
  if (!(paths = dlite_python_storage_paths())) return -1;
  return fu_paths_insert((FUPaths *)paths, path, n);
}

/*
  Appends `path` to Python storage paths.
  Returns the index of the newly appended element or -1 on error.
*/
int dlite_python_storage_paths_append(const char *path)
{
  const FUPaths *paths;
  if (!(paths = dlite_python_storage_paths())) return -1;
  return fu_paths_append((FUPaths *)paths, path);
}

/*
  Removes path number `index` from Python storage paths.
  Returns non-zero on error.
*/
int dlite_python_storage_paths_remove_index(int index)
{
  const FUPaths *paths;
  if (!(paths = dlite_python_storage_paths())) return -1;
  return fu_paths_remove_index((FUPaths *)paths, index);
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
  PyObject *storagebase;
  unsigned char hash[32];
  const FUPaths *paths;
  PythonStorageGlobals *g = get_globals();

  if (!(storagebase = dlite_python_storage_base())) return NULL;
  if (!(paths = dlite_python_storage_paths())) return NULL;
  if (pathshash(hash, sizeof(hash), paths, "*.py")) return NULL;

  if (!g->loaded_storages || memcmp(g->paths_hash, hash, sizeof(hash)) != 0) {
    memcpy(g->paths_hash, hash, sizeof(hash));
    if (g->loaded_storages) dlite_python_storage_unload();
    g->loaded_storages = dlite_pyembed_load_plugins((FUPaths *)paths,
                                                    storagebase,
                                                    &g->failed_paths,
                                                    &g->failed_len);
  }
  return (void *)g->loaded_storages;
}

/* Unloads all currently loaded storages. */
void dlite_python_storage_unload(void)
{
  PythonStorageGlobals *g = get_globals();
  if (g->loaded_storages) {
    Py_DECREF(g->loaded_storages);
    g->loaded_storages = NULL;
  }
}
