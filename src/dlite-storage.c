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

#define GLOBALS_ID "dlite-storage-id"
#define HOTLIST_CHUNK_LENGTH 8


/* Iterator over dlite storage paths. */
struct _DLiteStoragePathIter {
  FUIter *pathiter;
};

/* Hotlist of open storages for fast lookup of instances with
   dlite_instance_get(). */
typedef struct _DLiteStorageHotlist {
  size_t length;                  /* allocated length of `storages` */
  size_t nmemb;                   /* used length of `storages` */
  const DLiteStorage **storages;  /* array of hotlisted storages */
} DLiteStorageHotlist;


/* Global variables for dlite-storage */
typedef struct {
  FUPaths *storage_paths;
  DLiteStorageHotlist hotlist;
} Globals;


/* Frees global state for this module - called by atexit() */
static void free_globals(void *globals)
{
  Globals *g = globals;
  dlite_storage_paths_free();
  dlite_storage_hotlist_clear();
  free(g);
}

/* Return a pointer to global state for this module */
static Globals *get_globals(void)
{
  Globals *g = dlite_globals_get_state(GLOBALS_ID);
  if (!g) {
    if (!(g = calloc(1, sizeof(Globals))))
     FAILCODE(dliteMemoryError, "allocation failure");
    dlite_globals_add_state(GLOBALS_ID, g, free_globals);
  }
  return g;
 fail:
  if (g) free(g);
  return NULL;
}




/********************************************************************
 * Public api
 ********************************************************************/

/*
  Opens a storage located at `location` using `driver`.
  Returns a opaque pointer or NULL on error.

  The `options` are passed to the driver.
 */
DLiteStorage *dlite_storage_open(const char *driver, const char *location,
                                 const char *options)
{
  const DLiteStoragePlugin *api;
  DLiteStorage *s=NULL;

  if (!location) FAIL("missing location");
  if (!driver || !*driver) driver = fu_fileext(location);
  if (!driver || !*driver) FAIL("missing driver");
  if (!(api = dlite_storage_plugin_get(driver))) goto fail;
  if (!(s = api->open(api, location, options))) goto fail;
  s->api = api;
  if (!(s->location = strdup(location)))
   FAILCODE(dliteMemoryError, "allocation failure");
  if (options && !(s->options = strdup(options)))
   FAILCODE(dliteMemoryError, "allocation failure");

  map_init(&s->cache);

  if (s->flags & dliteReadable && s->flags& dliteGeneric)
    dlite_storage_hotlist_add(s);

  s->refcount = 1;
  return s;
 fail:
  if (s) free(s);
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
  int stat=0;
  assert(s);
  if (s->api->flush) stat = s->api->flush(s);

  /* Only free the storage when there are no more references to it. */
  if (--s->refcount > 0) return 0;

  if (s->flags & dliteReadable && s->flags & dliteGeneric)
    dlite_storage_hotlist_remove(s);

  stat |= s->api->close(s);
  free(s->location);
  if (s->options) free(s->options);
  map_deinit(&s->cache);
  free(s);
  return stat;
}

/*
   Flush storage `s`. Returns non-zero on error.
*/
int dlite_storage_flush(DLiteStorage *s)
{
  assert(s);
  if (s->api->flush) return s->api->flush(s);
  return err(dliteUnsupportedError, "storage does not support flush: %s",
             s->api->name);
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
  void *iter;
  if (!s->api->iterCreate)
    return errx(dliteUnsupportedError,
                "driver '%s' does not support iterCreate()",
                s->api->name), NULL;
  if ((iter = s->api->iterCreate(s, pattern)))
    s->refcount++;  // increase refcount on storage
  return iter;
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
    return errx(dliteUnsupportedError,
                "driver '%s' does not support iterNext()", s->api->name);
  return s->api->iterNext(iter, buf);
}

/*
  Free's iterator created with dlite_storage_iter_create().
 */
