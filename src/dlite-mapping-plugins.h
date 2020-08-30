#ifndef _DLITE_MAPPING_PLUGINS_H
#define _DLITE_MAPPING_PLUGINS_H

/**
  @file
  @brief Common API for all mapping plugins (internal).

  A DPite mapping plugin should be a shared library that defines the
  function

      const DLiteMappingPlugin *
      get_dlite_mapping_api(const char *name);

  that returns a pointer to a struct with pointers to all functions
  provided by the plugin.  The `name` is just a hint that plugins are
  free to ignore.  It is used by mapping plugins that supports
  several different drivers, to select which api that should be
  returned.

  The mapping plugin search path is initialised from the environment
  variable `DLITE_MAPPING_PLUGIN_DIRS`.
*/

#include "utils/dsl.h"
#include "utils/plugin.h"
#include "dlite-entity.h"

/**
  A struct with data and function pointers provided by a plugin.
 */
typedef struct _DLiteMappingPlugin DLiteMappingPlugin;

/**
 An iterator over all registered mapping plugins.
 */
typedef struct _PluginIter DLiteMappingPluginIter;


/**
  Prototype for function returning a pointer to a
  DLiteMappingPlugin or NULL on error.

  The `iter` argument is normally ignored.  It is provided to support
  plugins exposing several APIs.  If the plugin has more APIs to
  expose, it should increase the integer pointed to by `iter` by one.
 */
typedef const DLiteMappingPlugin *(*GetDLiteMappingAPI)(int *iter);


/**
  @name Plugin frontend
  @{
 */

/**
  Returns a mapping plugin with the given name, or NULL if it
  cannot be found.

  If a plugin with the given name is registered, it is returned.

  Otherwise the plugin search path is checked for shared libraries
  matching `name.EXT` where `EXT` is the extension for shared library
  on the current platform ("dll" on Windows and "so" on Unix/Linux).
  If a plugin with the provided name is found, it is loaded,
  registered and returned.

  Otherwise the plugin search path is checked again, but this time for
  any shared library.  If a plugin with the provided name is found, it
  is loaded, registered and returned.

  Otherwise NULL is returned.
 */
const DLiteMappingPlugin *dlite_mapping_plugin_get(const char *name);

/**
  Registers `api` for a mapping plugin.  Returns non-zero on error.
*/
int dlite_mapping_plugin_register_api(const DLiteMappingPlugin *api);

/**
  Initiates a mapping plugin iterator.  Returns non-zero on error.
*/
int dlite_mapping_plugin_init_iter(DLiteMappingPluginIter *iter);

/**
  Returns the next registered mapping plugin or NULL if all plugins
  has been visited.

  Used for iterating over plugins.  Plugins should not be registered
  or removed while iterating.
 */
const DLiteMappingPlugin *
dlite_mapping_plugin_next(DLiteMappingPluginIter *iter);


/**
  Unloads and unregisters mapping plugin with the given name.
  Returns non-zero on error.
*/
int dlite_mapping_plugin_unload(const char *name);

/**
  Unloads and unregisters all mappings.  Returns non-zero on error.
*/
int dlite_mapping_plugin_unload_all(void);



/**
  Returns a pointer to the current mapping plugin search path.  It is
  initialised from the environment variable `DLITE_MAPPING_PLUGIN_DIRS`.

  Use dlite_mapping_plugin_path_insert(),
  dlite_mapping_plugin_path_append()
  and dlite_mapping_plugin_path_remove() to modify it.
*/
const char **dlite_mapping_plugin_paths(void);

/**
  Inserts `path` into the current search path at index `n`.  If `n` is
  negative, it counts from the end of the search path (like Python).

  If `n` is out of range, it is clipped.

  Returns non-zero on error.
*/
int dlite_mapping_plugin_path_insert(int n, const char *path);

/**
  Appends `path` into the current search path.

  Returns non-zero on error.
*/
int dlite_mapping_plugin_path_append(const char *path);

/**
  Like dlite_mapping_plugin_path_append(), but appends at most the
  first `n` bytes of `path` to the current search path.

  Returns non-zero on error.
*/
int dlite_mapping_plugin_path_appendn(const char *path, size_t n);

/**
  Removes path number `n` from current search path.

  Returns non-zero on error.
*/
int dlite_mapping_plugin_path_remove(int n);


/** @} */


/**
 * @name Signatures of functions defined by the plugins.
 * @{
 */


/**
  Returns a new instance obtained by mapping `instances`.
  Returns NULL on error.
 */
typedef DLiteInstance *(*Mapper)(const DLiteMappingPlugin *api,
                                 const DLiteInstance **instances, int n);

/**
  Releases internal resources associated with `api`.
 */
typedef void (*Freer)(DLiteMappingPlugin *api);

/** @} */



/**
  Struct with the name and pointers to function for a plugin. All
  plugins should define themselves by defining an intance of
  DLiteMappingPlugin.

  The cost of a mapping is an integer greater than (or equal to) zero.
  Mappings with low costs are preferred in front of mappings with high
  costs.  The default cost for a mapping is 20, while the cost for the
  trivial mapping to an existing input is zero.
*/
struct _DLiteMappingPlugin {
  PluginAPI_HEAD
  const char *   output_uri; /*!< Output metedata URI */
  int            ninput;     /*!< Number of inputs */
  const char **  input_uris; /*!< Array of input metedata URIs */
  Mapper         mapper;     /*!< Pointer to mapping function */
  int            cost;       /*!< Cost of this mapping. Default: 20 */
  void *         data;       /*!< Internal data used by the mapper */
};


#endif /* _DLITE_MAPPING_PLUGINS_H */
