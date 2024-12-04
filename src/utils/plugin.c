/* plugins.c -- simple plugin library
 *
 * Copyright (C) 2017 SINTEF
 *
 * Distributed under terms of the MIT license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
    (void)dsl_close(plugin->handle);
    free(plugin);
  }
  return count;
}


/*
  Creates a new plugin type and returns a pointer to information about it.

  `kind` is the name of the new plugin kind.
  `symbol` is the name of the function that plugins should define
  `envvar` is the name of environment variable with plugin search path
  `state` pointer to global state passed to the plugin function.

  Returns NULL on error.
*/
PluginInfo *plugin_info_create(const char *kind, const char *symbol,
                               const char *envvar, void *state)
{
  PluginInfo *info=NULL;
  if (!(info = calloc(1, sizeof(PluginInfo))))
    return err(1, "allocation failure"), NULL;

  info->kind = strdup(kind);
  info->symbol = strdup(symbol);
  info->envvar = (envvar) ? strdup(envvar) : NULL;
  info->state = state;
  fu_paths_init(&info->paths, envvar);

  map_init(&info->plugins);
  map_init(&info->pluginpaths);
  map_init(&info->apis);

  return info;
}


/*
  Free's plugin info and all corresponding APIs.
 */
void plugin_info_free(PluginInfo *info)
{
  map_iter_t iter;
  const char *path, *name;

  free((char *)info->kind);
  free((char *)info->symbol);
  if (info->envvar) free((char *)info->envvar);
  fu_paths_deinit(&info->paths);

  iter = map_iter(&info->apis);
  while ((name = map_next(&info->apis, &iter))) {
    PluginAPI **p = map_get(&info->apis, name);
    PluginAPI *api = *p;
    assert(api);
    if (api->freeapi) api->freeapi(api);
  }
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
  Help function for plugin_load().  Registers a plugin with given
  `path`, `api` and dsl `handle` into `info`.

  Returns non-zero on error.
 */
static int register_plugin(PluginInfo *info, const PluginAPI *api,
			   const char *path, dsl_handle handle)
{
  char *name = api->name;
  Plugin *plugin = NULL;
  assert(api);

  if (map_get(&info->apis, name))
    return errx(1, "api already registered: %s", name);

  if (path) {
    Plugin **p;
    assert(handle);
    if ((p = map_get(&info->plugins, path))) {
      /* Plugin is already registered (but it may still provides more
         plugin APIs... */
      plugin = *p;
      plugin_incref(plugin);
    } else {
      if (!(plugin = calloc(1, sizeof(Plugin)))) FAIL("allocation failure");
      if (!(plugin->path = strdup(path))) FAIL("allocation failure");
      plugin_incref(plugin);
      plugin->handle = handle;

      if (map_set(&info->plugins, plugin->path, plugin))
        fatal(1, "failed to register plugin: %s", path);
    }
    if (map_set(&info->pluginpaths, name, plugin->path))
      fatal(1, "failed to map plugin name '%s' to path: %s", name, path);
  }

  if (map_set(&info->apis, name, (PluginAPI *)api))
    fatal(1, "failed to register api: %s", name);

  return 0;
 fail:
  if (plugin) {
    map_remove(&info->pluginpaths, name);
    if (plugin->path) {
      map_remove(&info->plugins, plugin->path);
      free(plugin->path);
    }
    free(plugin);
  }
  return 1;
}


/*
  Looks up all file names matching `pattern` in the plugin search
  paths in `info` and try to load it as a plugin.  If it succeeds and
  `name` matches the plugin name, the plugin is registered and a
  pointer to the plugin API is returned.

  If `name` is NULL, all plugins matching `pattern` are registered and a
  pointer to latest successfully loaded API is returned.

  If `errcode` is non-zero, an error with this code will be emitted in case a
  named plugin cannot be loaded.

  Returns a pointer to the plugin API or NULL on error.
 */
const PluginAPI *plugin_load(PluginInfo *info, const char *name,
                             const char *pattern, int errcode)
{
  FUIter *iter=NULL;
  const char *filepath;
  dsl_handle handle=NULL;
  void *sym=NULL;
  PluginFunc func;
  PluginAPI *api=NULL, **apiptr;
  const void *loaded_api=NULL, *registered_api=NULL, *retval=NULL;

  if (!(iter = fu_startmatch(pattern, &info->paths))) goto fail;

  /* Check if plugin is already loaded */
  if (name && (apiptr = map_get(&info->apis, name)))
    return *apiptr;

  while ((filepath = fu_nextmatch(iter))) {
    int iter1=0, iter2=0;
    err_clear();

    /* load plugin */
    if (!(handle = dsl_open(filepath))) {
      warn("cannot open plugin: \"%s\": %s", filepath, dsl_error());
      continue;
    }

    if (!(sym = dsl_sym(handle, info->symbol))) {
      warn("dsl_sym: %s", dsl_error());
      (void)dsl_close(handle);
      continue;
    }
    err_clear();

    /* Silence gcc warning about that ISO C forbids conversion of object
       pointer to function pointer */
    *(void **)(&func) = sym;

    while ((api = (PluginAPI *)func(info->state, &iter1))) {
      registered_api = NULL;

      if (!map_get(&info->apis, api->name)) {  /* no plugin with this name */
        loaded_api = api;
        if (!name) {
          if (!register_plugin(info, api, filepath, handle))
            registered_api = api;
        } else if (strcmp(api->name, name) == 0) {
          if (register_plugin(info, api, filepath, handle)) goto fail;
          registered_api = api;
          fu_endmatch(iter);
          return api;
        }
      }
      if (!registered_api && api && api->freeapi)
        api->freeapi(api);

      if (iter1 == iter2) break;
      iter2 = iter1;
    }
    if (!api)
      warn("failure calling \"%s\" in plugin \"%s\": %s",
           info->symbol, filepath, dsl_error());

    if (!registered_api && handle) {
      if (handle && dsl_close(handle))
        err(1, "error closing \"%s\": %s", filepath, dsl_error());
      handle = NULL;
    }
  }
  if (name) {
    if (errcode) errx(errcode, "no such plugin: \"%s\"", name);
    retval = NULL;
  } else {
    retval = loaded_api;
  }
 fail:
  if (!retval && handle)
    (void)dsl_close(handle);
  if (iter) fu_endmatch(iter);

  return retval;
}


/*
  Register a plugin `api` not associated to a dynamic loadable library.

  This function may e.g. be useful for registering plugins written in
  dynamic interpreted languages, like Python.
 */
int plugin_register_api(PluginInfo *info, const PluginAPI *api)
{
  if (map_get(&info->apis, api->name))
    return errx(1, "api already registered: %s", api->name);
  map_set(&info->apis, api->name, (PluginAPI *)api);
  return 0;
}


/*
  Returns non-zero if plugin api `name` is already registered.
 */
int plugin_has_api(PluginInfo *info, const char *name)
{
  return (map_get(&info->apis, name)) ? 1 : 0;
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

  If the plugin is not found, `err()` is called with `eval` set to `errcode`.

  Otherwise NULL is returned.
 */
const PluginAPI *plugin_get_api(PluginInfo *info, const char *name, int errcode)
{
  const PluginAPI *api=NULL;
  PluginAPI **p;
  char *pattern=NULL;

  /* Check already registered apis */
  if ((p = map_get(&info->apis, name)))
    return (const PluginAPI *)*p;

  /* Load plugin from search path */
  if (!(pattern = malloc(strlen(name) + strlen(DSL_EXT) + 1)))
    return err(pluginMemoryError, "allocation failure"), NULL;
  strcpy(pattern, name);
  strcat(pattern, DSL_EXT);
  if (!(api = plugin_load(info, name, pattern, 0)))
    api = plugin_load(info, name, "*" DSL_EXT, errcode);

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
  Unloads and unregisters plugin with the given name.
  Returns non-zero on error.
*/
int plugin_unload(PluginInfo *info, const char *name)
{
  char **ppath;
  char *pname = strdup(name);
  PluginAPI **apip, *api;
  int retval = 1;
  if (!(apip = map_get(&info->apis, pname)))
    FAIL1("cannot unload api: %s", pname);
  api = *apip;

  if (api->freeapi) api->freeapi(api);
  if ((ppath = map_get(&info->pluginpaths, pname))) {
    Plugin **p;
    assert(*ppath);
    if ((p = map_get(&info->plugins, *ppath))) {
      char *path;
      assert(*p);
      if (!(path = strdup(*ppath))) FAIL("allocation failure");
      if (plugin_decref(*p) <= 0) map_remove(&info->plugins, path);
      free(path);
    }
  }
  map_remove(&info->pluginpaths, pname);
  map_remove(&info->apis, pname);
  retval = 0;
 fail:
  free(pname);
  return retval;
}

/*
  Returns a NULL-terminated array of pointers to api names.
  Returns NULL on error.
*/
char **plugin_names(const PluginInfo *info)
{
  int n=0, size=0;
  char **names=NULL;
  PluginIter iter;
  const PluginAPI *api;
  plugin_api_iter_init(&iter, info);
  while ((api = plugin_api_iter_next(&iter))) {
    if (n >= size) {
      void *ptr;
      size += 16;
      if (!(ptr = realloc(names, size * sizeof(char *)))) {
        free(names);
        return err(1, "allocation failure"), NULL;
      }
      names = ptr;
    }
    names[n++] = strdup(api->name);
  }
  if (names) names[n] = NULL;
  return names;
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
const PluginAPI *plugin_api_iter_next(PluginIter *iter)
{
  PluginAPI **p, *api=NULL;
  PluginInfo *info = (PluginInfo *)iter->info;
  const char *name = map_next(&info->apis, &iter->miter);
  if (!name) return NULL;
  if (!(p = map_get(&info->apis, name)) || !(api = *p))
    fatal(1, "failed to get api: %s", name);
  return (const void *)api;
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
  Like plugin_path_append(), but appends at most the `n` first bytes
  of `path` to the current search path.

  Returns the index of the newly appended path or -1 on error.
*/
int plugin_path_appendn(PluginInfo *info, const char *path, size_t n)
{
  return fu_paths_appendn(&info->paths, path, n);
}

/*
  Extends current search path by appending all `pathsep`-separated paths
  in `s` to it.

  Returns the index of the last appended path or zero if nothing is appended.
  On error, -1 is returned.
 */
int plugin_path_extend(PluginInfo *info, const char *s, const char *pathsep)
{
  const char *p;
  char *endptr=NULL;
  int stat=0;

  while ((p = fu_nextpath(s, &endptr, pathsep))) {
    if (*p && (stat = plugin_path_appendn(info, p, endptr - p)) < 0)
      return stat;
  }
  return stat;
}

/*
  Like plugin_paths_extend(), but prefix all relative paths in `s`
  with `prefix` before appending them to `paths`.

  Returns the index of the last appended paths or zero if nothing is appended.
  On error, -1 is returned.
*/
int plugin_path_extend_prefix(PluginInfo *info, const char *prefix,
                              const char *s, const char *pathsep)
{
  const char *p;
  char *endptr=NULL;
  int stat=0;

  while ((p = fu_nextpath(s, &endptr, pathsep))) {
    int len = endptr - p;
    if (fu_isabs(p)) {
      if ((stat = plugin_path_appendn(info, p, len)) < 0) return stat;
    } else {
      char buf[1024];
      int n = snprintf(buf, sizeof(buf), "%s/%.*s", prefix, len, p);
      if (n < 0) return err(-1, "unexpected error in snprintf()");
      if (n >= (int)sizeof(buf) - 1)
        return err(-1, "path exeeds buffer size: %s/%.*s", prefix, len, p);
      if ((stat = plugin_path_append(info, buf)) < 0) return stat;
    }
  }
  return stat;
}


/*
  Removes path index `n` from current search path.  If `n` is
  negative, it counts from the end (like Python).

  Returns non-zero on error.
 */
int plugin_path_remove_index(PluginInfo *info, int index)
{
  return fu_paths_remove_index(&info->paths, index);
}

/*
  Removes path `path`.  Returns non-zero if there is no such path.
 */
int plugin_path_remove(PluginInfo *info, const char *path)
{
  int i = plugin_path_index(info, path);
  if (i < 0) return i;
  return plugin_path_remove_index(info, i);
}

/*
  Returns index of plugin path `path` or -1 on error.
 */
int plugin_path_index(PluginInfo *info, const char *path)
{
  return fu_paths_index(&info->paths, path);
}
