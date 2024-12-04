#include "config.h"
#include "config-paths.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "utils/compat.h"
#include "utils/err.h"
#include "utils/tgen.h"
#include "utils/plugin.h"

#include "pathshash.h"
#include "dlite-misc.h"
#include "dlite-datamodel.h"
#include "dlite-storage-plugins.h"

#ifdef WITH_PYTHON
#define NOPYTHON
#include "pyembed/dlite-python-storage.h"
#endif

#define GLOBALS_ID "dlite-storage-plugins-id"


struct _DLiteStoragePluginIter {
  PluginIter iter;
};

/* Global variables for dlite-storage-plugins */
typedef struct {
  PluginInfo *storage_plugin_info;  /* reference to storage plugin info */
  unsigned char storage_plugin_path_hash[32];  /* Sha3 hash of plugin paths */
} Globals;


/* Frees global state for this module - called by atexit() */
static void free_globals(void *globals)
{
  Globals *g = globals;
  if (g->storage_plugin_info) plugin_info_free(g->storage_plugin_info);
  free(g);
}

/* Return a pointer to global state for this module */
static Globals *get_globals(void)
{
  Globals *g = dlite_globals_get_state(GLOBALS_ID);
  if (!g) {
    if (!(g = calloc(1, sizeof(Globals))))
      return err(dliteMemoryError, "allocation failure"), NULL;
    dlite_globals_add_state(GLOBALS_ID, g, free_globals);
  }
  return g;
}

/* Returns a pointer to `storage_plugin_info`. */
static PluginInfo *get_storage_plugin_info(void)
{
  Globals *g;
  if (!(g = get_globals())) return NULL;

  if (!g->storage_plugin_info &&
      (g->storage_plugin_info =
       plugin_info_create("storage-plugin",
			  "get_dlite_storage_plugin_api",
			  "DLITE_STORAGE_PLUGIN_DIRS",
                          dlite_globals_get()))) {
    fu_paths_set_platform(&g->storage_plugin_info->paths, dlite_get_platform());
    if (dlite_use_build_root())
      plugin_path_extend(g->storage_plugin_info, dlite_STORAGE_PLUGINS, NULL);
    else
      plugin_path_extend_prefix(g->storage_plugin_info, dlite_root_get(),
                                DLITE_STORAGE_PLUGIN_DIRS, NULL);

    /* Make sure that dlite DLLs are added to the library search path */
    dlite_add_dll_path();
  }
  return g->storage_plugin_info;
}


/*
  Returns a storage plugin with the given name, or NULL if it cannot
  be found.

  If a plugin with the given name is registered, it is returned.

  Otherwise the plugin search path is checked for shared libraries
  matching `name.EXT` where `EXT` is the extension for shared library
  on the current platform ("dll" on Windows and "so" on Unix/Linux).
  If a plugin with the provided name is found, it is loaded,
  registered and returned.

  Otherwise the plugin search path is checked again, but this time for
  any shared library.  If a plugin with the provided name is found, it
  is loaded, registered and returned.

  Otherwise NULL is returned.
*/
const DLiteStoragePlugin *dlite_storage_plugin_get(const char *name)
{
  const DLiteStoragePlugin *api;
  PluginInfo *info;
  unsigned char hash[32];
  Globals *g;

  if (!(g = get_globals())) return NULL;
  if (!(info = get_storage_plugin_info())) return NULL;

  /* Return plugin if it is loaded */
 ErrTry:  // silence dliteStorageLoadError
  api = (const DLiteStoragePlugin *)plugin_get_api(info, name,
                                                   dliteStorageLoadError);
 ErrCatch(dliteStorageLoadError):
  break;
 ErrEnd;
  if (api) return api;

  /* ...otherwise, if any plugin path has changed, reload all plugins
     and try again */
  if (pathshash(hash, sizeof(hash), &info->paths, DSL_EXT) == 0) {

    if (memcmp(g->storage_plugin_path_hash, hash, sizeof(hash)) != 0) {
      plugin_load_all(info);
      memcpy(g->storage_plugin_path_hash, hash, sizeof(hash));

     ErrTry:  // silence dliteStorageLoadError
      api = (const DLiteStoragePlugin *)plugin_get_api(info, name,
                                                       dliteStorageLoadError);
      ErrCatch(dliteStorageLoadError):
      break;
     ErrEnd;
      if (api) return api;
    }
  }

  /* Cannot find api - create informative error message */
  {
    int n=0, r;
    const char *p, **paths = dlite_storage_plugin_paths();
    size_t size=0, m=0;
    char *buf=NULL;
    r = asnpprintf(&buf, &size, m, "cannot find storage plugin for driver "
                   "\"%s\" in\n   search path:\n", name);
    if (r >= 0) m += r;
    while (paths && (p = *(paths++)) && ++n) {
      r = asnpprintf(&buf, &size, m, "   - %s\n", p);
      if (r >= 0) m += r;
    }

#ifdef WITH_PYTHON
    char *submsg = (dlite_use_build_root()) ? "" : "DLITE_ROOT, ";
    FUPaths *ppaths = dlite_python_storage_paths();
    FUIter *iter = fu_startmatch("*.py", ppaths);
    const char **failed_paths;
    r = asnpprintf(&buf, &size, m,
                   "   The following Python plugins were also checked:\n");
    if (r >= 0) m += r;
    while ((p = fu_nextmatch(iter))) {
      r = asnpprintf(&buf, &size, m, "   - %s\n", p);
      if (r >= 0) m += r;
    }
    fu_endmatch(iter);

    if ((failed_paths = dlite_python_storage_failed_paths())) {
      r = asnpprintf(&buf, &size, m,
                     "   The following Python plugins failed to load:\n");
      if (r >= 0) m += r;
      while ((failed_paths && *failed_paths)) {
        r = asnpprintf(&buf, &size, m, "   - %s\n", *(failed_paths++));
        if (r >= 0) m += r;
      }
      if (!getenv("DLITE_PYDEBUG")) {
        r = asnpprintf(&buf, &size, m,
                       "   To see error messages from Python storages, please "
                       "rerun with the\n"
                       "   DLITE_PYDEBUG environment variable set.\n");
        if (r >= 0) m += r;
      }
    }

    if (n <= 1) {
      m += asnpprintf(&buf, &size, m,
                      "   If the plugin is listed above, but could not be "
                      "loaded, it may be an\n"
                      "   error in the plugin. Are the required Python "
                      "packages installed?\n");
    }
    if (!getenv("DLITE_PYDEBUG")) {
      r = asnpprintf(&buf, &size, m,
                     "   Please rerun with the DLITE_PYDEBUG environment "
                     "variable set.\n");
      if (r >= 0) m += r;
    }
    r = asnpprintf(&buf, &size, m,
                   "   If the plugin is not listed above, it may not be "
                   "in the search path.\n"
                   "   Are the %sDLITE_STORAGE_PLUGIN_DIRS or "
                   "DLITE_PYTHON_STORAGE_PLUGIN_DIRS\n"
                   "   environment variables set?", submsg);
    errx(dliteStorageOpenError, "%s", buf);
#endif
    free(buf);

  }
  return NULL;
}

