#include "config.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "utils/err.h"
#include "utils/tgen.h"
#include "utils/plugin.h"

#include "dlite-datamodel.h"
#include "dlite-translator-plugins.h"


/* Global reference to translator plugin info */
static PluginInfo *translator_plugin_info=NULL;


/* Frees up `translator_plugin_info`. */
static void translator_plugin_info_free(void)
{
  if (translator_plugin_info) plugin_info_free(translator_plugin_info);
  translator_plugin_info = NULL;
}

/* Returns a pointer to `translator_plugin_info`. */
static PluginInfo *get_translator_plugin_info(void)
{
  if (!translator_plugin_info &&
      (translator_plugin_info =
       plugin_info_create("translator-plugin",
                          "get_dlite_translator_plugin_api",
                          "DLITE_TRANSLATOR_PLUGINS"))) {
    atexit(translator_plugin_info_free);
    dlite_translator_plugin_path_append(DLITE_TRANSLATOR_PLUGINS_PATH);
  }
  return translator_plugin_info;
}


/*
  Returns a translator plugin with the given name, or NULL if it cannot
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
const DLiteTranslatorPlugin *dlite_translator_plugin_get(const char *name)
{
  const DLiteTranslatorPlugin *api;
  PluginInfo *info;

  if (!(info = get_translator_plugin_info())) return NULL;

  if (!(api = (const DLiteTranslatorPlugin *)plugin_get_api(info, name))) {
    TGenBuf buf;
    int n=0;
    const char *p, **paths = dlite_translator_plugin_paths();
    tgen_buf_init(&buf);
    tgen_buf_append_fmt(&buf, "cannot find translator plugin for driver \"%s\" "
                        "in search path:\n", name);
    while ((p = *(paths++)) && ++n) tgen_buf_append_fmt(&buf, "    %s\n", p);
    if (n <= 1)
      tgen_buf_append_fmt(&buf, "Is the DLITE_TRANSLATOR_PLUGINS enveronment "
                          "variable set?");
    errx(1, tgen_buf_get(&buf));
    tgen_buf_deinit(&buf);
  }
  return api;
}

/*
  Registers `api` for a translator plugin.  Returns non-zero on error.
*/
int dlite_translator_plugin_register_api(const DLiteTranslatorPlugin *api)
{
  PluginInfo *info;
  if (!(info = get_translator_plugin_info())) return 1;
  return plugin_register(info, api);
}

/*
  Unloads and unregisters translator plugin with the given name.
  Returns non-zero on error.
*/
int dlite_translator_plugin_unload(const char *name)
{
  PluginInfo *info;
  if (!(info = get_translator_plugin_info())) return 1;
  return plugin_unload(info, name);
}

/*
  Returns a NULL-terminated array of pointers to search paths or NULL
  if no search path is defined.

  Use dlite_translator_plugin_path_insert(), dlite_translator_plugin_path_append()
  and dlite_translator_plugin_path_remove() to modify it.
*/
const char **dlite_translator_plugin_paths()
{
  PluginInfo *info;
  if (!(info = get_translator_plugin_info())) return NULL;
  return plugin_path_get(info);
}

/*
  Inserts `path` into the current search path at index `n`.  If `n` is
  negative, it counts from the end of the search path (like Python).

  If `n` is out of range, it is clipped.

  Returns non-zero on error.
*/
int dlite_translator_plugin_path_insert(int n, const char *path)
{
  PluginInfo *info;
  if (!(info = get_translator_plugin_info())) return 1;
  return plugin_path_insert(info, path, n);
}

/*
  Appends `path` into the current search path.

  Returns non-zero on error.
*/
int dlite_translator_plugin_path_append(const char *path)
{
  PluginInfo *info;
  if (!(info = get_translator_plugin_info())) return 1;
  return plugin_path_append(info, path);
}

/*
  Removes path number `n` from current search path.

  Returns non-zero on error.
*/
int dlite_translator_plugin_path_remove(int n)
{
  PluginInfo *info;
  if (!(info = get_translator_plugin_info())) return 1;
  return plugin_path_remove(info, n);
}
