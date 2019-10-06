/* -*- C -*-  (not really, but good for syntax highlighting) */

%{
#include "dlite.h"
#include "dlite-storage.h"
#include "dlite-storage-plugins.h"
%}

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

Call signatures
---------------
Storage(driver, uri, options)
Storage(url)

Parameters
----------
driver : string
    Name of driver used to connect to the storage.
uri : string
    The location to the storage.  For file storages, this is the file name.
    For web-based storages this is the location-part of the url.
options : string
    Additional options passed to the driver as a list of semicolon-separated
    ``key=value`` pairs.  Each driver may have their own options.  Some
    common options are:
      - mode={'append','r','w'}: 'append': append to existing storage or
        create a new one (hdf5,json).
      - compact={'yes','no'}: Whether to store in a compact format (json).
      - meta={'yes','no'}: Whether to format output as metadata (json).
url : string
    A combination of `driver`, `uri` and `options` in the form

        driver://uri?options
") _DLiteStorage;
%rename(Storage) _DLiteStorage;
//%{
//  struct _DLiteStorage {
//    void *api;                /*!< Pointer to plugin api */
//    char *uri;                /*!< URI passed to dlite_storage_open() */
//    char *options;            /*!< Options passed to dlite_storage_open() */
//    int writable;             /*!< Whether storage is writable */
//    DLiteIDFlag idflag;       /*!< How to handle instance id's */
//  };
//%}

struct _DLiteStorage {
  %immutable;
  char *uri;                /*!< URI passed to dlite_storage_open() */
  char *options;            /*!< Options passed to dlite_storage_open() */
  int writable;             /*!< Whether storage is writable */
  int idflag;               /*!< How to handle instance id's */
};

%extend _DLiteStorage {
  %feature("docstring", "") __init__;
  _DLiteStorage(const char *driver, const char *uri, const char *options) {
    return dlite_storage_open(driver, uri, options);
  }
  _DLiteStorage(const char *url) {
    return dlite_storage_open_url(url);
  }
  ~_DLiteStorage(void) {
    dlite_storage_close($self);
  }
  %feature("docstring", "Returns name of driver for this storage.") get_driver;
  const char *get_driver(void) {
    return dlite_storage_get_driver($self);
  }

  //void set_idflag(enum _DLiteIDFlags idflag) {
  //  dlite_storage_set_idflag($self, idflag);
  //}

  char **get_uuids(void) {
    return dlite_storage_uuids($self);
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
%feature("docstring", "") new_StoragePluginIter;
%extend StoragePluginIter {
  StoragePluginIter(void) {
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


/* Dublicated declarations from dlite-storage.h */
%rename(storage_paths_insert) dlite_storage_paths_insert;
%rename(storage_paths_append) dlite_storage_paths_append;
%rename(storage_paths_remove) dlite_storage_paths_remove;
%rename(storage_paths_get)    dlite_storage_paths_get;
int dlite_storage_paths_insert(int n, const char *path);
int dlite_storage_paths_append(const char *path);
int dlite_storage_paths_remove(int n);
const_char **dlite_storage_paths_get(void);


/* Dublicated declarations from dlite-storage-plugins.h */
%{
  int dlite_storage_plugin_path_remove(int n);
%}
%rename(storage_plugin_path_insert) dlite_storage_plugin_path_insert;
%rename(storage_plugin_path_append) dlite_storage_plugin_path_append;
%rename(storage_plugin_path_remove) dlite_storage_plugin_path_remove;
%rename(storage_plugin_paths)       dlite_storage_plugin_paths;
int dlite_storage_plugin_path_insert(int n, const char *path);
int dlite_storage_plugin_path_append(const char *path);
int dlite_storage_plugin_path_remove(int n);
const_char **dlite_storage_plugin_paths(void);

%rename(storage_plugin_unload) dlite_storage_plugin_unload;
int dlite_storage_plugin_unload(const char *name);


/* -----------------------------------
 * Target language-spesific extensions
 * ----------------------------------- */
#ifdef SWIGPYTHON
%include "dlite-storage-python.i"
#endif
