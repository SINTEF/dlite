#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils/compat.h"
#include "utils/err.h"
#include "utils/fileutils.h"

#include "config-paths.h"

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-datamodel.h"
#include "dlite-storage-plugins.h"
#include "getuuid.h"


/** Iterator over dlite storage paths. */
struct _DLiteStoragePathIter {
  FUIter *pathiter;
};


/********************************************************************
 * Public api
 ********************************************************************/

/*
  Opens a storage located at `uri` using `driver`.
  Returns a opaque pointer or NULL on error.

  The `options` are passed to the driver.
 */
DLiteStorage *dlite_storage_open(const char *driver, const char *uri,
                                 const char *options)
{
  const DLiteStoragePlugin *api;
  DLiteStorage *storage=NULL;

  if (!uri) FAIL("missing uri");
  if (!driver || !*driver) driver = fu_fileext(uri);
  if (!driver || !*driver) FAIL("missing driver");
  if (!(api = dlite_storage_plugin_get(driver))) goto fail;
  if (!(storage = api->open(api, uri, options))) goto fail;
  storage->api = api;
  if (!(storage->uri = strdup(uri))) FAIL(NULL);
  if (options && !(storage->options = strdup(options))) FAIL(NULL);
  storage->idflag = dliteIDTranslateToUUID;

  return storage;
 fail:
  if (storage) free(storage);
  err_update_eval(dliteStorageOpenError);
  return NULL;
}


/*
  Like dlite_storage_open(), but takes as input an url of the form

      driver://location?options

  The question mark and options may be omitted.  If `location` refers
  to a file who's extension matches `driver`, the `driver://` part may
  also be omitted.

  Returns a new storage, or NULL on error.
*/
DLiteStorage *dlite_storage_open_url(const char *url)
{
  char *driver=NULL, *location=NULL, *options=NULL;
  DLiteStorage *s=NULL;
  char *p, *url2=strdup(url);
  if (dlite_split_url(url2, &driver, &location, &options, NULL)) goto fail;
  if (!driver && (p = strrchr(location, '.'))) driver = p+1;
  if (!driver) FAIL1("missing driver: %s", url);
  s = dlite_storage_open(driver, location, options);
 fail:
  free(url2);
  return s;
}


/*
   Closes storage `s`. Returns non-zero on error.
*/
int dlite_storage_close(DLiteStorage *s)
{
  int stat;
  assert(s);
  stat = s->api->close(s);
  free(s->uri);
  if (s->options) free(s->options);
  free(s);
  return stat;
}


/*
  Returns the current mode of how to handle instance IDs.
 */
DLiteIDFlag dlite_storage_get_idflag(const DLiteStorage *s)
{
  return s->idflag;
}

/*
  Sets how instance IDs are handled.
 */
void dlite_storage_set_idflag(DLiteStorage *s, DLiteIDFlag idflag)
{
  s->idflag = idflag;
}


/*
  Returns a new iterator over all instances in storage `s` who's metadata
  URI matches `pattern`.

  Returns NULL on error.
 */
void *dlite_storage_iter_create(DLiteStorage *s, const char *pattern)
{
  if (!s->api->iterCreate)
    return errx(1, "driver '%s' does not support iterCreate()",
                s->api->name), NULL;
  return s->api->iterCreate(s, pattern);
}

/*
  Writes the UUID to buffer pointed to by `buf` of the next instance
  in `iter`, where `iter` is an iterator created with
  dlite_storage_iter_create().

  Returns zero on success, 1 if there are no more UUIDs to iterate
  over and a negative number on other errors.
 */
int dlite_storage_iter_next(DLiteStorage *s, void *iter, char *buf)
{
  if (!s->api->iterNext)
    return errx(-1, "driver '%s' does not support iterNext()", s->api->name);
  return s->api->iterNext(iter, buf);
}

/*
  Free's iterator created with dlite_storage_iter_create().
 */
void dlite_storage_iter_free(DLiteStorage *s, void *iter)
{
  if (!s->api->iterFree)
    errx(1, "driver '%s' does not support iterFree()", s->api->name);
  else
    s->api->iterFree(iter);
}


/*
  Returns the UUIDs off all instances in storage `s` whos metadata URI
  matches the glob pattern `pattern`.  If `pattern` is NULL, it matches
  all instances.

  The UUIDs are returned as a NULL-terminated array of string
  pointers.  The caller is responsible to free the returned array with
  dlite_storage_uuids_free().

  Not all plugins may implement this function.  In that case, NULL is
  returned.
 */
