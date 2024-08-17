#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils/compat.h"
#include "utils/err.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-datamodel.h"




/********************************************************************
 * Required api
 ********************************************************************/


/*
  Returns a new data model for instance `id` in storage `s` or NULL on error.
  Should be free'ed with dlite_datamodel_free().
 */
DLiteDataModel *dlite_datamodel(const DLiteStorage *s, const char *id)
{
  DLiteDataModel *d=NULL;
  char **uuids=NULL;
  char uuid[DLITE_UUID_LENGTH+1];
  int uuidver;

  /* allow id to be NULL if the storage only contains one instance */
  if (!id || !*id) {
    int n=0;
    if ((uuids = dlite_storage_uuids(s, NULL))) {
      while (uuids[n]) n++;
      if (n == 1)
        id = uuids[0];
      else
        FAIL2("`id` required to load from storage \"%s\" with %d instances",
              s->location, n);
    } else if (!(s->flags & dliteWritable)) {
      FAIL1("`id` required to load from storage \"%s\"", s->location);
    }
  }

  if ((uuidver = dlite_get_uuid(uuid, id)) < 0)
    FAIL1("failed generating UUID from id \"%s\"", id);

  if (s->idflag == dliteIDKeepID) {
      d = s->api->dataModel(s, id);
  } else if (!id || !*id || s->idflag == dliteIDTranslateToUUID ||
      s->idflag == dliteIDRequireUUID) {
    if (uuidver != 0 && s->idflag == dliteIDRequireUUID)
      FAIL1("id is not a valid UUID: \"%s\"", id);
    d = s->api->dataModel(s, uuid);
  }

  if (!d)
    FAIL2("cannot create datamodel id='%s' for storage '%s'", id, s->api->name);

  /* Initialise common fields */
  d->api = s->api;  /* remove this field? */
  d->s = (DLiteStorage *)s;
  memcpy(d->uuid, uuid, sizeof(d->uuid));

  if (uuidver == 5 && s->flags & dliteWritable && s->api->setDataName)
    s->api->setDataName(d, id);

 fail:
  if (uuids) dlite_storage_uuids_free(uuids);
  return d;
}


/*
  Clears a data model initialised with dlite_datamodel_init().
 */
int dlite_datamodel_free(DLiteDataModel *d)
{
  int stat=0;
  assert(d);
  if (d->api->dataModelFree) stat = d->api->dataModelFree(d);
  free(d);
  return stat;
}


/*
  Returns newly malloc()'ed string with the metadata uri or NULL on error.
 */
char *dlite_datamodel_get_meta_uri(const DLiteDataModel *d)
{
  return d->api->getMetaURI(d);
}


void dlite_datamodel_resolve_dimensions(DLiteDataModel *d, const DLiteMeta *meta)
{
  if (d->api->resolveDimensions)
    d->api->resolveDimensions(d, meta);
}
/*
  Returns the size of dimension `name` or -1 on error.
 */
int dlite_datamodel_get_dimension_size(const DLiteDataModel *d,
                                       const char *name)
{
  return d->api->getDimensionSize(d, name);
}


/*
  Copies property `name` to memory pointed to by `ptr`.  Max `size`
  bytes are written.  If `size` is too small (or `ptr` is NULL),
  nothing is copied to `ptr`.

  Returns non-zero on error.
 */
int dlite_datamodel_get_property(const DLiteDataModel *d, const char *name,
                                 void *ptr, DLiteType type, size_t size,
                                 size_t ndims, const size_t *shape)
{
  return d->api->getProperty(d, name, ptr, type, size, ndims, shape);
}


/********************************************************************
 * Optional api
 ********************************************************************/

/*
  Sets property `name` from the memory (of `size` bytes) pointed to by `ptr`.
  Returns non-zero on error.
*/
int dlite_datamodel_set_property(DLiteDataModel *d, const char *name,
                                 const void *ptr, DLiteType type, size_t size,
                                 size_t ndims, const size_t *shape)
{
  if (!d->api->setProperty)
    return errx(1, "driver '%s' does not support set_property", d->api->name);
  return d->api->setProperty(d, name, ptr, type, size, ndims, shape);
}


/*
  Sets metadata uri.  Returns non-zero on error.
 */
