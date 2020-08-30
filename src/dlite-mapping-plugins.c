#include "config.h"
#include "config-paths.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "utils/err.h"
#include "utils/dsl.h"
#include "utils/fileutils.h"
#include "utils/plugin.h"
#include "utils/tgen.h"

#include "dlite-datamodel.h"
#include "dlite-mapping-plugins.h"


/* Global reference to mapping plugin info */
static PluginInfo *mapping_plugin_info = NULL;


/* Frees up `mapping_plugin_info`. */
static void mapping_plugin_info_free(void)
{
  if (mapping_plugin_info) {
    PluginIter iter;
    plugin_api_iter_init(&iter, mapping_plugin_info);
    plugin_info_free(mapping_plugin_info);
  }
  mapping_plugin_info = NULL;
}


/* Returns a pointer to `mapping_plugin_info`. */
static PluginInfo *get_mapping_plugin_info(void)
{
  if (!mapping_plugin_info &&
      (mapping_plugin_info =
       plugin_info_create("mapping-plugin",
                          "get_dlite_mapping_api",
                          "DLITE_MAPPING_PLUGIN_DIRS"))) {
    atexit(mapping_plugin_info_free);

    if (dlite_use_build_root())
      plugin_path_extend(mapping_plugin_info, dlite_MAPPING_PLUGINS, NULL);
    else
      plugin_path_extend_prefix(mapping_plugin_info, dlite_root_get(),
                                DLITE_MAPPING_PLUGIN_DIRS, NULL);

    /* Make sure that dlite DLLs are added to the library search path */
    dlite_add_dll_path();
  }
  return mapping_plugin_info;
}

/* Loads all plugins (if we haven't done that before) */
static void load_mapping_plugins(void)
{
  static int mapping_plugins_loaded = 0;
  PluginInfo *info;
  if (!mapping_plugins_loaded && (info = get_mapping_plugin_info())) {
    plugin_load_all(info);
    mapping_plugins_loaded = 1;
  }
}


/*
  Returns a mapping plugin with the given name, or NULL if it cannot
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
const DLiteMappingPlugin *dlite_mapping_plugin_get(const char *name)
{
  DLiteMappingPlugin *api;
  PluginInfo *info;

  if (!(info = get_mapping_plugin_info())) return NULL;

  if (!(api = (DLiteMappingPlugin *)plugin_get_api(info, name))) {
    TGenBuf buf;
    int n=0;
    const char *p, **paths = dlite_mapping_plugin_paths();
    tgen_buf_init(&buf);
    tgen_buf_append_fmt(&buf, "cannot find mapping plugin for driver \"%s\" "
                        "in search path:\n", name);
    while ((p = *(paths++)) && ++n) tgen_buf_append_fmt(&buf, "    %s\n", p);
    if (n <= 1)
      tgen_buf_append_fmt(&buf, "Is the DLITE_MAPPING_PLUGIN_DIRS enveronment "
                          "variable set?");
    errx(1, "%s", tgen_buf_get(&buf));
    tgen_buf_deinit(&buf);
  }
  return api;
}

/*
  Registers `api` for a mapping plugin.  Returns non-zero on error.
*/
int dlite_mapping_plugin_register_api(const DLiteMappingPlugin *api)
{
  PluginInfo *info;
  if (!(info = get_mapping_plugin_info())) return 1;
  return plugin_register_api(info, (PluginAPI *)api);
}

/*
  Initiates a mapping plugin iterator.  Returns non-zero on error.
*/
int dlite_mapping_plugin_init_iter(DLiteMappingPluginIter *iter)
{
  PluginInfo *info;
  load_mapping_plugins();
  if (!(info = get_mapping_plugin_info())) return 1;
  plugin_api_iter_init((PluginIter *)iter, info);
  return 0;
}

/*
  Returns the next registered mapping plugin or NULL if all plugins
  has been visited.

  Used for iterating over plugins.  Plugins should not be registered
  or removed while iterating.
 */
const DLiteMappingPlugin *
dlite_mapping_plugin_next(DLiteMappingPluginIter *iter)
{
  return (const DLiteMappingPlugin *)plugin_api_iter_next((PluginIter *)iter);
}



/*
  Unloads and unregisters mapping plugin with the given name.
  Returns non-zero on error.
*/
int dlite_mapping_plugin_unload(const char *name)
{
  int stat;
  PluginInfo *info;
  if (!(info = get_mapping_plugin_info())) return 1;
  stat = plugin_unload(info, name);
  return stat;
}

/*
  Unloads and unregisters all mappings.  Returns non-zero on error.
*/
int dlite_mapping_plugin_unload_all(void)
{
  mapping_plugin_info_free();
  return 0;
}
/*
  // xxx
  PluginInfo *info;
  PluginIter iter;
  DLiteMappingPlugin *api;
  if (!(info = get_mapping_plugin_info())) return 1;
  plugin_api_iter_init(&iter, info);
  while ((api = (DLiteMappingPlugin *)plugin_api_iter_next(&iter))) {
    //plugin_unload(info, api->name);
    if (api && api->freer) api->freer(api);
  }
  plugin_info_free(info);

  //plugin_deinit_iter(&iter);
  return 0;
}
*/

/*
  Returns a NULL-terminated array of pointers to search paths or NULL
  if no search path is defined.

  Use dlite_mapping_plugin_path_insert(), dlite_mapping_plugin_path_append()
  and dlite_mapping_plugin_path_remove() to modify it.
*/
const char **dlite_mapping_plugin_paths()
{
  PluginInfo *info;
  if (!(info = get_mapping_plugin_info())) return NULL;
  return plugin_path_get(info);
}

/*
  Inserts `path` into the current search path at index `n`.  If `n` is
  negative, it counts from the end of the search path (like Python).

  If `n` is out of range, it is clipped.

  Returns non-zero on error.
*/
int dlite_mapping_plugin_path_insert(int n, const char *path)
{
  PluginInfo *info;
  if (!(info = get_mapping_plugin_info())) return 1;
  return plugin_path_insert(info, path, n);
}

/*
  Appends `path` into the current search path.

  Returns non-zero on error.
*/
int dlite_mapping_plugin_path_append(const char *path)
{
  PluginInfo *info;
  if (!(info = get_mapping_plugin_info())) return 1;
  return plugin_path_append(info, path);
}

/*
  Like dlite_mapping_plugin_path_append(), but appends at most the
  first `n` bytes of `path` to the current search path.

  Returns non-zero on error.
*/
int dlite_mapping_plugin_path_appendn(const char *path, size_t n)
{
  PluginInfo *info;
  if (!(info = get_mapping_plugin_info())) return 1;
  return plugin_path_appendn(info, path, n);
}

/*
  Removes path number `n` from current search path.

  Returns non-zero on error.
*/
int dlite_mapping_plugin_path_remove(int n)
{
  PluginInfo *info;
  if (!(info = get_mapping_plugin_info())) return 1;
  return plugin_path_remove(info, n);
}
