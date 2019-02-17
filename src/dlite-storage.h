#ifndef _DLITE_STORAGE_H
#define _DLITE_STORAGE_H

/**
  @file
  @brief Opens and closes storages.
*/

/** Opaque type for a DLiteStorage.

    Nothing is actually declared to be a DLiteStorage, but all plugin
    data structures can be cast to DLiteStorage. */
typedef struct _DLiteStorage DLiteStorage;

/** Opaque type for an instance. */
typedef struct _DLiteInstance DLiteInstance;

/** Opaque type for an Entity. */
typedef struct _DLiteEntity DLiteEntity;

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
  Opens a storage located at `uri` using `driver`.
  Returns a opaque pointer or NULL on error.

  The `options` are passed to the driver.  Options for known
  drivers are:

    * hdf5
        - rw   Read and write: open existing file or create new file (default)
        - r    Read-only: open existing file for read-only
        - w    Write: truncate existing file or create new file
        - a    Append: open existing file for read and write
*/
DLiteStorage *dlite_storage_open(const char *driver, const char *uri,
                                 const char *options);

/**
  Like dlite_storage_open(), but takes as input an url of the form
  ``driver://uri?options``.  The question mark and options may be left out.

  Returns a new storage, or NULL on error.
*/
DLiteStorage *dlite_storage_open_url(const char *url);


/**
  Closes data handle `d`. Returns non-zero on error.
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
  Returns a NULL-terminated array of string pointers to instance UUID's.
  The caller is responsible to free the returned array with
  dlite_storage_uuids_free().

  Not all plugins may implement this function.  In that case, NULL is
  returned.
 */
char **dlite_storage_uuids(const DLiteStorage *s);


/**
  Frees NULL-terminated array of instance names returned by
  dlite_storage_uuids().
 */
void dlite_storage_uuids_free(char **uuids);


/**
   Returns non-zero if storage `s` is writable.
 */
int dlite_storage_is_writable(const DLiteStorage *s);


/* Dublicated declarations from dlite-storage-plugins.h */
int dlite_storage_plugin_unload(const char *name);
const char **dlite_storage_plugin_paths(void);
int dlite_storage_plugin_path_insert(int n, const char *path);
int dlite_storage_plugin_path_append(const char *path);

#endif /* _DLITE_STORAGE_H */
