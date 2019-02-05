/* dh5lite.c -- DLite plugin for hdf5 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

//#include <postgresql.h>

#include "config.h"

//#include "boolean.h"
#include "utils/err.h"

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-datamodel.h"


typedef struct {
  DLiteStorage_HEAD
} PGStorage;

typedef struct {
  DLiteDataModel_HEAD
} PGDataModel;



/*
  Opens `uri` and returns a newly created storage for it.

  The `options` argument provies additional input to the driver.
  Which options that are supported varies between the plugins.  It
  should be a valid URL query string of the form:

      key1=value1;key2=value2...

  An ampersand (&) may be used instead of the semicolon (;).

  Typical options supported by most drivers include:
  - mode : append | r | w
      Valid values are:
      - append   Append to existing file or create new file (default)
      - r        Open existing file for read-only
      - w        Truncate existing file or create new file

  Returns NULL on error.
 */
static DLiteStorage *open(const char *uri, const char *options)
{
  PGStorage *s;

  printf("*** open('%s', '%s')\n", uri, options);
  fflush(stdout);

  if (!( s= calloc(1, sizeof(PGStorage)))) return NULL;
  return (DLiteStorage *)s;
}


/*
  Closes storage `s`.  Returns non-zero on error.
 */
static int close(DLiteStorage *storage)
{
  UNUSED(storage);
  return 0;
}


/*
  Creates a new datamodel for storage `s`.

  If `uuid` exists in the root of `s`, the datamodel should describe the
  corresponding instance.

  Otherwise (if `s` is writable), a new instance described by the
  datamodel should be created in `s`.

  Returns the new datamodel or NULL on error.
 */
static DLiteDataModel *datamodel(const DLiteStorage *storage, const char *uuid)
{
  PGDataModel *d;

  UNUSED(storage);
  printf("*** datamodel(uuid='%s')\n", uuid);
  fflush(stdout);

  //if (!(d = calloc(1, sizeof(PGDataModel)))) return NULL;
  if (!(d = calloc(1, sizeof(PGDataModel)))) FAIL("allocation failure");
 fail:
  return (DLiteDataModel *)d;
}


/*
  Frees all memory associated with datamodel `d`.
 */
static int datamodel_free(DLiteDataModel *d)
{
  free(d);
  return 0;
}


/*
  Returns a pointer to newly malloc'ed metadata uri for datamodel `d`
  or NULL on error.
 */
static char *get_meta_uri(const DLiteDataModel *d)
{
  UNUSED(d);
  return strdup("meta.sintef.no/0.1/FakeEntity");
}


/*
  Returns the size of dimension `name` or 0 on error.
 */
static int get_dimension_size(const DLiteDataModel *d, const char *name)
{
  UNUSED(d);
  UNUSED(name);
  return 0;
}


/*
  Copies property `name` to memory pointed to by `ptr`.

  The expected type, size, number of dimensions and size of each
  dimension of the memory is described by `type`, `size`, `ndims` and
  `dims`, respectively.

  Returns non-zero on error.
 */
static int get_property(const DLiteDataModel *d, const char *name,
                        void *ptr, DLiteType type, size_t size,
                        size_t ndims, const size_t *dims)
{
  UNUSED(d);
  UNUSED(name);
  UNUSED(ptr);
  UNUSED(type);
  UNUSED(size);
  UNUSED(ndims);
  UNUSED(dims);
  return 1;
}






/*------------------------------------------------------------------*/

static DLiteStoragePlugin postgresql_plugin = {
  "postgresql",             /* name - name of storage plugin */

  open,                     /* Open - opens storage */
  close,                    /* Close - closes storage */

  datamodel,                /* DataModel - creates new datamodel */
  datamodel_free,           /* DataModelFree - frees data model */

  get_meta_uri,             /* GetMetaURI - returns uri to metadata */
  get_dimension_size,       /* GetDimensionSize - returns dimension size */
  get_property,             /* GetProperty - returns property value */

  /* optional */
  NULL,                     /* GetUUIDs -  */

  NULL,                     /* SetMetaURI */
  NULL,                     /* SetDimensionSize */
  NULL,                     /* SetProperty */

  NULL,                     /* HasDimension */
  NULL,                     /* HasPropety */

  NULL,                     /* GetDataName */
  NULL,                     /* SetDataName */

  NULL,                     /* GetEntity */
  NULL                      /* SetEntity */
};


DSL_EXPORT const DLiteStoragePlugin *
get_dlite_storage_plugin_api(const char *name)
{
  UNUSED(name);
  return &postgresql_plugin;
}
