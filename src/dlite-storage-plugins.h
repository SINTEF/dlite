#ifndef _DLITE_STORAGE_PLUGINS_H
#define _DLITE_STORAGE_PLUGINS_H

/**
  @file
  @brief Common API for all storage plugins (internal).

  A DLite storage plugin should be a shared library that defines the
  function

      const DLiteStoragePlugin *get_dlite_storage_api(const char *name);

  that returns a pointer to a struct with pointers to all functions
  provided by the plugin.  The `name` is just a hint that plugins are
  free to ignore.  It is used by storage plugins that supports several
  different drivers, to select which api that should be returned.

  The storage plugin search path is initialised from the environment
  variable `DLITE_STORAGE_PLUGIN_DIRS`.

  Two APIs
  --------
  Storage plugins may choose to implement either the datamodel API
  or the instance API.

  The datamodel API is the original API resembling SOFT5. It provides a
  generic abstract layer between the data representation in the storage
  and the DLite instances.

  Since DLite now offers an abstract datamodel-like API for all
  instances, the need for the datamodel layer seems to be gone.  DLite
  therefore offers an alternative and simpler API for storage plugins
  that works directly on DLite instances and only contains two
  functions; LoadInstance() and SaveInstance().
*/
#include "utils/dsl.h"
#include "utils/fileutils.h"
#include "utils/plugin.h"

#include "dlite-datamodel.h"
#include "dlite-storage.h"
#include "dlite-entity.h"

/** A struct with function pointers to all functions provided by a plugin. */
typedef struct _DLiteStoragePlugin     DLiteStoragePlugin;
typedef struct _DLiteStoragePluginIter DLiteStoragePluginIter;

/** Initial segment of all DLiteStorage plugin data structures. */
#define DLiteStorage_HEAD                                                  \
  const DLiteStoragePlugin *api;  /*!< Pointer to plugin api */            \
  char *location;           /*!< Location passed to dlite_storage_open() */ \
  char *options;            /*!< Options passed to dlite_storage_open() */ \
  int writable;             /*!< Whether storage is writable */            \
  DLiteIDFlag idflag;       /*!< How to handle instance id's */


/** Initial segment of all DLiteDataModel plugin data structures. */
#define DLiteDataModel_HEAD                                        \
  const DLiteStoragePlugin *api;  /*!< Pointer to plugin api */    \
  DLiteStorage *s;             /*!< Pointer to storage */          \
  char uuid[37];               /*!< UUID for the stored data */


/** Base definition of a DLite storage, that all plugin storage
    objects can be cast to.  Is never actually instansiated. */
struct _DLiteStorage {
  DLiteStorage_HEAD
};

/** Base definition of a DLite data model, that all plugin data model
    objects can be cast to.  Is never actually instansiated. */
struct _DLiteDataModel {
  DLiteDataModel_HEAD
};


/**
  @name Plugin frontend
  @{
 */

/**
  Returns a pointer to a DLiteStoragePlugin or NULL on error.

  The `iter` argument is normally ignored.  It is provided to support
  plugins exposing several APIs.  If the plugin has more APIs to
  expose, it should increase the integer pointed to by `iter` by one.
 */
typedef const DLiteStoragePlugin *(*GetDLiteStorageAPI)(DLiteGlobals *g,
                                                        int *iter);


/**
  Returns a storage plugin with the given name, or NULL if it cannot
  be found.

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
const DLiteStoragePlugin *dlite_storage_plugin_get(const char *name);

/**
  Load all plugins that can be found in the plugin search path.
 */
int dlite_storage_plugin_load_all();

/**
  Unloads and unregisters all storage plugins.
*/
void dlite_storage_plugin_unload_all();

/**
  Returns a pointer to a new plugin iterator or NULL on error.  It
  should be free'ed with dlite_storage_plugin_iter_free().
 */
DLiteStoragePluginIter *dlite_storage_plugin_iter_create();

/**
  Returns pointer the next plugin or NULL if there a re no more plugins.
  `iter` is the iterator returned by dlite_storage_plugin_iter_create().
 */
const DLiteStoragePlugin *
dlite_storage_plugin_iter_next(DLiteStoragePluginIter *iter);

/**
  Frees plugin iterator `iter` created with
  dlite_storage_plugin_iter_create().
 */
void dlite_storage_plugin_iter_free(DLiteStoragePluginIter *iter);


/**
  Unloads and unregisters storage plugin with the given name.
  Returns non-zero on error.
*/
int dlite_storage_plugin_unload(const char *name);

/**
  Returns a pointer to the underlying FUPaths object for storage plugins
  or NULL on error.
 */
FUPaths *dlite_storage_plugin_paths_get(void);

/**
  Returns a pointer to the current storage plugin search path.  It is
  initialised from the environment variable `DLITE_STORAGE_PLUGIN_DIRS`.

  Use dlite_storage_plugin_path_insert(), dlite_storage_plugin_path_append()
  and dlite_storage_plugin_path_remove() to modify it.
*/
const char **dlite_storage_plugin_paths(void);

