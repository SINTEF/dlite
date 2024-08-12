#include "config.h"
#include "config-paths.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "utils/err.h"
#include "utils/dsl.h"
#include "utils/sha3.h"
#include "utils/compat.h"
#include "utils/fileutils.h"
#include "utils/plugin.h"

#include "dlite-datamodel.h"
#include "dlite-mapping-plugins.h"
#include "dlite-macros.h"
#ifdef WITH_PYTHON
#include "pyembed/dlite-python-mapping.h"
#endif

#define GLOBALS_ID "dlite-mapping-plugins-id"


typedef struct {
  /* Global reference to mapping plugin info */
  PluginInfo *mapping_plugin_info;

  /* Sha256 hash of plugin paths */
  unsigned char mapping_plugin_path_hash[32];
} Globals;


/* Free global state for this module */
static void free_globals(void *globals)
{
  Globals *g = globals;
  if (g->mapping_plugin_info) {
    plugin_info_free(g->mapping_plugin_info);
  }
  free(g);
}

/* Return a pointer to global state for this module */
static Globals *get_globals(void) {
  Globals *g = dlite_globals_get_state(GLOBALS_ID);
  if (!g) {
    if (!(g = calloc(1, sizeof(Globals))))
     FAILCODE(dliteMemoryError, "allocation failure");

    g->mapping_plugin_info = plugin_info_create("mapping-plugin",
                                                "get_dlite_mapping_api",
                                                "DLITE_MAPPING_PLUGIN_DIRS",
                                                dlite_globals_get());
    if (!g->mapping_plugin_info) goto fail;
    fu_paths_set_platform(&g->mapping_plugin_info->paths, dlite_get_platform());
    if (dlite_use_build_root())
      plugin_path_extend(g->mapping_plugin_info, dlite_MAPPING_PLUGINS, NULL);
    plugin_path_extend_prefix(g->mapping_plugin_info, dlite_root_get(),
                              DLITE_ROOT "/" DLITE_MAPPING_PLUGIN_DIRS, NULL);

    /* Make sure that dlite DLLs are added to the library search path */
    dlite_add_dll_path();

    dlite_globals_add_state(GLOBALS_ID, g, free_globals);
  }
  return g;
 fail:
  if (g) free(g);
  return NULL;
}

/* Returns a pointer to `mapping_plugin_info`. */
static PluginInfo *get_mapping_plugin_info(void)
{
  Globals *g = get_globals();
  return (g) ? g->mapping_plugin_info : NULL;
}

