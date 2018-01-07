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


/*
  Writes an UUID to `buff` based on `id`.

  If `id` is NULL or empty, a new random version 4 UUID is generated.
  If `id` is an invalid UUID string, a new version 5 sha1-based UUID
  is generated from `id` using the DNS namespace.
  Otherwise `id` is copied to `buff`.

  Length of `buff` must at least 37 bytes (36 for UUID + NUL termination).

  Returns a pointer to `buff`, or NULL on error.
 */
char *dget_uuid(char *buff, const char *id)
{
  return (getuuid(buff, id)) ? NULL : buff;
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
  if (id && strcmp(uuid, id) && !dis_readonly(d) && api->setDataName &&
      api->setDataName(d, id)) goto fail;

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
  int stat;
  assert(d);
  stat = d->api->close(d);
  free(d->uri);
  if (d->metadata) free(d->metadata);
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
  return d->api->getDimensionSize(d, name);
}


/*
  Copies property `name` to memory pointed to by `ptr`.  Max `size`
  bytes are written.  If `size` is too small (or `ptr` is NULL),
  nothing is copied to `ptr`.

  Returns non-zero on error.
 */
int dget_property(const DLite *d, const char *name, void *ptr,
                  DLiteType type, size_t size, int ndims, const int *dims)
{
  return d->api->getProperty(d, name, ptr, type, size, ndims, dims);
}


/********************************************************************
 * Optional api
 ********************************************************************/

/*
  Sets property `name` to the memory (of `size` bytes) pointed to by `value`.
  Returns non-zero on error.
*/
int dset_property(DLite *d, const char *name, const void *ptr,
                  DLiteType type, size_t size, int ndims, const int *dims)
{
  if (!d->api->setProperty)
    return errx(1, "driver '%s' does not support set_property", d->api->name);
  return d->api->setProperty(d, name, ptr, type, size, ndims, dims);
}


/*
  Sets metadata.  Returns non-zero on error.
 */
int dset_metadata(DLite *d, const char *metadata)
{
  int stat;
  if (!d->api->setMetadata)
    return errx(1, "driver '%s' does not support set_metadata", d->api->name);
  stat = d->api->setMetadata(d, metadata);
  if (d->metadata) free(d->metadata);
  if (!(d->metadata = strdup(metadata))) return err(1, NULL);
  return stat;
}


/*
  Sets size of dimension `name`.  Returns non-zero on error.
*/
int dset_dimension_size(DLite *d, const char *name, int size)
{
  if (!d->api->setDimensionSize)
    return errx(1, "driver '%s' does not support set_dimension_size",
                d->api->name);
  return d->api->setDimensionSize(d, name, size);
}


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
  Returns a positive value if dimension `name` is defined, zero if it
  isn't and a negative value on error (e.g. if this function isn't
  supported by the backend).
 */
int dhas_dimension(DLite *d, const char *name)
{
  if (d->api->hasDimension) return d->api->hasDimension(d, name);
  return errx(-1, "driver '%s' does not support hasDimension()", d->api->name);
}


/*
  Returns a positive value if property `name` is defined, zero if it
  isn't and a negative value on error (e.g. if this function isn't
  supported by the backend).
 */
int dhas_property(DLite *d, const char *name)
{
  if (d->api->hasProperty) return d->api->hasProperty(d, name);
  return errx(-1, "driver '%s' does not support hasProperty()", d->api->name);
}


/*
  If the uuid was generated from a unique name, return a pointer to a
  newly malloc'ed string with this name.  Otherwise NULL is returned.
*/
char *dget_dataname(DLite *d)
{
  if (d->api->getDataName) return d->api->getDataName(d);
  errx(1, "driver '%s' does not support getDataName()", d->api->name);
  return NULL;
}


/*
  Returns 1 if DLite object has been opened in read-only mode, 0 if it
  allows writing and -1 if this function isn't supported by the backend.
 */
int dis_readonly(DLite *d)
{
  if (d->api->isReadOnly) return d->api->isReadOnly(d);
  errx(1, "driver '%s' does not support isReadOnly()", d->api->name);
  return -1;
}


/********************************************************************
 * Utility functions intended to be used by teh backends
 ********************************************************************/

/* Initialises DLite instance. */
void dlite_init(DLite *d)
{
  memset(d, 0, sizeof(DLite));
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
