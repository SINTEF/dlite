#include <assert.h>
#include <string.h>

#include "err.h"
#include "fileutils.h"
#include "dsl.h"
#include "uuid4.h"
#include "plugin.h"

/** Convenient macros for failing */
#define FAIL(msg) do { \
    err(1, msg); goto fail; } while (0)
#define FAIL1(msg, a1) do { \
    err(1, msg, a1); goto fail; } while (0)
#define FAIL2(msg, a1, a2) do { \
    err(1, msg, a1, a2); goto fail; } while (0)



/* Struct holding data for a loaded plugin */
struct _Plugin {
  char *path;          /* plugin file path */
  int count;           /* number of APIs associated to this plugin */
  dsl_handle handle;   /* plugin handle */
};


int plugin_incref(Plugin *plugin)
{
  return ++plugin->count;
}

int plugin_decref(Plugin *plugin)
{
  int count = --plugin->count;
  if (count <= 0) {
    free(plugin->path);
    dsl_close(plugin->handle);
    free(plugin);
  }
  return count;
}


/*
  Creates a new plugin type and returns a pointer to information about it.

  `name` is the name of the new plugin type.
  `symbol` is the name of the function that plugins should define
  `envvar` is the name of environment variable with plugin search path

  Returns NULL on error.
*/
PluginInfo *plugin_info_create(const char *kind, const char *symbol,
                               const char *envvar)
{
  PluginInfo *info=NULL;

  if (!(info = calloc(1, sizeof(PluginInfo))))
    return err(1, "allocation failure"), NULL;

  info->kind = strdup(kind);
  info->symbol = strdup(symbol);
  info->envvar = (envvar) ? strdup(envvar) : NULL;

  fu_paths_init(&info->paths, envvar);
  map_init(&info->plugins);
  map_init(&info->pluginpaths);
  map_init(&info->apis);

  return info;
}


/*
  Free's plugin info.
 */
void plugin_info_free(PluginInfo *info)
{
  map_iter_t iter;
  const char *path;

  free((char *)info->kind);
  free((char *)info->symbol);
  if (info->envvar) free((char *)info->envvar);
  fu_paths_deinit(&info->paths);

  iter = map_iter(&info->plugins);
  while ((path = map_next(&info->plugins, &iter))) {
    Plugin **p = map_get(&info->plugins, path);
    assert(p);
    plugin_decref(*p);
  }
  map_deinit(&info->plugins);
  map_deinit(&info->pluginpaths);
  map_deinit(&info->apis);
  free(info);
}


/*
  Help function for plugin_register_api().  Registers a plugin with given
  `path`, `api` and dsl `handle` into `info`.

  Returns non-zero on error.
 */
static int register_api(PluginInfo *info, const void *api,
			   const char *path, dsl_handle handle)
{
  char *name;
  Plugin *plugin=NULL;
  assert(api);
  name = *((char **)api);

  if (map_get(&info->apis, name))
    return errx(1, "api already registered: %s", name);

  if (path) {
    assert(handle);
    if (map_get(&info->plugins, path)) {
      warnx("plugin already registered: %s", path);
    } else {
      if (!(plugin = calloc(1, sizeof(Plugin)))) FAIL("allocation failure");
      if (!(plugin->path = strdup(path))) FAIL("allocation failure");
      plugin->count++;
      plugin->handle = handle;
      if (map_set(&info->plugins, plugin->path, plugin))
        fatal(1, "failed to register plugin: %s", path);
      if (map_set(&info->pluginpaths, name, plugin->path))
        fatal(1, "failed to map plugin name '%s' to path: %s", name, path);
    }
  }

  if (map_set(&info->apis, name, (void *)api))
    fatal(1, "failed to register api: %s", name);

  return 0;
 fail:
  if (plugin->path) free(plugin->path);
  if (plugin) free(plugin);
  return 1;
}


/*
  Registers `api` into `info`.  Returns non-zero on error.
 */
int plugin_register_api(PluginInfo *info, const void *api)
{
  return register_api(info, api, NULL, NULL);
}


/*
  Looks up all file names matching `pattern` in the plugin search
  paths in `info` and try to load it as a plugin.  If it succeeds and
  `name` matches the plugin name, the plugin is registered and a
  pointer to the plugin API is returned.

  If `name` is NULL, all plugins matching `pattern` are registered and a
  pointer to latest successfully loaded API is returned.

  If `emit_err` is non-zero, an error message will be emitted in case a
  named plugin cannot be loaded.

  Returns a pointer to the plugin API or NULL on error.
 */
