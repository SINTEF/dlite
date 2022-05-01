#ifndef _DLITE_STORAGE_H
#define _DLITE_STORAGE_H

/**
  @file
  @brief Opens and closes storages.
*/

#include "utils/fileutils.h"


/** Opaque type for a DLiteStorage.

    Nothing is actually declared to be a DLiteStorage, but all plugin
    data structures can be cast to DLiteStorage. */
typedef struct _DLiteStorage DLiteStorage;

/** Opaque type for an instance. */
typedef struct _DLiteInstance DLiteInstance;

/** Iterator over dlite storage paths. */
typedef struct _DLiteStoragePathIter DLiteStoragePathIter;

/** Flags for how to handle instance IDs. */
typedef enum _DLiteIDFlag {
  dliteIDTranslateToUUID=0, /*!< Translate id's that are not a valid UUID to
                                 a (version 5) UUID (default). */
  dliteIDRequireUUID=1,     /*!< Require that `id` is a valid UUID. */
  dliteIDKeepID=2           /*!< Store data under the given id, even if it
                                 is not a valid UUID.  Not SOFT compatible,
                                 but may be useful for input files. */
} DLiteIDFlag;


/**
  Opens a storage located at `location` using `driver`.
  Returns a opaque pointer or NULL on error.

  The `options` are passed to the driver.  Options for known
  drivers are:

    * hdf5
        - rw   Read and write: open existing file or create new file (default)
        - r    Read-only: open existing file for read-only
        - w    Write: truncate existing file or create new file
        - a    Append: open existing file for read and write
*/
DLiteStorage *dlite_storage_open(const char *driver, const char *location,
                                 const char *options);

/**
  Like dlite_storage_open(), but takes as input an url of the form
  ``driver://location?options``.  The question mark and options may be left out.

  Returns a new storage, or NULL on error.
*/
DLiteStorage *dlite_storage_open_url(const char *url);


/**
  Closes storage `s`. Returns non-zero on error.
*/
int dlite_storage_close(DLiteStorage *s);



/**
  Returns the current mode of how to handle instance IDs.
 */
DLiteIDFlag dlite_storage_get_idflag(const DLiteStorage *s);

/**
  Sets how instance IDs are handled.
 */
void dlite_storage_set_idflag(DLiteStorage *s, DLiteIDFlag idflag);

/**
  Returns non-zero if storage `s` is writable.
 */
int dlite_storage_is_writable(const DLiteStorage *s);

/**
  Returns name of driver associated with storage `s`.
 */
const char *dlite_storage_get_driver(const DLiteStorage *s);


/* Dublicated declarations from dlite-storage-plugins.h */
int dlite_storage_plugin_unload(const char *name);
const char **dlite_storage_plugin_paths(void);
int dlite_storage_plugin_path_insert(int n, const char *path);
int dlite_storage_plugin_path_append(const char *path);

/**
 * @name Querying content of a storage
 * @{
 */

/**
  Returns a new iterator over all instances in storage `s` who's metadata
  URI matches `pattern`.

  Returns NULL on error.
 */
void *dlite_storage_iter_create(DLiteStorage *s, const char *pattern);

/**
  Writes the UUID to buffer pointed to by `buf` of the next instance
  in `iter`, where `iter` is an iterator created with
  dlite_storage_iter_create().

  Returns zero on success, 1 if there are no more UUIDs to iterate
  over and a negative number on other errors.
 */
int dlite_storage_iter_next(DLiteStorage *s, void *iter, char *buf);

/**
  Free's iterator created with dlite_storage_iter_create().
 */
void dlite_storage_iter_free(DLiteStorage *s, void *iter);



/**
  Returns the UUIDs off all instances in storage `s` whos metadata URI
  matches the glob pattern `pattern`.  If `pattern` is NULL, it matches
  all instances.

  The UUIDs are returned as a NULL-terminated array of string
  pointers.  The caller is responsible to free the returned array with
  dlite_storage_uuids_free().

  Not all plugins may implement this function.  In that case, NULL is
  returned.
 */
char **dlite_storage_uuids(const DLiteStorage *s, const char *pattern);


/**
  Frees NULL-terminated array of instance names returned by
  dlite_storage_uuids().
 */
void dlite_storage_uuids_free(char **uuids);

/** @} */



/**
 * @name Storage paths
 * @{
 */

/**
  Returns pointer to storage paths.a
*/
FUPaths *dlite_storage_paths(void);

/**
  Free's up memory used by storage paths.
*/
void dlite_storage_paths_free(void);

/**
  Inserts `path` into storage paths before position `n`.  If `n` is
  negative, it counts from the end (like Python).

  Returns the index of the newly inserted element or -1 on error.
 */
int dlite_storage_paths_insert(int n, const char *path);

/**
  Appends `path` to storage paths.

  Returns the index of the newly inserted element or -1 on error.
 */
int dlite_storage_paths_append(const char *path);

/**
  Removes path with index `n` from storage paths.  If `n` is negative, it
  counts from the end (like Python).

  Returns non-zero on error.
 */
int dlite_storage_paths_remove_index(int index);

/**
  Returns a NULL-terminated array of pointers to paths/urls or NULL if
  no storage paths have been assigned.

  The returned array is owned by DLite and should not be free'ed. It
  may be invalidated by further calls to dlite_storage_paths_insert()
  and dlite_storage_paths_append().
 */
const char **dlite_storage_paths_get();


/**
  Returns an iterator over all files in storage paths (with glob
  patterns in paths expanded).

  Returns NULL on error.

  Should be used together with dlite_storage_path_iter_next() and
  dlite_storage_path_iter_stop().
 */
DLiteStoragePathIter *dlite_storage_paths_iter_start();

/**
  Returns name of the next file in the iterator `iter` created with
  dlite_storage_paths_iter_start() or NULL if there are no more matches.

  @note
  The returned string is owned by the iterator. It will be overwritten
  by the next call to fu_nextmatch() and should not be changed.  Use
  strdup() or strncpy() if a copy is needed.
 */
const char *dlite_storage_paths_iter_next(DLiteStoragePathIter *iter);

/**
  Stops and deallocates iterator created with dlite_storage_paths_iter_start().
 */
int dlite_storage_paths_iter_stop(DLiteStoragePathIter *iter);


/** @} */

#endif /* _DLITE_STORAGE_H */