/**
  Returns an allocated string with the content of `paths` formatted
  according to the current platform.  See dlite_set_platform().

  Returns NULL on error.
 */
char *dlite_storage_plugin_path_string(void);

/**
  Inserts `path` into the current search path at index `n`.  If `n` is
  negative, it counts from the end of the search path (like Python).

  If `n` is out of range, it is clipped.

  Returns non-zero on error.
*/
int dlite_storage_plugin_path_insert(int n, const char *path);

/**
  Appends `path` into the current search path.

  Returns the index of the newly appended element or -1 on error.
*/
int dlite_storage_plugin_path_append(const char *path);

/**
  Like dlite_storage_plugin_path_append(), but appends at most the
  first `n` bytes of `path` to the current search path.

  Returns the index of the newly appended element or -1 on error.
*/
int dlite_storage_plugin_path_appendn(const char *path, size_t n);

/**
  Removes path number `n` from current search path.

  Returns non-zero on error.
*/
int dlite_storage_plugin_path_remove_index(int index);

/**
  Removes path `path` from current search path.

  Returns non-zero if there is no such path.
*/
int dlite_storage_plugin_path_remove(const char *path);


/** @} */


/**
 * @name Basic API
 * Signatures of functions that must be defined by all plugins.
 * @{
 */

/**
  Opens `uri` and returns a newly created storage for it.

  The `api` argument can normally be ignored (it is needed for the
  Python storage backend).

  The `options` argument provies additional input to the driver.
  Which options that are supported varies between the plugins.  It
  should be a valid URL query string of the form:

      key1=value1;key2=value2...

  An ampersand (&) may be used instead of the semicolon (;).

  Typical options supported by most drivers include:
  - mode : append | r | w
      Valid values are:
      - append   Append to existing file or create new file (default)
      - r        Open existing file for read-only
      - w        Truncate existing file or create new file

  Returns NULL on error.
 */
typedef DLiteStorage *
(*Open)(const DLiteStoragePlugin *api, const char *uri, const char *options);

/**
  Closes storage `s`.  Returns non-zero on error.
 */
typedef int (*Close)(DLiteStorage *s);

/** @} */


/**
 * @name Queue API
 * Function for querying the content of a plugin.  The new IterCreate(),
 * IterNext() and IterFree() are preferred in front of the old GetUUIDs()
 * function.
 *
 * Any of these functions are fully optional for the storage plugin to
 * implement.
 * @{
 */

/**
  Returns a new iterator over all instances in storage `s` who's metadata
  URI matches `pattern`.

  Returns NULL on error.
 */
typedef void *(*IterCreate)(const DLiteStorage *s, const char *pattern);

/**
  Writes the uuid of the next instance to `buf`, where `iter` is an
  iterator returned by IterCreate().

  Returns zero on success, 1 if there are no more UUIDs to iterate
  over and a negative number on other errors.
 */
typedef int (*IterNext)(void *iter, char *buf);

/**
  Free's iterator created with IterCreate().
 */
typedef void (*IterFree)(void *iter);


/**
  Returns a newly malloc'ed NULL-terminated array of (malloc'ed)
  string pointers to the UUID's of all instances in storage `s`.

  The caller is responsible to free the returned array.

  Returns NULL on error.

  @deprecated Will most likely be deprecated in favour for
  IterCreate(), IterNext() and IterFree().
 */
typedef char **(*GetUUIDs)(const DLiteStorage *s);

/** @} */



/**
 * @name Instance API
 * New API for loading and saving instances that works direct on
 * instances themselves.
 *
 * Both LoadInstance() and SaveInstance() are optional for the plugin
 * to implement.  If one is not implemented, the old datamodel API will
 * be attempted.
 * @{
 */

/**
  Returns a new instance from `uuid` in storage `s`.  NULL is returned
  on error.
 */
typedef DLiteInstance *(*LoadInstance)(const DLiteStorage *s, const char *uuid);

/**
  Stores instance `inst` to storage `s`.  Returns non-zero on error.
 */
typedef int (*SaveInstance)(DLiteStorage *s, const DLiteInstance *inst);

/** @} */



/**
 * @name Datamodel API

 * Old datamodel-based API for loading instances from and saving them
 * to a storage.
 *
 * The functions in the datamodel API may be omitted if the corresponding
 * functions in the instance API are implemented.
 *
 * @deprecated Will most likely be deprecated in favour for the new
 * and simpler Instance API.
 * @{
 */

/**
  Creates a new datamodel for storage `s`.

  If `uuid` exists in the root of `s`, the datamodel should describe the
  corresponding instance.

  Otherwise (if `s` is writable), a new instance described by the
  datamodel should be created in `s`.

  Returns the new datamodel or NULL on error.
 */
typedef DLiteDataModel *(*DataModel)(const DLiteStorage *s, const char *uuid);


/**
  Frees all memory associated with datamodel `d`.
 */