const void *plugin_load(PluginInfo *info, const char *name,
                        const char *pattern, int emit_err)
{
  FUIter *iter=NULL;
  const char *filepath;
  dsl_handle handle=NULL;
  void *sym=NULL;
  PluginFunc func;
  const void *api=NULL, *loaded_api=NULL, *retval=NULL;

  if (!(iter = fu_startmatch(pattern, &info->paths))) goto fail;

  while ((filepath = fu_nextmatch(iter))) {
    int iter1=0, iter2=0;
    err_clear();

    /* check that plugin is not already loaded */
    if (map_get(&info->plugins, filepath)) continue;

    /* load plugin */
    if (!(handle = dsl_open(filepath))) {
      warn("cannot open plugin: \"%s\": %s", filepath, dsl_error());
      continue;
    }
    if (!(sym = dsl_sym(handle, info->symbol))) {
      warn("dsl_sym: %s", dsl_error());
      dsl_close(handle);
      continue;
    }
    err_clear();

    /* Silence gcc warning about that ISO C forbids conversion of object
       pointer to function pointer */
    *(void **)(&func) = sym;

    while ((api = func(&iter1))) {
      char *apiname = *((char **)api);
      if (!map_get(&info->apis, apiname)) {  /* not plugin with this name */
        loaded_api = api;
        if (!name) {
          register_api(info, api, filepath, handle);
        } else if (strcmp(apiname, name) == 0) {
          if (register_api(info, api, filepath, handle)) goto fail;
          fu_endmatch(iter);
          return api;
        }
      }
      if (iter1 == iter2) break;
      iter2 = iter1;
    }
    if (!api) warn("failure calling \"%s\" in plugin \"%s\": %s",
                   info->symbol, filepath, dsl_error());
  }
  if (name && emit_err)
    errx(1, "no such api: \"%s\"", name);
  else
    retval = loaded_api;
 fail:
  if (!retval && handle) dsl_close(handle);
  if (iter) fu_endmatch(iter);

  return retval;
}


/*
  Returns a plugin with the given name, or NULL if it cannot be found.

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
const void *plugin_get_api(PluginInfo *info, const char *name)
{
  const void *api=NULL;
  char *pattern=NULL;
  void **p;

  /* Check already registered apis */
  if ((p = map_get(&info->apis, name)))
    return (const void *)*p;

  /* Load plugin from search path */
  if (!(pattern = malloc(strlen(name) + strlen(DSL_EXT) + 1)))
    return err(1, "allocation failure"), NULL;
  strcpy(pattern, name);
  strcat(pattern, DSL_EXT);
  if (!(api = plugin_load(info, name, pattern, 0)) &&
      !(api = plugin_load(info, name, "*" DSL_EXT, 1)))
    err(1, "cannot find api: '%s'", name);

  if (pattern) free(pattern);
  return api;
}


/*
  Load all plugins that can be found in the plugin search path.
 */
void plugin_load_all(PluginInfo *info)
{
  char *pattern = malloc(strlen(DSL_EXT) + 2);
  pattern[0] = '*';
  strcpy(pattern+1, DSL_EXT);
  while (1)
    if (!plugin_load(info, NULL, pattern, 0)) break;
  free(pattern);
}


/*
  Initiates a plugin API iterator.
*/
void plugin_api_iter_init(PluginIter *iter, const PluginInfo *info)
{
  memset(iter, 0, sizeof(PluginIter));
  iter->info = info;
  iter->miter = map_iter(&info->apis);
}

/*
  Returns pointer to the next registered API or NULL if all APIs
  have been visited.

  Used for iterating over plugins.  Plugins should not be registered
  or removed while iterating.
 */
const void *plugin_api_iter_next(PluginIter *iter)
{
  void **p, *api;
  PluginInfo *info = (PluginInfo *)iter->info;
  const char *name = map_next(&info->apis, &iter->miter);
  if (!name) return NULL;
  if (!(p = map_get(&info->apis, name)) || !(api = *p))
    fatal(1, "failed to get api: %s", name);
  return (const void *)api;
}


/*
  Unloads and unregisters plugin with the given name.
  Returns non-zero on error.
*/
int plugin_unload(PluginInfo *info, const char *name)
{
  char **ppath;
  if (!map_get(&info->apis, name))
    return errx(1, "cannot unload api: %s", name);
  if ((ppath = map_get(&info->pluginpaths, name))) {
    Plugin **p;
    assert(*ppath);
    if ((p = map_get(&info->plugins, *ppath))) {
      char *path;
      assert(*p);
      if (!(path = strdup(*ppath))) return err(1, "allocation failure");
      if (plugin_decref(*p) <= 0) map_remove(&info->plugins, path);
      free(path);
    }
  }
  map_remove(&info->pluginpaths, name);
  map_remove(&info->apis, name);
  return 0;
}


/*
  Returns a NULL-terminated array of pointers to search paths or NULL
  if no search path is defined.
 */
const char **plugin_path_get(PluginInfo *info)
{
  return fu_paths_get(&info->paths);
}

/*
  Inserts `path` into the current search path at index `n`.  If `n` is
  negative, it counts from the end of the search path (like Python).

  If `n` is out of range, it is clipped.

  Returns the index of the newly inserted path or -1 on error.
 */
int plugin_path_insert(PluginInfo *info, const char *path, int n)
{
  return fu_paths_insert(&info->paths, path, n);
}

/*
  Appends `path` into the current search path.

  Returns the index of the newly appended path or -1 on error.
*/
int plugin_path_append(PluginInfo *info, const char *path)
{
  return fu_paths_append(&info->paths, path);

}

/*
  Removes path index `n` from current search path.  If `n` is
  negative, it counts from the end (like Python).

  Returns non-zero on error.
 */
int plugin_path_remove(PluginInfo *info, int n)
{
  return fu_paths_remove(&info->paths, n);
}
