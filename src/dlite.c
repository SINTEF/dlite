#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compat.h"
#include "dlite.h"
#include "dlite-api.h"
#include "getuuid.h"
#include "err.h"


/* Convenient macros for failing */
#define FAIL(msg) do { err(1, msg); goto fail; } while (0)
#define FAIL1(msg, a1) do { err(1, msg, a1); goto fail; } while (0)


/* Nothing is actually declared to be a DLite struct, but all api
   data structures can be cast to DLite. */
struct _DLite {
  DLite_HEAD
};


/* NULL-terminated array of all backends */
extern API h5_api;
API *api_list[] = {
  &h5_api,
  NULL
};


/* Returns a pointer to API for driver or NULL on error. */
static API *get_api(const char *driver)
{
  API *api;
  for (api=api_list[0]; api; api++)
    if (strcmp(api->name, driver) == 0)
      break;
  if (!api) errx(1, "invalid driver: '%s'", driver);
  return api;
}


/********************************************************************
 * Utility functions
 ********************************************************************/

/* Returns descriptive name for `type` or NULL on error. */
char *dget_typename(DLiteType type)
{
  char *types[] = {
    "blob",
    "boolean",
    "integer",
    "unsigned_integer",
    "float",
    "string",
    "string_pointer"
  };
  if (type < 0 || type >= sizeof(types) / sizeof(char *))
    return errx(1, "invalid type number: %d", type), NULL;
  return types[type];
}


/********************************************************************
 * Required api
 ********************************************************************/

/*
  Opens data item `id` from `uri` using `driver`.
  Returns a opaque pointer to a data handle or NULL on error.

  The `options` are passed to the driver.
 */
DLite *dopen(const char *driver, const char *uri, const char *options,
              const char *id)
{
  API *api;
  DLite *d=NULL;
  char uuid[UUID_LEN + 1];

  if (getuuid(uuid, id)) FAIL1("failed generating UUID from id \"%s\"", id);
  if (!(api = get_api(driver))) goto fail;
  if (!(d = api->open(uri, options, uuid))) goto fail;

  d->api = api;
  strncpy(d->uuid, uuid, sizeof(d->uuid));
  if (!(d->uri = strdup(uri))) FAIL(NULL);
  if (id && strcmp(uuid, id) && api->setDataName && api->setDataName(d, id))
    goto fail;

  return d;
 fail:
  if (d) free(d);
  return NULL;
}


/*
   Closes data handle `d`. Returns non-zero on error.
*/
int dclose(DLite *d)
{
  int i, stat;
  assert(d);
  stat = d->api->close(d);

  free(d->uri);
  if (d->metadata) free(d->metadata);
  if (d->dims) free(d->dims);
  if (d->dimnames) {
    for (i=0; i<(int)d->ndims; i++) free(d->dimnames[i]);
    free(d->dimnames);
  }
  if (d->propnames) {
    for (i=0; i<(int)d->nprops; i++) free(d->propnames[i]);
    free(d->propnames);
  }
  free(d);

  return stat;
}


/*
  Returns pointer to metadata or NULL on error. Do not free.
 */
const char *dget_metadata(const DLite *d)
{
  if (!d->metadata)
    ((DLite *)d)->metadata = (char *)d->api->getMetadata(d);
  return d->metadata;
}


/*
  Returns the size of dimension `name` or -1 on error.
 */
int dget_dimension_size(const DLite *d, const char *name)
{
  int i;
  if (d->ndims >= 0 && d->dimnames && d->dims)
    for (i=0; i<d->ndims; i++)
      if (strcmp(d->dimnames[i], name) == 0) return d->dims[i];
  return d->api->getDimensionSize(d, name);
}


/*
  Copies property `name` to memory pointed to by `ptr`.  Max `size`
  bytes are written.  If `size` is too small (or `ptr` is NULL),
  nothing is copied to `ptr`.

  Returns the size of data to write to `ptr`.  On error, -1 is
  returned.
 */
int dget_property(const DLite *d, const char *name, void *ptr,
                  DLiteType type, size_t size, int ndims, const int *dims)
{
  return d->api->getProperty(d, name, ptr, type, size, ndims, dims);
}


