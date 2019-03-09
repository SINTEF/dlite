#ifndef _DLITE_TRANSLATOR_PLUGINS_H
#define _DLITE_TRANSLATOR_PLUGINS_H

/**
  @file
  @brief Common API for all translator plugins (internal).

  A DPite translator plugin should be a shared library that defines the
  function

      const DLiteTranslatorPlugin *
      get_dlite_translator_api(const char *name);

  that returns a pointer to a struct with pointers to all functions
  provided by the plugin.  The `name` is just a hint that plugins are
  free to ignore.  It is used by translator plugins that supports
  several different drivers, to select which api that should be
  returned.

  The translator plugin search path is initialised from the environment
  variable `DLITE_TRANSLATOR_PLUGINS`.
*/
#include "utils/dsl.h"
#include "utils/fileutils.h"

//#include "dlite-translator.h"
#include "dlite-entity.h"

/**
  A struct with data and function pointers provided by a plugin.
 */
typedef struct _DLiteTranslatorPlugin  DLiteTranslatorPlugin;


/**
  Prototype for function returning a pointer to a
  DLiteTranslatorPlugin or NULL on error.

  The `name` is just a hint that plugins are free to ignore.  It is
  used by translator plugins that supports several different
  translators to select which api that should be returned.
 */
typedef const DLiteTranslatorPlugin *
(*GetDLiteTranslatorAPI)(const char *name);


/**
  @name Plugin frontend
  @{
 */

/**
  Returns a translator plugin with the given name, or NULL if it
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
const DLiteTranslatorPlugin *dlite_translator_plugin_get(const char *name);

/**
  Registers `api` for a translator plugin.  Returns non-zero on error.
*/
int dlite_translator_plugin_register_api(const DLiteTranslatorPlugin *api);

/**
  Unloads and unregisters translator plugin with the given name.
  Returns non-zero on error.
*/
int dlite_translator_plugin_unload(const char *name);

/**
  Returns a pointer to the current translator plugin search path.  It is
  initialised from the environment variable `DLITE_TRANSLATOR_PLUGINS`.

  Use dlite_translator_plugin_path_insert(),
  dlite_translator_plugin_path_append()
  and dlite_translator_plugin_path_remove() to modify it.
*/
const char **dlite_translator_plugin_paths(void);

/**
  Inserts `path` into the current search path at index `n`.  If `n` is
  negative, it counts from the end of the search path (like Python).

  If `n` is out of range, it is clipped.

  Returns non-zero on error.
*/
int dlite_translator_plugin_path_insert(int n, const char *path);

/**
  Appends `path` into the current search path.

  Returns non-zero on error.
*/
int dlite_translator_plugin_path_append(const char *path);

/**
  Removes path number `n` from current search path.

  Returns non-zero on error.
*/
int dlite_translator_plugin_path_remove(int n);


/** @} */


/**
 * @name Signatures of functions defined by the plugins.
 * @{
 */


/**
  Returns a new instance obtained by translating `instances` using the
  translator `t`.  Returns NULL on error.
 */
typedef DLiteInstance *(Translate)(DLiteTranslator *t,
                                   DLiteInstance **instances);

/** @} */



/**
  Struct with the name and pointers to function for a plugin. All
  plugins should define themselves by defining an intance of
  DLiteTranslatorPlugin.
*/
struct _DLiteTranslatorPlugin {
  /* Name of plugin */
  const char *   name;       /*!< Name of plugin */
  const char *   output_uri; /*!< Output metedata URI */
  int            ninput;     /*!< Number of inputs */
  const char **  input_uris; /*!< Array of input metedata URIs */
  Translate      translate;  /*!< Pointer to translator function */
 };


#endif /* _DLITE_TRANSLATOR_PLUGINS_H */