void dlite_storage_iter_free(DLiteStorage *s, void *iter)
{
  // Do not call iterFree() during atexit(), since it may lead to segfault
  if (!s->api->iterFree)
    errx(dliteUnsupportedError,
         "driver '%s' does not support iterFree()", s->api->name);
  else if (!dlite_globals_in_atexit() || getenv("DLITE_ATEXIT_FREE"))
    s->api->iterFree(iter);

  /* dlite_storage_close() decreases the refcount on `s` which was
     increased by dlite_storage_iter_create(). */
  dlite_storage_close(s);
}


/*
  Loads instance from storage `s` using the loadInstance api.
  Returns NULL on error or if loadInstance is not supported.
 */
DLiteInstance *dlite_storage_load(const DLiteStorage *s, const char *id)
{
  char uuid[DLITE_UUID_LENGTH+1];
  DLiteInstance **ptr, *inst=NULL;
  if (dlite_get_uuid(uuid, id) < 0) return NULL;
  if ((ptr = map_get(&(((DLiteStorage *)s)->cache), uuid))) return *ptr;

  if (s->api->loadInstance) {
    /* Add NULL to cache to mark that we are about to load the instance and
       break recursive calls */
    map_set(&(((DLiteStorage *)s)->cache), uuid, NULL);
    inst = s->api->loadInstance(s, id);
    map_set(&(((DLiteStorage *)s)->cache), uuid, inst);
  }
  return inst;
}


/*
  Delete instance from storage `s` using the deleteInstance api.
  Returns non-zero on error or if deleteInstance is not supported.
 */
int dlite_storage_delete(DLiteStorage *s, const char *id)
{
  if (s->api->deleteInstance) return s->api->deleteInstance(s, id);
  return err(dliteUnsupportedError, "storage does not support delete: %s",
             s->api->name);
}

/*
  Returns a malloc'ed string with plugin documentation or NULL on error.
 */
char *dlite_storage_help(DLiteStorage *s)
{
  if (s->api->help) return s->api->help(s->api);
  return err(dliteUnsupportedError, "storage does not support help: %s",
             s->api->name), NULL;
}


/*
  Returns the UUIDs off all instances in storage `s` whos metadata URI
  matches the glob pattern `pattern`.  If `pattern` is NULL, it matches
  all instances.

  The UUIDs are returned as a NULL-terminated array of string
  pointers.  The caller is responsible to free the returned array with
  dlite_storage_uuids_free().

  Not all plugins may implement this function.  In that case, NULL is
  returned.  NULL is also returned on error.
 */