/*
  Sets property `name` to the memory (of `size` bytes) pointed to by `value`.
  Returns non-zero on error.
*/
int dset_property(DLite *d, const char *name, const void *ptr,
                  DLiteType type, size_t size, int ndims, const int *dims)
{
  return d->api->setProperty(d, name, ptr, type, size, ndims, dims);
}


/*
  Sets metadata.  Returns non-zero on error.
 */
int dset_metadata(DLite *d, const char *metadata)
{
  if (d->api->setMetadata) d->api->setMetadata(d, metadata);
  if (d->metadata) free(d->metadata);
  if (!(d->metadata = strdup(metadata))) return err(1, NULL);
  return 0;
}


/*
  Sets size of dimension `name`.  Returns non-zero on error.
*/
int dset_dimension_size(DLite *d, const char *name, int size)
{
  int i, stat;
  if (d->api->setDimensionSize &&
      (stat = d->api->setDimensionSize(d, name, size))) return stat;
  if (d->ndims >= 0 && d->dimnames && d->dims) {
    for (i=0; i<d->ndims; i++)
      if (strcmp(d->dimnames[i], name) == 0) {
        d->dims[i] = size;
        return 0;
      }
    return errx(1, "invalid dimension name: '%s'", name);
  }
  return 0;
}

/********************************************************************
 * Optional api
 ********************************************************************/


/*
  Returns a NULL-terminated array of string pointers to instance UUID's.
  The caller is responsible to free the returned array.
 */
char **dget_instance_names(const char *driver, const char *uri,
                           const char *options)
{
  API *api;
  if (!(api = get_api(driver))) return NULL;
  if (!api->getInstanceNames)
    return errx(1, "driver '%s' does not support "
                "getInstanceNames()", driver), NULL;
  return api->getInstanceNames(uri, options);
}


/*
  Frees NULL-terminated array of instance names returned by
  dget_instance_names().
*/
void dfree_instance_names(char **names)
{
  char **p;
  if (!names) return;
  for (p=names; *p; p++) free(*p);
  free(names);
}


/*
  If the uuid was generated from a unique name, return a pointer to a
  newly malloc'ed string with this name.  Otherwise NULL is returned.
*/
char *dget_dataname(DLite *d)
{
  if (d->api->getDataName) return d->api->getDataName(d);
  return NULL;
}


/*
  Returns the number of dimensions or -1 on error.
 */
int dget_ndimensions(const DLite *d)
{
  if (d->ndims >= 0) return d->ndims;
  if (!d->api->getNDimensions)
    return errx(-1, "driver '%s' does not support getNDimensions()",
                d->api->name);
  if (d->api->getNDimensions) return d->api->getNDimensions(d);
  return -1;
}


/*
  Returns the name of dimension `n` or NULL on error.  Do not free.
 */
const char *dget_dimension_name(const DLite *d, int n)
{
  if (d->ndims >= 0) {
    if (n < 0 || n >= d->ndims)
      return errx(-1, "dimension index out of range: %d", n), NULL;
    if (d->dimnames) return d->dimnames[n];
  }
  if (!d->api->getDimensionName)
    return errx(-1, "driver '%s' does not support getDimensionName()",
                d->api->name), NULL;
  return d->api->getDimensionName(d, n);
}


/*
  Returns the size of dimension `n` or -1 on error.
 */
int dget_dimension_size_by_index(const DLite *d, int n)
{
  if (d->ndims >= 0) {
    if (n < 0 || n >= d->ndims)
      return errx(-1, "dimension index out of range: %d", n);
    if (d->dims) return d->dims[n];
  }
  if (!d->api->getDimensionSizeByIndex)
    return errx(-1, "driver '%s' does not support getDimensionSizeByIndex()",
                d->api->name);
  return d->api->getDimensionSizeByIndex(d, n);
}


/*
  Returns the number of properties or -1 on error.
 */
int dget_nproperties(const DLite *d)
{
  if (d->nprops >= 0) return d->nprops;
  if (!d->api->getNProperties)
    return errx(-1, "Driver '%s' does not support getNProperties()",
                d->api->name);
  return d->api->getNProperties(d);
}


