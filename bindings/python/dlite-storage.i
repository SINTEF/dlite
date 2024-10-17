/* -*- C -*-  (not really, but good for syntax highlighting) */

%{
#include "dlite.h"
#include "dlite-errors.h"
#include "dlite-storage.h"
#include "dlite-storage-plugins.h"

char *_storage_plugin_help(const char *name) {
  const DLiteStoragePlugin *api = dlite_storage_plugin_get(name);
  if (!api) return NULL;
  if (api->help) return api->help(api);
  return dlite_err(dliteUnsupportedError,
                   "\"%s\" storage does not support help", name), NULL;
}

%}


/* Storage iterator */
%feature("docstring", "\
Iterates over instances in storage `s`.  If `pattern` is given, only
instances whos metadata URI matches `pattern` are returned.
") StorageIterator;
%inline %{
  struct StorageIterator {
    DLiteStorage *s;  /*!< Reference to storage. */
    void *state;      /*!< Internal state managed by the storage plugin. */
  };
%}
%extend StorageIterator {
  StorageIterator(struct _DLiteStorage *s, const char *pattern=NULL) {
    struct StorageIterator *iter;
    if (!(iter = calloc(1, sizeof(struct StorageIterator))))
      return dlite_err(1, "allocation failure"), NULL;
    iter->s = s;
    if (!(iter->state = dlite_storage_iter_create(s, pattern))) {
      free(iter);
      return NULL;
    }
    return iter;
  }
  ~StorageIterator(void) {
    dlite_storage_iter_free($self->s, $self->state);
    free($self);
  }

  %feature("docstring", "\
Returns next instance or None if exhausted.") next;
  %newobject next;
  struct _DLiteInstance *next(void) {
    char uuid[DLITE_UUID_LENGTH+1];
    if (dlite_storage_iter_next($self->s, $self->state, uuid) == 0) {
      DLiteInstance *inst = dlite_instance_load($self->s, uuid);
      // Why isn't the refcount already increased?
      if (inst) dlite_instance_incref(inst);
      return inst;
    }
    return NULL;
  }

  struct StorageIterator *__iter__(void) {
    return $self;
  }
}


/* Flags for how to handle instance IDs. */
%rename(IDFlags) _DLiteIDFlags;
%rename("%(strip:[dlite])s") "";
enum _DLiteIDFlag {
  dliteIDTranslateToUUID=0, /*!< Translate id's that are not a valid UUID to
                                 a (version 5) UUID (default). */
  dliteIDRequireUUID=1,     /*!< Require that `id` is a valid UUID. */
  dliteIDKeepID=2           /*!< Store data under the given id, even if it
                                 is not a valid UUID.  Not SOFT compatible,
                                 but may be useful for input files. */
};


/* Storage */
%feature("docstring", "\
Represents a data storage.

Arguments
---------
driver_or_url : string
    Name of driver used to connect to the storage or, if `location` is not
    given, the URL to the storage:

        driver://location?options

location : string
    The location to the storage. For file storages, this is the file name.
options : string
    Additional options passed to the driver as a list of semicolon-separated
    ``key=value`` pairs. See the documentation of the individual drivers to
    see which options they support.
") _DLiteStorage;
%rename(Storage) _DLiteStorage;

struct _DLiteStorage {
  %immutable;
  char *location;           /*!< Location passed to dlite_storage_open() */
  char *options;            /*!< Options passed to dlite_storage_open() */
  int flags;                /*!< Storage flags */
  int idflag;               /*!< How to handle instance id's */
};

%extend _DLiteStorage {
  _DLiteStorage(const char *driver_or_url, const char *location=NULL,
                const char *options=NULL) {
    if (!location)
      return dlite_storage_open_url(driver_or_url);
    else
      return dlite_storage_open(driver_or_url, location, options);
  }
  ~_DLiteStorage(void) {
    dlite_storage_close($self);
  }

  %feature("docstring", "Flush storage.") flush;
  void flush(void) {
    dlite_storage_flush($self);
  }

  %feature("docstring", "Delete instance with given `id`.") delete;
  void delete(const char *id) {
    dlite_storage_delete($self, id);
  }

  %feature("docstring", "Returns name of driver for this storage.") get_driver;
  const char *get_driver(void) {
    return dlite_storage_get_driver($self);
  }

  %feature("docstring", "Returns documentation for storage plugin.") help;
  %newobject help;
  char *help(void) {
    return dlite_storage_help($self);
  }

  //void set_idflag(enum _DLiteIDFlags idflag) {
  //  dlite_storage_set_idflag($self, idflag);
  //}

  %feature("docstring",
           "Returns a list of UUIDs of all instances in the storage whos "
           "metadata matches `pattern`. If `pattern` is None, all UUIDs "
           "will be returned.") get_uuids;
  char **get_uuids(const char *pattern=NULL) {
    return dlite_storage_uuids($self, pattern);
  }

  //StorageIterator *instances(const char *pattern=NULL) {
  //  return StorageIterator
  //}

  %feature("docstring",
           "Returns whether the storage is readable.") _get_readable;
  bool _get_readable(void) {
    return ($self->flags & dliteReadable) ? 1 : 0;
  }

  %feature("docstring", "Set storage readability.") _set_readable;
  void _set_readable(bool readable) {
    if (readable)
      $self->flags |= dliteReadable;
    else
      $self->flags &= ~dliteReadable;
  }

  %feature("docstring",
           "Returns whether the storage is writable.") _get_writable;
  bool _get_writable(void) {
    return ($self->flags & dliteWritable) ? 1 : 0;
  }

  %feature("docstring", "Set storage writability.") _set_writable;
  void _set_writable(bool writable) {
    if (writable)
      $self->flags |= dliteWritable;
    else
      $self->flags &= ~dliteWritable;
  }

  %feature("docstring", "Returns whether the storage is generic.") _get_generic;
  bool _get_generic(void) {
    return ($self->flags & dliteGeneric) ? 1 : 0;
  }

  %feature("docstring", "Set whether storage is generic") _set_generic;
  void _set_generic(bool generic) {
    if (generic)
      $self->flags |= dliteGeneric;
    else
      $self->flags &= ~dliteGeneric;
  }

}


/* Plugin iterator */
%feature("docstring", "\
Iterates over loaded storage plugins.
") StoragePluginIter;
%inline %{
  struct StoragePluginIter {
    DLiteStoragePluginIter *iter;
  };
%}
%extend StoragePluginIter {
  StoragePluginIter(void) {
    //dlite_storage_plugin_load_all();
    DLiteStoragePluginIter *iter = dlite_storage_plugin_iter_create();
    return (struct StoragePluginIter *)iter;
  }
  ~StoragePluginIter(void) {
    DLiteStoragePluginIter *iter = (DLiteStoragePluginIter *)$self;
    dlite_storage_plugin_iter_free(iter);
  }
  %feature("docstring", "Returns name of next plugin or None if exhausted.
  ") next;
  const char *next(void) {
    DLiteStoragePluginIter *iter = (DLiteStoragePluginIter *)$self;
    const DLiteStoragePlugin *api = dlite_storage_plugin_iter_next(iter);
    return (api) ? api->name : NULL;
  }
  struct StoragePluginIter *__iter__(void) {
    return $self;
  }
}


%rename(_load_all_storage_plugins) dlite_storage_plugin_load_all;
int dlite_storage_plugin_load_all();

%rename(_unload_storage_plugin) dlite_storage_plugin_unload;
int dlite_storage_plugin_unload(const char *name);

char *_storage_plugin_help(const char *name);


/* -----------------------------------
 * Target language-spesific extensions
 * ----------------------------------- */
#ifdef SWIGPYTHON
%include "dlite-storage-python.i"
#endif