char **dlite_storage_uuids(const DLiteStorage *s, const char *pattern)
{
  char **p = NULL;
  if (s->api->iterCreate && s->api->iterNext && s->api->iterFree) {
    char buf[DLITE_UUID_LENGTH+1];
    void *ptr, *iter = s->api->iterCreate(s, pattern);
    int n=0, len=0;

    if (!iter) return NULL;
    while (s->api->iterNext(iter, buf) == 0) {
      if (n >= len) {
        len += 32;
        if (!(ptr = realloc(p, len*sizeof(char *)))) {
          free(p);
          return err(dliteMemoryError, "allocation failure"), NULL;
        }
        p = ptr;
      }
      p[n++] = strdup(buf);
    }
    s->api->iterFree(iter);
    if (p) {
      if (!(ptr = realloc(p, (n+1)*sizeof(char *)))) {
        free(p);
        return err(dliteMemoryError, "allocation failure"), NULL;
      }
      p = ptr;
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
  return (s->flags & dliteWritable) ? 1 : 0;
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

/* Returns referance to storage paths */
FUPaths *dlite_storage_paths(void)
{
  Globals *g;
  if (!(g = get_globals())) return NULL;
  if (!g->storage_paths) {
    if (!(g->storage_paths = calloc(1, sizeof(FUPaths))))
      return err(dliteMemoryError, "allocation failure"), NULL;
    fu_paths_init_sep(g->storage_paths, "DLITE_STORAGES", "|");
    fu_paths_set_platform(g->storage_paths, dlite_get_platform());

    if (dlite_use_build_root()) {
      fu_paths_append(g->storage_paths, dlite_BUILD_ROOT
                      "/bindings/python/dlite/share/dlite/storages");
      fu_paths_extend(g->storage_paths, dlite_STORAGES, "|");
    } else {
      fu_paths_extend_prefix(g->storage_paths, dlite_pkg_root_get(),
                             DLITE_STORAGES, "|");
    }
  }
  return g->storage_paths;
}

/* Free's up and reset storage paths */
void dlite_storage_paths_free(void)
{
  Globals *g;
  if (!(g = get_globals())) return;
  if (g->storage_paths) {
    fu_paths_deinit(g->storage_paths);
    free(g->storage_paths);
  }
  g->storage_paths = NULL;
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
int dlite_storage_paths_remove_index(int index)
{
  FUPaths *paths = dlite_storage_paths();
  return fu_paths_remove_index(paths, index);
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
    return err(dliteMemoryError, "Allocation failure"), NULL;
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



/* Clears the storage hotlist. Returns non-zero on error. */
int dlite_storage_hotlist_clear()
{
  Globals *g;
  DLiteStorageHotlist *h;
  if (!(g = get_globals())) return -1;
  h = &g->hotlist;
  if (h->storages) free((void *)h->storages);
  memset(h, 0, sizeof(DLiteStorageHotlist));
  return 0;
}

/* Adds storage `s` to list of open storages for fast lookup of instances.
   Returns non-zero on error. */
int dlite_storage_hotlist_add(const DLiteStorage *s)
{
  Globals *g;
  DLiteStorageHotlist *h;
  assert(s);
  if (!(g = get_globals())) return -1;
  h = &g->hotlist;
  if (h->length <= h->nmemb) {
    size_t newlength = h->length + HOTLIST_CHUNK_LENGTH;
    const DLiteStorage **storages = realloc((DLiteStorage **)h->storages,
                                            newlength*sizeof(DLiteStorage *));
    if (!storages) return err(dliteMemoryError, "allocation failure");
    h->length = newlength;
    h->storages = storages;
  }
  assert(h->length > h->nmemb);
  h->storages[h->nmemb++] = s;
  return 0;
}

/* Remove storage `s` from hotlist.
   Returns zero on success.  One is returned if `s` is not in the hotlist.
   For other errors a negative number is returned. */
int dlite_storage_hotlist_remove(const DLiteStorage *s)
{
  Globals *g;
  DLiteStorageHotlist *h;
  int removed = -1;
  size_t i, length;
  assert(s);
  if (!(g = get_globals())) return -1;
  h = &g->hotlist;
  for (i=0; i < h->nmemb; i++) {
    if (h->storages[i] == s) {
      removed = i;
      if (i < h->nmemb-1) h->storages[i] = h->storages[h->nmemb-1];
      h->nmemb--;
      break;
    }
  }

  /* Keep a buffer of at least one chunk length when reducing allocated memory
     to avoid flickering. */
  length = (h->nmemb / HOTLIST_CHUNK_LENGTH + 2)*HOTLIST_CHUNK_LENGTH;
  assert(length > h->nmemb);
  if (h->length > length) {
    const DLiteStorage **storages = realloc((DLiteStorage **)h->storages,
                                            length);
    assert(storages);  // redusing allocated memory should always be successful
    h->length = length;
    h->storages = storages;
  }
  return (removed >= 0) ? 0 : 1;
}

/* Initialise hotlist iterator `iter`.
   Returns non-zero on error. */
int dlite_storage_hotlist_iter_init(DLiteStorageHotlistIter *iter)
{
  memset(iter, 0, sizeof(DLiteStorageHotlistIter));
  return 0;
}

/* Returns a pointer to next hotlisted storage or NULL if the iterator is
   exausted (or on other errors). */
const DLiteStorage *
dlite_storage_hotlist_iter_next(DLiteStorageHotlistIter *iter)
{
  Globals *g;
  DLiteStorageHotlist *h;
  if (!(g = get_globals())) return NULL;
  h = &g->hotlist;
  if (*(size_t *)iter >= h->nmemb) return NULL;
  return h->storages[(*(size_t *)iter)++];
}

/* Deinitialise hotlist iterator `iter`.
   Returns non-zero on error. */
int dlite_storage_hotlist_iter_deinit(DLiteStorageHotlistIter *iter)
{
  UNUSED(iter);
  return 0;
}
