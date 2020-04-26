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

      const void *symbol(int *iter);

  This function should return a pointer to a struct with function
  pointers to all functions provided by the plugin (data member are
  also allowed).  Below we refer to this struct as the plugin API.
  The first element in the API must be a pointer to a string containg
  the name of the plugin.  Plugin names should be unique.

  The `iter` argument is normally ignored.  It is provided to support
  plugins exposing several APIs.  If the plugin has more APIs to
  expose, it should increase the integer pointed to by `iter` by one.

  A new plugin kind, with its own API, can be created with
  plugin_info_create().

  @see http://gernotklingler.com/blog/creating-using-shared-libraries-different-compilers-different-operating-systems/
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dsl.h"
#include "fileutils.h"
#include "globmatch.h"
#include "map.h"

/** Prototype for function that is looked up in shared library */
typedef const void *(*PluginFunc)(int *iter);

/** Opaque struct for list of plugins */
typedef struct _Plugin Plugin;

/** Maps plugin file names to plugins */
typedef map_t(Plugin *) Plugins;


/** Info about a plugin kind */
typedef struct _PluginInfo {
  const char *kind;      /*!< Name of this plugin kind */
  const char *symbol;    /*!< Name of function in plugin returning the api */
  const char *envvar;    /*!< Name of environment variable initialising the
                              plugin search path */
  FUPaths paths;         /*!< Current plugin search paths */
  Plugins plugins;       /*!< Maps plugin paths to loaded plugins */
  map_str_t pluginpaths; /*!< Maps api names to plugin paths */
  map_void_t apis;       /*!< Maps api names to apis (void pointers) */
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

  Returns NULL on error.
*/
PluginInfo *plugin_info_create(const char *kind, const char *symbol,
                               const char *envvar);

/**
  Free's plugin info.
 */
void plugin_info_free(PluginInfo *info);


/*
  Registers `api` into `info`.  Returns non-zero on error.
 */
int plugin_register_api(PluginInfo *info, const void *api);


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
const void *plugin_get_api(PluginInfo *info, const char *name);

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
const void *plugin_api_iter_next(PluginIter *iter);



/**
  Unloads and unregisters plugin with the given name.
  Returns non-zero on error.
*/
int plugin_unload(PluginInfo *info, const char *name);


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
  Removes path index `n` from current search path.  If `n` is
  negative, it counts from the end (like Python).

  Returns non-zero on error.
 */
int plugin_path_remove(PluginInfo *info, int n);



#endif /* _PLUGINS_H */
