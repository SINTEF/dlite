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

  return info;
}


/*
  Free's plugin info.
 */
void plugin_info_free(PluginInfo *info)
{
  size_t i;
  free((char *)info->kind);
  free((char *)info->symbol);
  if (info->envvar) free((char *)info->envvar);
  fu_paths_deinit(&info->paths);
  for (i=0; i<info->nplugins; i++) {
    if (info->plugins[i]->handle) dsl_close(info->plugins[i]->handle);
    free(info->plugins[i]);
  }
  if (info->plugins) free(info->plugins);
  free(info);
}


/*
  Returns index of plugin with given name, or -1 if no such plugin
  can be found.
 */
int plugin_get_index(const PluginInfo *info, const char *name)
{
  size_t i;
  for (i=0; i<info->nplugins; i++)
    if (strcmp(*((char **)info->plugins[i]->api), name) == 0)
      return i;
  return -1;
}


/*
  Registers a plugin with given `api` and dsl `handle` into `info`.
  Returns non-zero on error.
 */
static int register_plugin(PluginInfo *info, const void *api, dsl_handle handle)
{
  Plugin *plugin;

  if (info->nplugins >= info->nalloc) {
    info->nalloc = info->nplugins + 32;
    if (!(info->plugins = realloc(info->plugins,
                                  info->nalloc*sizeof(Plugin *))))
      return err(1, "allocation failure");
  }

  if (!(plugin = calloc(1, sizeof(Plugin))))
    return err(1, "allocation failure");
  plugin->api = api;
  plugin->handle = handle;

  info->plugins[info->nplugins++] = plugin;
  return 0;
}


/*
  Registers plugin with given api into `info`.  Returns non-zero on error.
 */
int plugin_register(PluginInfo *info, const void *api)
{
  return register_plugin(info, api, NULL);
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
      if (register_plugin(info, api, handle)) goto fail;
      return api;
    }
  }

 fail:
  if (iter) fu_endmatch(iter);
  if (handle) dsl_close(handle);
  return NULL;
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
const void *plugin_get_api(PluginInfo *info, const char *name)
{
  size_t i;
  const void *api=NULL;
  char *pattern=NULL;

  /* Check already registered plugins */
  for (i=0; i < info->nplugins; i++)
    if (strcmp(*((char **)info->plugins[i]->api), name) == 0)
      return info->plugins[i]->api;

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
  Unloads and unregisters plugin with the given name.
  Returns non-zero on error.
*/
int plugin_unload(PluginInfo *info, const char *name)
{
  int n = plugin_get_index(info, name);
  if (n < 0) return errx(1, "no such plugin: \"%s\"", name);
  dsl_close(info->plugins[n]->handle);
  free(info->plugins[n]);
  info->plugins[n] = info->plugins[--info->nplugins];
  if (info->nplugins == 0) {
    free(info->plugins);
    info->plugins = NULL;
  }
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
