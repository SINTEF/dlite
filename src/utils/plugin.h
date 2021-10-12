/* plugins.h -- simple plugin library
 *
 * Copyright (C) 2017 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#ifndef _PLUGINS_H
#define _PLUGINS_H

/**
  @file
  @brief Simple portable plugin library

  Plugins accessed with this library, are dynamic shared libraries
  exposing a single function with prototype

      const PluginAPI *symbol(void *state, int *iter);

  This function should return a pointer to a struct with function
  pointers to all functions provided by the plugin (data member are
  also allowed).  Below we refer to this struct as the plugin API.
  The first element in the API must be a pointer to a string containg
  the name of the plugin.  Plugin names should be unique.

  The `state` argument is used to pass a pointer to the global state
  of the caller to the plugin.  You may use the session.h module for that.

  The `iter` argument is normally ignored.  It is provided to support
  plugins exposing several APIs.  `*iter` will be zero at the first
  time the function is called.  If the plugin has more APIs to
  expose, it should increase `*iter` by one to indicate that it should
  be called again to return the next API.  When returning the last API,
  it should leave `*iter` unchanged.

  A new plugin kind, with its own API, can be created with
  plugin_info_create().

  @see http://gernotklingler.com/blog/creating-using-shared-libraries-different-compilers-different-operating-systems/
 */

#include "dsl.h"
#include "fileutils.h"
#include "globmatch.h"
#include "map.h"


/** Initial fields in all plugin APIs. */
#define PluginAPI_HEAD                                                  \
  char *name;                               /* Plugin name */           \
  void (*freeapi)(struct _PluginAPI *api);  /* Optional function */     \
                                            /* that free's instances */ \
                                            /* of this struct */


/** Base declaration of a plugin API that all plugin APIs can be cast
    into. */
typedef struct _PluginAPI {
  PluginAPI_HEAD
} PluginAPI;

/** Prototype for function that is looked up in shared library.
    See above for more info. */
typedef const PluginAPI *(*PluginFunc)(void *state, int *iter);

/** Opaque struct for list of loaded plugins (shared libraries) */
typedef struct _Plugin Plugin;

/** New map types for plugins and plugin apis */
typedef map_t(Plugin *) map_plg_t;
typedef map_t(PluginAPI *) map_api_t;

/** Info about a plugin kind */
typedef struct _PluginInfo {
  const char *kind;      /*!< Name of this plugin kind */
  const char *symbol;    /*!< Name of function in plugin returning the api */
  const char *envvar;    /*!< Name of environment variable initialising the
                              plugin search path */
  void *state;           /*!< Pointer to global state passed to PluginFunc */
  FUPaths paths;         /*!< Current plugin search paths */
  map_plg_t plugins;     /*!< Maps plugin paths to loaded plugins */
  map_str_t pluginpaths; /*!< Maps api names to plugin path names */
  map_api_t apis;        /*!< Maps api names to plugin apis */
} PluginInfo;


/** Struct for iterating over registered plugins */
typedef struct _PluginIter {
  const PluginInfo *info;
  map_iter_t miter;
} PluginIter;


/**
  Creates a new plugin kind and returns a pointer to information about it.

  `kind` is the name of the new plugin kind.
  `symbol` is the name of the function that plugins should define.
  `envvar` is the name of environment variable with plugin search path.
  `state` pointer to global state passed to the plugin function.

  Returns NULL on error.
*/
PluginInfo *plugin_info_create(const char *kind, const char *symbol,
                               const char *envvar, void *state);

/**
  Free's plugin info.
 */
void plugin_info_free(PluginInfo *info);


/**
  Returns pointer to plugin api.

  If a plugin with the given name is already registered, it is returned.

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
const PluginAPI *plugin_get_api(PluginInfo *info, const char *name);

/**
  Load all plugins that can be found in the plugin search path.
  Returns non-zero on error.
 */
void plugin_load_all(PluginInfo *info);



/**
  Initiates a plugin iterator.
*/
void plugin_api_iter_init(PluginIter *iter, const PluginInfo *info);

/**
  Returns pointer to the next registered API or NULL if all APIs
  have been visited.

  Used for iterating over plugins.  Plugins should not be registered
  or removed while iterating.
 */
const PluginAPI *plugin_api_iter_next(PluginIter *iter);



/**
  Unloads and unregisters plugin with the given name.
  Returns non-zero on error.
*/
int plugin_unload(PluginInfo *info, const char *name);


/**
  Returns a NULL-terminated array of pointers to api names.
  Returns NULL on error.
*/
char **plugin_names(const PluginInfo *info);


/**
  Returns a NULL-terminated array of pointers to search paths or NULL
  if no search path is defined.
 */
const char **plugin_path_get(PluginInfo *info);

/**
  Inserts `path` into the current search path at index `n`.  If `n` is
  negative, it counts from the end of the search path (like Python).

  If `n` is out of range, it is clipped.

  Returns the index of the newly inserted path or -1 on error.
 */
int plugin_path_insert(PluginInfo *info, const char *path, int n);

/**
  Appends `path` into the current search path.

  Returns the index of the newly appended path or -1 on error.
 */
int plugin_path_append(PluginInfo *info, const char *path);

/**
  Like plugin_path_append(), but appends at most the `n` first bytes
  of `path` to the current search path.

  Returns the index of the newly appended path or -1 on error.
*/
int plugin_path_appendn(PluginInfo *info, const char *path, size_t n);

/**
  Extends current search path by appending all `pathsep`-separated paths
  in `s` to it.

  Returns the index of the last appended path or zero if nothing is appended.
  On error, -1 is returned.
*/
int plugin_path_extend(PluginInfo *info, const char *s, const char *pathsep);

/**
  Like plugin_paths_extend(), but prefix all relative paths in `s`
  with `prefix` before appending them to `paths`.

  Returns the index of the last appended paths or zero if nothing is appended.
  On error, -1 is returned.
*/
int plugin_path_extend_prefix(PluginInfo *info, const char *prefix,
                              const char *s, const char *pathsep);

/**
  Removes path index `n` from current search path.  If `n` is
  negative, it counts from the end (like Python).

  Returns non-zero on error.
 */
int plugin_path_remove_index(PluginInfo *info, int index);

/**
  Removes path `path`.  Returns non-zero if there is no such path.
 */
int plugin_path_remove(PluginInfo *info, const char *path);

/**
  Returns index of plugin path `path` or -1 on error.
 */
int plugin_path_index(PluginInfo *info, const char *path);


#endif /* _PLUGINS_H */