typedef int (*DataModelFree)(DLiteDataModel *d);


/**
  Returns a pointer to newly malloc'ed metadata uri for datamodel `d`
  or NULL on error.
 */
typedef char *(*GetMetaURI)(const DLiteDataModel *d);

/**
 * Resolve the dimensions from the properties (JSON or YAML storage)
 */
typedef void (*ResolveDimensions)(DLiteDataModel *d, const DLiteMeta *meta);

/**
  Returns the size of dimension `name` or -1 on error.
 */
typedef int (*GetDimensionSize)(const DLiteDataModel *d, const char *name);


/**
  Copies property `name` to memory pointed to by `ptr`.

  The expected type, size, number of dimensions and size of each
  dimension of the memory is described by `type`, `size`, `ndims` and
  `dims`, respectively.

  Returns non-zero on error.
 */
typedef int (*GetProperty)(const DLiteDataModel *d, const char *name,
                           void *ptr, DLiteType type, size_t size,
                           size_t ndims, const size_t *dims);


/** @} */

/**
 * @name Optional part of the DataModel API
 * Signatures of function that are optional for the plugins to define.
 * @{
 */


/**
  Sets the metadata uri in datamodel `d` to `uri`.  Returns non-zero on error.
 */
typedef int (*SetMetaURI)(DLiteDataModel *d, const char *uri);


/**
  Sets the size of dimension `name`.  Returns non-zero on error.
 */
typedef int (*SetDimensionSize)(DLiteDataModel *d, const char *name,
                                size_t size);


/**
  Sets property `name` to the memory pointed to by `ptr`.

  The expected type, size, number of dimensions and size of each
  dimension of the memory is described by `type`, `size`, `ndims` and
  `dims`, respectively.

  Returns non-zero on error.
 */
typedef int (*SetProperty)(DLiteDataModel *d, const char *name,
                           const void *ptr, DLiteType type, size_t size,
                           size_t ndims, const size_t *dims);


/**
  Returns a positive value if dimension `name` is defined, zero if it
  isn't and a negative value on error.
 */
typedef int (*HasDimension)(const DLiteDataModel *d, const char *name);


/**
  Returns a positive value if property `name` is defined, zero if it
  isn't and a negative value on error.
 */
typedef int (*HasProperty)(const DLiteDataModel *d, const char *name);


/**
  If the uuid was generated from a unique name, return a pointer to a
  newly malloc'ed string with this name.  Otherwise NULL is returned.
*/
typedef char *(*GetDataName)(const DLiteDataModel *d);


/**
  Gives the instance a name.  This function should only be called
  if the uuid was generated from `name`.
  Returns non-zero on error.
*/
typedef int (*SetDataName)(DLiteDataModel *d, const char *name);

/** @} */


/**
 * @name Internal data
 * Internal data used by the driver.  Optional.
 * @{
 */

/**
  Releases internal resources associated with `api`.
*/
typedef void (*DriverFreer)(DLiteStoragePlugin *api);

/** @} */


/**
  Struct with the name and pointers to function for a plugin. All
  plugins should define themselves by defining an intance of
  DLiteStoragePlugin.
*/
struct _DLiteStoragePlugin {
  PluginAPI_HEAD

  /* Basic API (required) */
  Open               open;             /*!< Open storage */
  Close              close;            /*!< Close storage */

  /* Queue API */
  IterCreate         iterCreate;       /*!< Creates iterator over storage */
  IterNext           iterNext;         /*!< Returns next UUID */
  IterFree           iterFree;         /*!< Free's iterator */

  GetUUIDs           getUUIDs;         /*!< Returns all UUIDs in storage */

  /* Instance API */
  LoadInstance       loadInstance;     /*!< Returns new instance from storage */
  SaveInstance       saveInstance;     /*!< Stores an instance */

  /* DataModel API */
  DataModel          dataModel;        /*!< Creates new data model */
  DataModelFree      dataModelFree;    /*!< Frees a data model */

  GetMetaURI         getMetaURI;       /*!< Returns uri to metadata */
  ResolveDimensions  resolveDimensions;/*!< Resolves dimensions from properties */
  GetDimensionSize   getDimensionSize; /*!< Returns size of dimension */
  GetProperty        getProperty;      /*!< Gets value of property */

  /* ... (optional) */
  SetMetaURI         setMetaURI;       /*!< Sets metadata uri */
  SetDimensionSize   setDimensionSize; /*!< Sets size of dimension */
  SetProperty        setProperty;      /*!< Sets value of property */

  HasDimension       hasDimension;     /*!< Checks for dimension name */
  HasProperty        hasProperty;      /*!< Checks for property name */

  GetDataName        getDataName;      /*!< Returns name of instance */
  SetDataName        setDataName;      /*!< Assigns name to instance */

  /* Driver data */
  void *             data;             /*!< Internal data used by the driver */
};


#endif /* _DLITE_STORAGE_PLUGINS_H */
