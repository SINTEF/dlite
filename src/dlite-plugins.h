#ifndef _DLITE_PLUGINS_H
#define _DLITE_PLUGINS_H

/**
  @file
  @brief Common API for all plugins.
*/

#include "dlite-storage.h"


/** Initial segment of all DLiteStorage plugin data structures. */
#define DLiteStorage_HEAD                                               \
  struct _DLitePlugin *api; /*!< Pointer to plugin api */               \
  char *uri;                /*!< URI passed to dlite_storage_open() */  \
  char *options;            /*!< Options passed to dlite_storage_open() */ \
  int writable;             /*!< Whether storage is writable */ \
  DLiteIDFlag idflag;       /*!< How to handle instance id's */


/** Initial segment of all DLiteDataModel plugin data structures. */
#define DLiteDataModel_HEAD                                     \
  struct _DLitePlugin *api; /*!< Pointer to plugin api */       \
  DLiteStorage *s;          /*!< Pointer to storage */          \
  char uuid[37];            /*!< UUID for the stored data */


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


/*
  FIXME - plugins should be dynamically loadable

  See http://gernotklingler.com/blog/creating-using-shared-libraries-different-compilers-different-operating-systems/

  (sodyll)[https://github.com/petervaro/sodyll] provides a nice
  portable header.  Unfortunately it is GPL. Can we find something
  similar?
 */



/* Required api */
typedef DLiteStorage *(*Open)(const char *uri, const char *options);
typedef int (*Close)(DLiteStorage *s);

typedef DLiteDataModel *(*DataModel)(const DLiteStorage *s, const char *uuid);
typedef int (*DataModelFree)(DLiteDataModel *d);

typedef const char *(*GetMetaURI)(const DLiteDataModel *d);
typedef size_t (*GetDimensionSize)(const DLiteDataModel *d, const char *name);
typedef int (*GetProperty)(const DLiteDataModel *d, const char *name,
                           void *ptr, DLiteType type, size_t size,
                           size_t ndims, const size_t *dims);

/* Optional api */
typedef char **(*GetUUIDs)(const DLiteStorage *s);

typedef int (*SetMetaURI)(DLiteDataModel *d, const char *uri);
typedef int (*SetDimensionSize)(DLiteDataModel *d, const char *name,
                                size_t size);
typedef int (*SetProperty)(DLiteDataModel *d, const char *name,
                           const void *ptr, DLiteType type, size_t size,
                           size_t ndims, const size_t *dims);


typedef int (*HasDimension)(const DLiteDataModel *d, const char *name);
typedef int (*HasProperty)(const DLiteDataModel *d, const char *name);

typedef char *(*GetDataName)(const DLiteDataModel *d);
typedef int (*SetDataName)(DLiteDataModel *d, const char *name);

typedef DLiteEntity *(*GetEntity)(const DLiteStorage *s, const char *uuid);
typedef int (*SetEntity)(DLiteStorage *s, const DLiteEntity *e);


/** Struct with the name and pointers to function for a plugin. All
    plugins should define themselves by defining an intance of DLitePlugin. */
typedef struct _DLitePlugin {
  /* Name of plugin */
  const char *       name;             /* Name of plugin */

  /* Minimum api */
  Open               open;             /* Open storage */
  Close              close;            /* Close storage */

  DataModel          dataModel;        /* Creates new data model */
  DataModelFree      dataModelFree;    /* Frees a data model */

  GetMetaURI         getMetaURI;       /* Returns uri to metadata */
  GetDimensionSize   getDimensionSize; /* Returns size of dimension */
  GetProperty        getProperty;      /* Gets value of property */

  /* Optional api */
  GetUUIDs           getUUIDs;         /* Returns all UUIDs in storage */

  SetMetaURI         setMetaURI;       /* Sets metadata uri */
  SetDimensionSize   setDimensionSize; /* Sets size of dimension */
  SetProperty        setProperty;      /* Sets value of property */

  HasDimension       hasDimension;     /* Checks for dimension name */
  HasProperty        hasProperty;      /* Checks for property name */

  GetDataName        getDataName;      /* Returns name of instance */
  SetDataName        setDataName;      /* Assigns name to instance */

  GetEntity          getEntity;        /* Returns a new Entity from storage */
  SetEntity          setEntity;        /* Stores an Entity */
} DLitePlugin;


#endif /* _DLITE_PLUGINS_H */
