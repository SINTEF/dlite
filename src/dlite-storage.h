#ifndef _DLITE_STORAGE_H
#define _DLITE_STORAGE_H


/**
  @file
*/


/** Opaque type for a DLiteStorage.

    Nothing is actually declared to be a DLiteStorage, but all plugin
    data structures can be cast to DLiteStorage. */
typedef struct _DLiteStorage DLiteStorage;



/**
  Opens data item \a id from \a uri using \a driver.
  Returns a opaque pointer to a data handle or NULL on error.

  The \a options are passed to the driver.  Options for known
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
  Closes data handle \a d. Returns non-zero on error.
*/
int dlite_storage_close(DLiteStorage *s);


/**
  Returns a NULL-terminated array of string pointers to instance UUID's.
  The caller is responsible to free the returned array with
  dlite_storage_free_uuids().

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
   Returns non-zero if storage \a s is writable.
 */
int dlite_storage_is_writable(const DLiteStorage *s);


#endif /* _DLITE_STORAGE_H */
