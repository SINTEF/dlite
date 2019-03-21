#include <assert.h>
#include <string.h>

#include "err.h"
#include "fileutils.h"
#include "dsl.h"
#include "plugin.h"


/* Struct holding data for a loaded plugin */
struct _Plugin {
  const void *api;
  dsl_handle handle;
};



/* Prototype for function that is looked up in shared library */
typedef const void *(*PluginFunc)(const char *name);


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
    if ((*p)->handle) dsl_close((*p)->handle);
    free(*p);
  }
  map_deinit(&info->plugins);
  free(info);
}


/*
  Returns index of plugin with given name, or -1 if no such plugin
  can be found.
 */
//int plugin_get_index(const PluginInfo *info, const char *name)
//{
//  size_t i;
//  for (i=0; i<info->nplugins; i++)
//    if (strcmp(*((char **)info->plugins[i]->api), name) == 0)
//      return i;
//  return -1;
//}


/*
  Registers a plugin with given `path`,  `api` and dsl `handle` into `info`.
  Returns non-zero on error.
 */
static int register_plugin(PluginInfo *info, const char *path,
			   const void *api, dsl_handle handle)
{
  Plugin *plugin;

  if (map_get(&info->plugins, path))
    return errx(2, "plugin %s is already registered", path);

  if (!(plugin = calloc(1, sizeof(Plugin))))
    return err(1, "allocation failure");
  plugin->api = api;
  plugin->handle = handle;

  if (map_set(&info->plugins, path, plugin)) {
    free(plugin);
    return errx(1, "failure to register plugin: %s", path);
  }

  return 0;
}


/*
  Registers plugin loaded from `path` with given api into `info`.
  Returns non-zero on error.
 */
int plugin_register(PluginInfo *info, const char *path, const void *api)
{
  return register_plugin(info, path, api, NULL);
}


/*
  Returns a pointer to a struct specific to the plugin-type that describes
  the api provided by the plugin.

  Returns NULL on error.
 */
const void *plugin_load(PluginInfo *info, const char *name, const char *pattern)
{
  FUIter *iter=NULL;
  const char *filepath;
  dsl_handle handle=NULL;
  void *sym=NULL;
  PluginFunc func;
  const void *api=NULL;

  if (!(iter = fu_startmatch(pattern, &info->paths))) goto fail;

  while ((filepath = fu_nextmatch(iter))) {

    /* check that plugin is not already loaded */
    if (map_get(&info->plugins, filepath)) continue;

    /* load plugin */
    if (!(handle = dsl_open(filepath))) {
      warn("cannot open plugin: \"%s\": %s", filepath, dsl_error());
      continue;
    }
    if (!(sym = dsl_sym(handle, info->symbol))) {
      //warn("dsl_sym: %s", dsl_error());
      dsl_close(handle);
      continue;
    }

    /* Silence gcc warning about that ISO C forbids conversion of object
       pointer to function pointer */
    *(void **)(&func) = sym;

    if (!(api = func(name))) {
      warn("failure calling \"%s\" in plugin \"%s\": %s",
           info->symbol, filepath, dsl_error());
      dsl_close(handle);
      continue;
    }

    if (strcmp(*((char **)api), name) == 0) {
      fu_endmatch(iter);
      if (register_plugin(info, filepath, api, handle)) goto fail;
      return api;
    }
  }

 fail:
  if (iter) fu_endmatch(iter);
  if (handle) dsl_close(handle);
  return NULL;
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
  const char *path;
  map_iter_t iter;

  /* Check already registered plugins */
  iter = map_iter(&info->plugins);
  while((path = map_next(&info->plugins, &iter))) {
    Plugin **p = map_get(&info->plugins, path);
    assert(p);
    if (strcmp(*((char **)((*p)->api)), name) == 0)
      return (*p)->api;
  }

  /* Load plugin from search path */
  if (!(pattern = malloc(strlen(name) + strlen(DSL_EXT) + 1)))
    return err(1, "allocation failure"), NULL;
  strcpy(pattern, name);
  strcat(pattern, DSL_EXT);
  if (!(api = plugin_load(info, name, pattern)) &&
      !(api = plugin_load(info, name, "*" DSL_EXT)))
    return NULL;

  if (pattern) free(pattern);
  return api;
}


/*
  Load all plugins that can be found in the plugin search path.
  Returns non-zero on error.
 */
int plugin_load_all(PluginInfo *info)
{
  FUIter *iter;
  char *pattern = malloc(strlen(DSL_EXT) + 2);
  const char *path;
  pattern[0] = '*';
  strcpy(pattern+1, DSL_EXT);
  if (!(iter = fu_startmatch(pattern, &info->paths))) return 1;
  while ((path = fu_nextmatch(iter))) {
    Plugin **p;
    if (!(p = map_get(&info->plugins, path)))
      plugin_register(info, path, (*p)->api);
  }
  fu_endmatch(iter);
  return 0;
}


/*
  Initiates a plugin iterator.
*/
void plugin_init_iter(PluginIter *iter, const PluginInfo *info)
{
  memset(iter, 0, sizeof(PluginIter));
  iter->info = info;
  iter->miter = map_iter(&info->plugins);
}

/*
  Returns pointer to the next registered plugin or NULL if all plugins
  has been visited.

  Used for iterating over plugins.  Plugins should not be registered
  or removed while iterating.
 */
const void *plugin_next(PluginIter *iter)
{
  const char *path = map_next((Plugins *)&iter->info->plugins, &iter->miter);
  Plugin **p = map_get((Plugins *)&iter->info->plugins, path);
  assert(p);
  return (*p)->api;
}


/*
  Unloads and unregisters plugin with the given name.
  Returns non-zero on error.
*/
int plugin_unload(PluginInfo *info, const char *name)
{
  const char *path, *delpath=NULL;
  map_iter_t miter = map_iter(&info-plugins);
  while ((path = map_next(&info->plugins, &miter))) {
    Plugin *plugin, **p = map_get(&info->plugins, path);
    assert(p);
    plugin = *p;
    if (strcmp(*(char **)(plugin->api), name) == 0) {
      delpath = path;
      dsl_close(plugin->handle);
      free(plugin);
      break;
    }
  }
  if (!delpath)
    return err(1, "no such plugin: \"%s\"", name);
  map_remove(&info->plugins, delpath);
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