char **dlite_storage_uuids(const DLiteStorage *s, const char *pattern)
{
  char **p = NULL;
  if (s->api->iterCreate && s->api->iterCreate && s->api->iterCreate) {
    char buf[DLITE_UUID_LENGTH+1];
    void *iter = s->api->iterCreate(s, pattern);
    int n=0, len=0;
    while (s->api->iterNext(iter, buf) == 0) {
      if (n >= len) {
        len += 32;
        if (!(p = realloc(p, len*sizeof(char *))))
          return err(1, "allocation failure"), NULL;
      }
      p[n++] = strdup(buf);
    }
    s->api->iterFree(iter);
    if (p) {
      p = realloc(p, (n+1)*sizeof(char *));
      p[n] = NULL;
    }
  } else if (s->api->getUUIDs)
    p = s->api->getUUIDs(s);
  else
    errx(1, "driver '%s' does not support getUUIDs()", s->api->name);
  return p;
}


/*
  Frees NULL-terminated array of instance names returned by
  dlite_storage_uuids().
*/
void dlite_storage_uuids_free(char **names)
{
  char **p;
  if (!names) return;
  for (p=names; *p; p++) free(*p);
  free(names);
}


/*
  Returns non-zero if storage `s` is writable.
 */
int dlite_storage_is_writable(const DLiteStorage *s)
{
  return s->writable;
}


/*
  Returns name of driver associated with storage `s`.
 */
const char *dlite_storage_get_driver(const DLiteStorage *s)
{
  return s->api->name;
}



/*******************************************************************
 *  Storage paths and URLs
 *******************************************************************/
static FUPaths *_storage_paths = NULL;

/* Returns referance to storage paths */
FUPaths *dlite_storage_paths(void)
{
  if (!_storage_paths) {
    if (!(_storage_paths = calloc(1, sizeof(FUPaths))))
      return err(1, "allocation failure"), NULL;
    fu_paths_init_sep(_storage_paths, "DLITE_STORAGES", "|");
    atexit(dlite_storage_paths_free);

    if (dlite_use_build_root())
      fu_paths_extend(_storage_paths, dlite_STORAGES, "|");
    else
      fu_paths_extend_prefix(_storage_paths, dlite_root_get(),
                             DLITE_STORAGES, "|");
  }
  return _storage_paths;
}

/* Free's up and reset storage paths */
void dlite_storage_paths_free(void)
{
  if (_storage_paths) {
    fu_paths_deinit(_storage_paths);
    free(_storage_paths);
  }
  _storage_paths = NULL;
}

/*
  Inserts `path` into storage paths before position `n`.  If `n` is
  negative, it counts from the end (like Python).

  Returns the index of the newly inserted element or -1 on error.
 */
int dlite_storage_paths_insert(int n, const char *path)
{
  FUPaths *paths = dlite_storage_paths();
  return fu_paths_insert(paths, path, n);
}

/*
  Appends `path` to storage paths.

  Returns the index of the newly inserted element or -1 on error.
 */
int dlite_storage_paths_append(const char *path)
{
  FUPaths *paths = dlite_storage_paths();
  return fu_paths_append(paths, path);
}

/*
  Removes path with index `n` from storage paths.  If `n` is negative, it
  counts from the end (like Python).

  Returns non-zero on error.
 */
int dlite_storage_paths_remove(int n)
{
  FUPaths *paths = dlite_storage_paths();
  return fu_paths_remove(paths, n);
}

/*
  Returns a NULL-terminated array of pointers to paths/urls or NULL if
  no storage paths have been assigned.

  The returned array is owned by DLite and should not be free'ed. It
  may be invalidated by further calls to dlite_storage_paths_insert()
  and dlite_storage_paths_append().
 */
const char **dlite_storage_paths_get()
{
  FUPaths *paths = dlite_storage_paths();
  return fu_paths_get(paths);
}

/*
  Returns an iterator over all files in storage paths (with glob
  patterns in paths expanded).

  Returns NULL on error.

  Should be used together with dlite_storage_path_iter_next() and
  dlite_storage_path_iter_stop().
 */
DLiteStoragePathIter *dlite_storage_paths_iter_start()
{
  DLiteStoragePathIter *iter=NULL;
  if (!(iter = calloc(1, sizeof(DLiteStoragePathIter))))
    return err(1, "Allocation failure"), NULL;
  if (!(iter->pathiter = fu_pathsiter_init(dlite_storage_paths(), NULL))) {
    free(iter);
    return err(1, "Failure initiating storage path iterator"), NULL;
  }
  return iter;
}

/*
  Returns name of the next file in the iterator `iter` created with
  dlite_storage_paths_iter_start() or NULL if there are no more matches.

  @note
  The returned string is owned by the iterator. It will be overwritten
  by the next call to fu_nextmatch() and should not be changed.  Use
  strdup() or strncpy() if a copy is needed.
 */
const char *dlite_storage_paths_iter_next(DLiteStoragePathIter *iter)
{
  return fu_pathsiter_next(iter->pathiter);
}

/*
  Stops and deallocates iterator created with dlite_storage_paths_iter_start().
 */
int dlite_storage_paths_iter_stop(DLiteStoragePathIter *iter)
{
  int stat;
  stat = fu_pathsiter_deinit(iter->pathiter);
  free(iter);
  return stat;
}
