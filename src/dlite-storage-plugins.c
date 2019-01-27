#include "config.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "utils/err.h"
#include "utils/tgen.h"
#include "utils/plugin.h"

#include "dlite-datamodel.h"
#include "dlite-storage-plugins.h"


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
  If a plugin with the provided name is fount, it is loaded,
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
    TGenBuf buf;
    int n=0;
    const char *p, **paths = dlite_storage_plugin_paths();
    tgen_buf_init(&buf);
    tgen_buf_append_fmt(&buf, "cannot find storage plugin for driver \"%s\" "
                        "in search path:\n", name);
    while ((p = *(paths++)) && ++n) tgen_buf_append_fmt(&buf, "    %s\n", p);
    tgen_buf_append_fmt(&buf, "\n");
    if (n <= 1)
      tgen_buf_append_fmt(&buf, "Is the DLITE_STORAGE_PLUGINS enveronment "
                          "variable set?");
    err(1, tgen_buf_get(&buf));
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
  return plugin_register(info, api);
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
