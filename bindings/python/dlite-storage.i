/* -*- C -*-  (not really, but good for syntax highlighting) */

//%{
//#include "dlite.h"
//#include "dlite-storage-plugins.h"
//%}

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


%rename(Storage) _DLiteStorage;
%inline %{
  struct _DLiteStorage {
    void *api;                /*!< Pointer to plugin api */
    char *uri;                /*!< URI passed to dlite_storage_open() */
    char *options;            /*!< Options passed to dlite_storage_open() */
    int writable;             /*!< Whether storage is writable */
    DLiteIDFlag idflag;       /*!< How to handle instance id's */
  };
 %}

%extend _DLiteStorage {
  _DLiteStorage(const char *driver, const char *uri, const char *options) {
    return dlite_storage_open(driver, uri, options);
  }
  _DLiteStorage(const char *url) {
    return dlite_storage_open_url(url);
  }
  ~_DLiteStorage(void) {
    dlite_storage_close($self);
  }

  //void set_idflag(enum _DLiteIDFlags idflag) {
  //  dlite_storage_set_idflag($self, idflag);
  //}

  char **get_uuids(void) {
    return dlite_storage_uuids($self);
  }
}

/* Dublicated declarations from dlite-storage-plugins.h */
int dlite_storage_plugin_unload(const char *name);
//const_char **dlite_storage_plugin_paths(void);
int dlite_storage_plugin_path_insert(int n, const char *path);
int dlite_storage_plugin_path_append(const char *path);