/*
  Returns a pointer to property name or NULL on error.  Do not free.
 */
const char *dget_property_name(const DLite *d, int n)
{
  if (d->nprops >= 0) {
    if (n < 0 || n >= d->nprops)
      return errx(-1, "property index out of range: %d", n), NULL;
    if (d->propnames) return d->propnames[n];
  }
  if (!d->api->getPropertyName)
    return errx(-1, "Driver '%s' does not support getPropertyName()",
                d->api->name), NULL;
  return d->api->getPropertyName(d, n);
}


/*
  Copies property \a n to memory pointed to by \a ptr.  Max \a size
  bytes are written.  If \a size is too small (or \a ptr is NULL),
  nothing is copied to \a ptr.

  Returns the size of data to write to \a ptr.  On error, -1 is
  returned.
 */
int dget_property_by_index(const DLite *d, int n, void *ptr,
                           DLiteType type, size_t size, int ndims,
                           const int *dims)
{
  if (!d->api->getPropertyByIndex)
    return dget_property(d, dget_property_name(d, n), ptr,
                         type, size, ndims, dims);
  return d->api->getPropertyByIndex(d, n, ptr, type, size, ndims, dims);
}



/********************************************************************
 * Utility functions intended to be used by teh backends
 ********************************************************************/

/* Initialises DLite instance. */
void dlite_init(DLite *d)
{
  memset(d, 0, sizeof(DLite));
  d->ndims = -1;
  d->nprops = -1;
}


/* Copies data from nested pointer to pointers array \a src to the
   flat continuous C-ordered array \a dst. The size of dest must be
   sufficient large.  Returns non-zero on error. */
int dcopy_to_flat(void *dst, const void *src, size_t size, int ndims,
                  const int *dims)
{
  int i, n=0, ntot=1, *ind=NULL, retval=1;
  char *q=dst;
  void **p=(void **)src;

  if (!(ind = calloc(ndims, sizeof(int)))) FAIL("allocation failure");

  for (i=0; i<ndims-1; i++) p = p[ind[i]];
  for (i=0; i<ndims; i++) ntot *= (dims) ? dims[i] : 1;

  while (n++ < ntot) {
    memcpy(q, *p, size);
    p++;
    q += size;
    if (++ind[ndims-1] >= ((dims) ? dims[ndims-1] : 1)) {
      ind[ndims-1] = 0;
      for (i=ndims-2; i>=0; i--) {
        if (++ind[i] < ((dims) ? dims[i] : i))
          break;
        else
          ind[i] = 0;
      }
      for (i=0, p=(void **)src; i<ndims-1; i++) p = p[ind[i]];
    }
  }
  retval = 0;
 fail:
  if (ind) free(ind);
  return retval;
}


/* Copies data from flat continuous C-ordered array \a dst to nested
   pointer to pointers array \a src. The size of dest must be
   sufficient large.  Returns non-zero on error. */
int dcopy_to_nested(void *dst, const void *src, size_t size, int ndims,
                  const int *dims)
{
  int i, n=0, ntot=1, *ind=NULL, retval=1;
  const char *q=src;
  void **p=dst;

  if (!(ind = calloc(ndims, sizeof(int)))) FAIL("allocation failure");

  for (i=0; i<ndims-1; i++) p = p[ind[i]];
  for (i=0; i<ndims; i++) ntot *= (dims) ? dims[i] : 1;

  while (n++ < ntot) {
    memcpy(*p, q, size);
    p++;
    q += size;
    if (++ind[ndims-1] >= ((dims) ? dims[ndims-1] : 1)) {
      ind[ndims-1] = 0;
      for (i=ndims-2; i>=0; i--) {
        if (++ind[i] < ((dims) ? dims[i] : i))
          break;
        else
          ind[i] = 0;
      }
      for (i=0, p=(void **)src; i<ndims-1; i++) p = p[ind[i]];
    }
  }
  retval = 0;
 fail:
  if (ind) free(ind);
  return retval;
}