/*
  Load all plugins that can be found in the plugin search path.
  Returns non-zero on error.
 */
int dlite_storage_plugin_load_all()
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return 1;
  plugin_load_all(info);
  return 0;
}

/*
  Unloads and unregisters all storage plugins.
*/
void dlite_storage_plugin_unload_all()
{
  PluginInfo *info;
  char **p, **names;
  if (!(info = get_storage_plugin_info())) return;
  if (!(names = plugin_names(info))) return;
  for (p=names; *p; p++) {
    plugin_unload(info, *p);
    free(*p);
  }
  free(names);
}

/*
  Returns a pointer to a new plugin iterator or NULL on error.  It
  should be free'ed with dlite_storage_plugin_iter_free().
 */
DLiteStoragePluginIter *dlite_storage_plugin_iter_create()
{
  PluginInfo *info;
  DLiteStoragePluginIter *iter;
  if (!(info = get_storage_plugin_info())) return NULL;
  if (!(iter = calloc(1, sizeof(DLiteStoragePluginIter))))
    return err(dliteMemoryError, "allocation failure"), NULL;
  plugin_api_iter_init(&iter->iter, info);
  return iter;
}

/*
  Returns pointer the next plugin or NULL if there a re no more plugins.
  `iter` is the iterator returned by dlite_storage_plugin_iter_create().
 */
const DLiteStoragePlugin *
dlite_storage_plugin_iter_next(DLiteStoragePluginIter *iter)
{
  return (const DLiteStoragePlugin *)plugin_api_iter_next(&iter->iter);
}

/*
  Frees plugin iterator `iter` created with
  dlite_storage_plugin_iter_create().
 */
void dlite_storage_plugin_iter_free(DLiteStoragePluginIter *iter)
{
  free(iter);
}


/*
  Unloads and unregisters storage plugin with the given name.
  Returns non-zero on error.
*/
int dlite_storage_plugin_unload(const char *name)
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return 1;
  return plugin_unload(info, name);
}


/*
  Returns a pointer to the underlying FUPaths object for storage plugins
  or NULL on error.
 */
FUPaths *dlite_storage_plugin_paths_get(void)
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return NULL;
  return &info->paths;
}

/*
  Returns a NULL-terminated array of pointers to search paths or NULL
  if no search path is defined.

  Use dlite_storage_plugin_path_insert(), dlite_storage_plugin_path_append()
  and dlite_storage_plugin_path_remove() to modify it.
*/
const char **dlite_storage_plugin_paths(void)
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return NULL;
  return plugin_path_get(info);
}

/*
  Returns an allocated string with the content of `paths` formatted
  according to the current platform.  See dlite_set_platform().

  Returns NULL on error.
 */
char *dlite_storage_plugin_path_string(void)
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return NULL;
  return fu_paths_string(&info->paths);
}

/*
  Inserts `path` into the current search path at index `n`.  If `n` is
  negative, it counts from the end of the search path (like Python).

  If `n` is out of range, it is clipped.

  Returns the index of the newly inserted element or -1 on error.
*/
int dlite_storage_plugin_path_insert(int n, const char *path)
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return 1;
  return plugin_path_insert(info, path, n);
}

/*
  Appends `path` into the current search path.

  Returns the index of the newly appended element or -1 on error.
*/
int dlite_storage_plugin_path_append(const char *path)
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return 1;
  return plugin_path_append(info, path);
}

/*
  Like dlite_storage_plugin_path_append(), but appends at most the
  first `n` bytes of `path` to the current search path.

  Returns the index of the newly appended element or -1 on error.
*/
int dlite_storage_plugin_path_appendn(const char *path, size_t n)
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return 1;
  return plugin_path_appendn(info, path, n);
}

/*
  Removes path number `n` from current search path.

  Returns non-zero on error.
*/
int dlite_storage_plugin_path_remove_index(int index)
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return 1;
  return plugin_path_remove_index(info, index);
}

/*
  Removes path `path` from current search path.

  Returns non-zero if there is no such path.
*/
int dlite_storage_plugin_path_remove(const char *path)
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return 1;
  return plugin_path_remove(info, path);
}
