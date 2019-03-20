#include "config.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "utils/err.h"
#include "utils/tgen.h"
#include "utils/plugin.h"

#include "dlite-datamodel.h"
#include "dlite-mapping-plugins.h"


/* Global reference to mapping plugin info */
static PluginInfo *mapping_plugin_info=NULL;


/* Frees up `mapping_plugin_info`. */
static void mapping_plugin_info_free(void)
{
  if (mapping_plugin_info) plugin_info_free(mapping_plugin_info);
  mapping_plugin_info = NULL;
}

/* Returns a pointer to `mapping_plugin_info`. */
static PluginInfo *get_mapping_plugin_info(void)
{
  if (!mapping_plugin_info &&
      (mapping_plugin_info =
       plugin_info_create("mapping-plugin",
                          "get_dlite_mapping_api",
                          "DLITE_MAPPING_PLUGINS"))) {
    atexit(mapping_plugin_info_free);
    dlite_mapping_plugin_path_append(DLITE_MAPPING_PLUGINS_PATH);
  }
  return mapping_plugin_info;
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
  const DLiteMappingPlugin *api;
  PluginInfo *info;

  if (!(info = get_mapping_plugin_info())) return NULL;

  if (!(api = (const DLiteMappingPlugin *)plugin_get_api(info, name))) {
    TGenBuf buf;
    int n=0;
    const char *p, **paths = dlite_mapping_plugin_paths();
    tgen_buf_init(&buf);
    tgen_buf_append_fmt(&buf, "cannot find mapping plugin for driver \"%s\" "
                        "in search path:\n", name);
    while ((p = *(paths++)) && ++n) tgen_buf_append_fmt(&buf, "    %s\n", p);
    if (n <= 1)
      tgen_buf_append_fmt(&buf, "Is the DLITE_MAPPING_PLUGINS enveronment "
                          "variable set?");
    errx(1, tgen_buf_get(&buf));
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
  return plugin_register(info, api);
}

/*
  Initiates a mapping plugin iterator.
*/
void dlite_mapping_plugin_init_iter(DLiteMappingPluginIter *iter)
{
  plugin_init_iter((PluginIter *)iter, get_mapping_plugin_info());
}

/*
  Returns the next registered mapping plugin or NULL if all plugins
  has been visited.

  Used for iterating over plugins.  Plugins should not be registered
  or removed while iterating.
 */
DLiteMappingPlugin *dlite_mapping_plugin_next(DLiteMappingPluginIter *iter)
{
  return (DLiteMappingPlugin *)plugin_next((PluginIter *)iter);
}




/*
  Unloads and unregisters mapping plugin with the given name.
  Returns non-zero on error.
*/
int dlite_mapping_plugin_unload(const char *name)
{
  PluginInfo *info;
  if (!(info = get_mapping_plugin_info())) return 1;
  return plugin_unload(info, name);
}

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
  Removes path number `n` from current search path.

  Returns non-zero on error.
*/
int dlite_mapping_plugin_path_remove(int n)
{
  PluginInfo *info;
  if (!(info = get_mapping_plugin_info())) return 1;
  return plugin_path_remove(info, n);
}