/* Loads all plugins (if we haven't done that before) */
static void load_mapping_plugins(void)
{
  PluginInfo *info;
  FUIter *iter;
  const char *path;
  const unsigned char *hash;
  sha3_context c;
  Globals *g;

#ifdef WITH_PYTHON
  dlite_python_mapping_load();
#endif

  // FIXME - use pathshash() instead
  if (!(g = get_globals())) return;
  if (!(info = g->mapping_plugin_info)) return;
  if (!(iter = fu_pathsiter_init(&info->paths, NULL))) return;
  sha3_Init256(&c);
  while ((path = fu_pathsiter_next(iter)))
    sha3_Update(&c, path, strlen(path));

  hash = sha3_Finalize(&c);
  fu_pathsiter_deinit(iter);

  if (memcmp(hash, g->mapping_plugin_path_hash, 32) != 0) {
    plugin_load_all(info);
    memcpy(g->mapping_plugin_path_hash, hash, 32);
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
  const DLiteMappingPlugin *api;
  PluginInfo *info;

  if (!(info = get_mapping_plugin_info())) return NULL;
  if ((api = (DLiteMappingPlugin *)plugin_get_api(info, name))) return api;
  load_mapping_plugins();
  if ((api = (DLiteMappingPlugin *)plugin_get_api(info, name))) return api;
#ifdef WITH_PYTHON
  if ((api = dlite_python_mapping_get_api(name))) return api;
#endif
  /* Cannot find API */
  int i=0, j=2, m=0;
  char *buf=NULL;
  size_t size=0;
  const char **paths;
  m += asnpprintf(&buf, &size, m,
                  "cannot find mapping plugin for driver \"%s\" "
                  "in search path:\n", name);
  if ((paths = dlite_mapping_plugin_paths()))
    for (i=0; paths[i]; i++)
      m += asnpprintf(&buf, &size, m, "    %s\n", paths[i]);
#ifdef WITH_PYTHON
  if ((paths = dlite_python_mapping_paths_get()))
    for (j=0; paths[j]; j++)
      m += asnpprintf(&buf, &size, m, "    %s\n", paths[j]);
#endif
  if (i <= 1 || j <= 1)
    m += asnpprintf(&buf, &size, m,
                    "Are the DLITE_MAPPING_PLUGIN_DIRS and "
                    "DLITE_PYTHON_MAPPING_DIRS environment "
                    "variables set?");
  errx(1, "%s", buf);
  free(buf);
  return NULL;
}


/*
  Initiates a mapping plugin iterator.  Returns non-zero on error.
*/
int dlite_mapping_plugin_init_iter(DLiteMappingPluginIter *iter)
{
  PluginInfo *info;
  memset(iter, 0, sizeof(DLiteMappingPluginIter));
  load_mapping_plugins();
  if (!(info = get_mapping_plugin_info())) return 1;
  plugin_api_iter_init(&iter->iter, info);
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
  const DLiteMappingPlugin *api;
  if ((api = (const DLiteMappingPlugin *)plugin_api_iter_next(&iter->iter)))
    return api;
#ifdef WITH_PYTHON
  PluginInfo *info = (PluginInfo *)iter->iter.info;
  if (!iter->stop) {
    int n = iter->n;
    api = dlite_python_mapping_next(dlite_globals_get(), &iter->n);
    if (api && !plugin_has_api(info, api->name))
      plugin_register_api(info, (PluginAPI *)api);
    if (iter->n == n) iter->stop = 1;
  }
#endif
  return api;
}


/*
  Unloads and unregisters mapping plugin with the given name.

  If `name` is NULL, dlite_mapping_plugin_unload_all() is called.

  Returns non-zero on error.
*/
int dlite_mapping_plugin_unload(const char *name)
{
  int stat;
  PluginInfo *info;
  if (!(info = get_mapping_plugin_info())) return 1;
  if (name)
    stat = plugin_unload(info, name);
  else
    stat = dlite_mapping_plugin_unload_all();
  return stat;
}

/*
  Unloads and unregisters all mappings.  Returns non-zero on error.
*/
int dlite_mapping_plugin_unload_all(void)
{
  PluginInfo *info;
  char **p, **names;
  if (!(info = get_mapping_plugin_info())) return 1;
  if (!(names = plugin_names(info))) return 1;
  for (p=names; *p; p++) {
    plugin_unload(info, *p);
    free(*p);
  }
  free(names);
  return 0;
}

/*
  Returns a pointer to the underlying FUPaths object for storage plugins
  or NULL on error.
 */
FUPaths *dlite_mapping_plugin_paths_get(void)
{
  PluginInfo *info;
  if (!(info = get_mapping_plugin_info())) return NULL;
  return &info->paths;
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
  Returns an allocated string with the content of `paths` formatted
  according to the current platform.  See dlite_set_platform().

  Returns NULL on error.
 */
char *dlite_mapping_plugin_path_string(void)
{
  PluginInfo *info;
  if (!(info = get_mapping_plugin_info())) return NULL;
  return fu_paths_string(&info->paths);
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
int dlite_mapping_plugin_path_remove_index(int index)
{
  PluginInfo *info;
  if (!(info = get_mapping_plugin_info())) return 1;
  return plugin_path_remove_index(info, index);
}