int dlite_datamodel_set_meta_uri(DLiteDataModel *d, const char *uri)
{
  if (!d->api->setMetaURI)
    return errx(1, "driver '%s' does not support set_meta_uri", d->api->name);
  return d->api->setMetaURI(d, uri);
}


/*
  Sets size of dimension `name`.  Returns non-zero on error.
*/
int dlite_datamodel_set_dimension_size(DLiteDataModel *d, const char *name,
                                       size_t size)
{
  if (!d->api->setDimensionSize)
    return errx(1, "driver '%s' does not support set_dimension_size",
                d->api->name);
  return d->api->setDimensionSize(d, name, size);
}



/*
  Returns a positive value if dimension `name` is defined, zero if it
  isn't and a negative value on error (e.g. if this function isn't
  supported by the backend).
 */
int dlite_datamodel_has_dimension(DLiteDataModel *d, const char *name)
{
  if (d->api->hasDimension) return d->api->hasDimension(d, name);
  return errx(-1, "driver '%s' does not support hasDimension()", d->api->name);
}


/*
  Returns a positive value if property `name` is defined, zero if it
  isn't and a negative value on error (e.g. if this function isn't
  supported by the backend).
 */
int dlite_datamodel_has_property(DLiteDataModel *d, const char *name)
{
  if (d->api->hasProperty) return d->api->hasProperty(d, name);
  return errx(-1, "driver '%s' does not support hasProperty()", d->api->name);
}


/*
  If the uuid was generated from a unique name, return a pointer to a
  newly malloc'ed string with this name.  Otherwise NULL is returned.
*/
char *dlite_datamodel_get_dataname(DLiteDataModel *d)
{
  if (d->api->getDataName) return d->api->getDataName(d);
  errx(1, "driver '%s' does not support getDataName()", d->api->name);
  return NULL;
}



/********************************************************************
 * Utility functions intended to be used by the storage plugins
 ********************************************************************/

/* Copies data from nested pointer to pointers array \a src to the
   flat continuous C-ordered array \a dst. The size of dest must be
   sufficient large.  Returns non-zero on error. */
int dlite_copy_to_flat(void *dst, const void *src, size_t size,
                       size_t ndims, const size_t *shape)
{
  int i, n=0, ntot=1, retval=1, *ind=NULL;
  char *q=dst;
  void **p=(void **)src;

  if (!(ind = calloc(ndims, sizeof(int))))
    FAILCODE(dliteMemoryError, "allocation failure");

  for (i=0; i<(int)ndims-1; i++) p = p[ind[i]];
  for (i=0; i<(int)ndims; i++) ntot *= (shape) ? (int)shape[i] : 1;

  while (n++ < ntot) {
    memcpy(q, *p, size);
    p++;
    q += size;
    if (++ind[ndims-1] >= ((shape) ? (int)shape[ndims-1] : 1)) {
      ind[ndims-1] = 0;
      for (i=ndims-2; i>=0; i--) {
        if (++ind[i] < ((shape) ? (int)shape[i] : i))
          break;
        else
          ind[i] = 0;
      }
      for (i=0, p=(void **)src; i<(int)ndims-1; i++) p = p[ind[i]];
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
int dlite_copy_to_nested(void *dst, const void *src, size_t size,
                         size_t ndims, const size_t *shape)
{
  int i, n=0, ntot=1, *ind=NULL, retval=1;
  const char *q=src;
  void **p=dst;

  if (!(ind = calloc(ndims, sizeof(int))))
   FAILCODE(dliteMemoryError, "allocation failure");

  for (i=0; i<(int)ndims-1; i++) p = p[ind[i]];
  for (i=0; i<(int)ndims; i++) ntot *= (shape) ? (int)shape[i] : 1;

  while (n++ < ntot) {
    memcpy(*p, q, size);
    p++;
    q += size;
    if (++ind[ndims-1] >= ((shape) ? (int)shape[ndims-1] : 1)) {
      ind[ndims-1] = 0;
      for (i=ndims-2; i>=0; i--) {
        if (++ind[i] < ((shape) ? (int)shape[i] : i))
          break;
        else
          ind[i] = 0;
      }
      for (i=0, p=(void **)src; i<(int)ndims-1; i++) p = p[ind[i]];
    }
  }
  retval = 0;
 fail:
  if (ind) free(ind);
  return retval;
}
