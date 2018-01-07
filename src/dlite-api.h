/* dlite-api.h -- common API for all backends */
#ifndef _DLITE_API_H
#define _DLITE_API_H

#include "dlite.h"

/*
  FIXME: Use https://github.com/petervaro/sodyll to support for dynamic
  loadable plugins in a simple and portable way.
 */


/** Initial segment of all DLite backend data structures */
#define DLite_HEAD                                                           \
  /** Fields managed by dlite */                                             \
  struct _API *api; /*!< Pointer to backend api */                           \
  char uuid[37];    /*!< stored reference to uuid */                         \
  char *uri;        /*!< stored reference to uri, allocated at creation */   \
  char *metadata;   /*!< stored reference to metadata, allocated lazily */


/* Minimum api */
typedef DLite *(*Open)(const char *uri, const char *options, const char *id);
typedef int (*Close)(DLite *d);

typedef const char *(*GetMetadata)(const DLite *d);
typedef int (*GetDimensionSize)(const DLite *d, const char *name);
typedef int (*GetProperty)(const DLite *d, const char *name, void *ptr,
                           DLiteType type, size_t size,
                           int ndims, const int *dims);

/* Optional api */
typedef int (*SetMetadata)(DLite *d, const char *metadata);
typedef int (*SetDimensionSize)(DLite *d, const char *name, int size);
typedef int (*SetProperty)(DLite *d, const char *name, const void *ptr,
                           DLiteType type, size_t size,
                           int ndims, const int *dims);

typedef char **(*GetInstanceNames)(const char *uri, const char *options);

typedef int (*HasDimension)(DLite *d, const char *name);
typedef int (*HasProperty)(DLite *d, const char *name);

typedef char *(*GetDataName)(DLite *d);
typedef int (*SetDataName)(DLite *d, const char *name);

typedef int (*IsReadOnly)(DLite *d);

/** Struct with the name and pointers to function for a backend. All
    backends should define themselves by defining an intance of API. */
typedef struct _API {
  /* Name of API */
  const char *             name;

  /* Minimum api */
  Open                     open;
  Close                    close;
  GetMetadata              getMetadata;
  GetDimensionSize         getDimensionSize;
  GetProperty              getProperty;

  /* Optional api */
  SetMetadata              setMetadata;
  SetDimensionSize         setDimensionSize;
  SetProperty              setProperty;

  GetInstanceNames         getInstanceNames;

  HasDimension             hasDimension;
  HasProperty              hasProperty;

  GetDataName              getDataName;
  SetDataName              setDataName;

  IsReadOnly               isReadOnly;
} API;



/**
 *  Utility functions intended to be used by the backends
 */


/** Initialises DLite instance. */
void dlite_init(DLite *d);

/** Copies data from nested pointer to pointers array \a src to the
    flat continuous C-ordered array \a dst. The size of dest must be
    sufficient large.  Returns non-zero on error. */
int dcopy_to_flat(void *dst, const void *src, size_t size, int ndims,
                  const int *dims);

/** Copies data from flat continuous C-ordered array \a dst to nested
    pointer to pointers array \a src. The size of dest must be
    sufficient large.  Returns non-zero on error. */
int dcopy_to_nested(void *dst, const void *src, size_t size, int ndims,
                    const int *dims);


#endif /* _DLITE_API_H */
