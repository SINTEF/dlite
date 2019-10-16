#include "config.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "utils/err.h"
#include "utils/tgen.h"
#include "utils/plugin.h"

#include "dlite-misc.h"
#include "dlite-datamodel.h"
#include "dlite-storage-plugins.h"


struct _DLiteStoragePluginIter {
  PluginIter iter;
};

/* Global reference to storage plugin info */
static PluginInfo *storage_plugin_info=NULL;


/* Frees up `storage_plugin_info`. */
static void storage_plugin_info_free(void)
{
  if (storage_plugin_info) plugin_info_free(storage_plugin_info);
  storage_plugin_info = NULL;
}

/* Returns a pointer to `storage_plugin_info`. */
static PluginInfo *get_storage_plugin_info(void)
{
  if (!storage_plugin_info &&
      (storage_plugin_info = plugin_info_create("storage-plugin",
                                                "get_dlite_storage_plugin_api",
                                                "DLITE_STORAGE_PLUGINS"))) {
    atexit(storage_plugin_info_free);
    dlite_storage_plugin_path_append(DLITE_STORAGE_PLUGINS_PATH);
  }
  return storage_plugin_info;
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

  if (!(info = get_storage_plugin_info())) return NULL;

  if (!(api = (const DLiteStoragePlugin *)plugin_get_api(info, name))) {
    /* create informative error message... */
    TGenBuf buf;
    int n=0;
    const char *p, **paths = dlite_storage_plugin_paths();
    tgen_buf_init(&buf);
    tgen_buf_append_fmt(&buf, "cannot find storage plugin for driver \"%s\" "
                        "in search path:\n", name);
    while ((p = *(paths++)) && ++n) tgen_buf_append_fmt(&buf, "    %s\n", p);
    if (n <= 1)
      tgen_buf_append_fmt(&buf, "Is the DLITE_STORAGE_PLUGINS enveronment "
                          "variable set?");
    errx(1, tgen_buf_get(&buf));
    tgen_buf_deinit(&buf);
  }
  return api;
}

/*
  Registers `api` for a storage plugin.  Returns non-zero on error.
*/
int dlite_storage_plugin_register_api(const DLiteStoragePlugin *api)
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return 1;
  return plugin_register_api(info, api);
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
  DLiteStoragePluginIter *iter;
  const DLiteStoragePlugin *api;
  //char **names=NULL;
  //int i, n=0, N=0;
  if (!(info = get_storage_plugin_info())) return;
  if (!(iter = dlite_storage_plugin_iter_create())) return;

  while ((api = dlite_storage_plugin_iter_next(iter))) {
    plugin_unload(info, api->name);
    if (api->freer) api->freer((DLiteStoragePlugin *)api);
    //free((DLiteStoragePlugin *)api);
  }

  /* make a list with all plugin names */
    /*
  while ((api = dlite_storage_plugin_iter_next(iter))) {
    if (n >= N) {
      N += 64;
      names = realloc(names, N*sizeof(char *));
    }
    names[n++] = strdup(api->name);
  }
  dlite_storage_plugin_iter_free(iter);

  for (i=0; i<n; i++) {
    plugin_unload(info, names[i]);
    free(names[i]);
  }
  if (names) free(names);
    */
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
    return err(1, "allocation failure"), NULL;
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
  Returns a NULL-terminated array of pointers to search paths or NULL
  if no search path is defined.

  Use dlite_storage_plugin_path_insert(), dlite_storage_plugin_path_append()
  and dlite_storage_plugin_path_remove() to modify it.
*/
const char **dlite_storage_plugin_paths()
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return NULL;
  return plugin_path_get(info);
}

/*
  Inserts `path` into the current search path at index `n`.  If `n` is
  negative, it counts from the end of the search path (like Python).

  If `n` is out of range, it is clipped.

  Returns non-zero on error.
*/
int dlite_storage_plugin_path_insert(int n, const char *path)
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return 1;
  return plugin_path_insert(info, path, n);
}

/*
  Appends `path` into the current search path.

  Returns non-zero on error.
*/
int dlite_storage_plugin_path_append(const char *path)
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return 1;
  return plugin_path_append(info, path);
}

/*
  Removes path number `n` from current search path.

  Returns non-zero on error.
*/
int dlite_storage_plugin_path_remove(int n)
{
  PluginInfo *info;
  if (!(info = get_storage_plugin_info())) return 1;
  return plugin_path_remove(info, n);
}
