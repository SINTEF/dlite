#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "err.h"
#include "dlite.h"
#include "dlite-type.h"
#include "dlite-entity.h"
#include "dlite-datamodel.h"


/* Convenient macros for failing */
#define FAIL(msg) do { err(1, msg); goto fail; } while (0)
#define FAIL1(msg, a1) do { err(1, msg, a1); goto fail; } while (0)


/********************************************************************
 *  Instances
 ********************************************************************/

/*
  Returns a new dlite instance from Entiry `meta` and dimensions
  `dims`.  The lengths of `dims` is found in `meta->ndims`.

  The `id` argment may be NULL, a valid UUID or an unique identifier
  to this instance (e.g. an url).  In the first case, a random UUID
  will be generated. In the second case, the instance will get the
  provided UUID.  In the third case, an UUID will be generated from
  `id`.  In addition, the instanc's url member will be assigned to
  `id`.

  All properties are initialised to zero and arrays for all dimensional
  properties are allocated and initialised to zero.

  On error, NULL is returned.
 */
DLiteInstance *dlite_instance_create(DLiteEntity *meta, size_t *dims,
                                     const char *id)
{
  char uuid[DLITE_UUID_LENGTH+1];
  size_t i, j;
  DLiteInstance *inst=NULL;
  int uuid_version;

#if 0
  size_t size, maxalign=0;     /* max alignment of any member */
  size_t a;

  /* maxalign - consider pointers from DLiteInstance_HEAD */
  if ((a = dlite_type_get_alignment(dliteString, sizeof(char *))) > maxalign)
    maxalign = a;

  /* maxalign - consider size_t in dimensions */
  if ((a = dlite_type_get_alignment(dliteUInt, sizeof(size_t))) > maxalign)
    maxalign = a;

  /* maxalign - consider each property separately */
  for (i=0; i<meta->nprops; i++)
    if ((a = dlite_type_get_alignment(meta->proptypes[i],
                                      meta->propsizes[i])) > maxalign)
      maxalign = a;

  size = sizeof(DLiteInstance) + meta->propoffsets[meta->nprops-1] +
    meta->maxalign;
#endif

  /* Allocate instance */
  if (!(inst = calloc(1, meta->size))) FAIL("allocation failure");

  /* Initialise header */
  if ((uuid_version = dlite_get_uuid(uuid, id)) < 0) goto fail;
  memcpy(inst->uuid, uuid, sizeof(uuid));
  if (uuid_version == 5) inst->url = strdup(id);
  inst->meta = (DLiteMeta *)meta;

  /* Set dimensions */
  if (meta->ndims) {
    size_t *dimensions = (size_t *)((char *)inst + meta->dimoffsets[0]);
    memcpy(dimensions, dims, meta->ndims*sizeof(size_t));
  }

  /* Allocate arrays dimensional properties */
  for (i=0; i<meta->nprops; i++) {
    if (meta->propndims[i] > 0 && meta->propdims[i]) {
      char **ptr = (char **)inst + meta->propoffsets[i];
      size_t nmemb=1;
      for (j=0; j<meta->propndims[i]; j++)
        nmemb *= dims[meta->propdims[i][j]];
      if (!(*ptr = calloc(nmemb, meta->propsizes[i]))) goto fail;
    }
  }

  return inst;
 fail:
  if (inst) dlite_instance_free(inst);
  return NULL;
}


/*
  Loads instance identified by `id` from storage `s` and returns a
  new and fully initialised dlite instance.

  On error, NULL is returned.
 */
DLiteInstance *dlite_instance_load(DLiteStorage *s, const char *id);


/*
  Saves instance `inst` to storage `s`.  Returns non-zero on error.
 */
int dlite_instance_save(DLiteStorage *s, const DLiteInstance *inst);


/*
  Free's an instance and all arrays associated with dimensional properties.
 */
void dlite_instance_free(DLiteInstance *inst)
{
  DLiteMeta *meta = inst->meta;
  size_t i, nprops = meta->nprops;

  for (i=0; i<nprops; i++) {
    if (meta->propndims[i] > 0 && meta->propdims[i]) {
      char **ptr = (char **)inst + meta->propoffsets[i];
      free(*ptr);
    }
  }
  free(inst);
}


/********************************************************************
 *  Entities
 ********************************************************************/


/*
  Loads Entity identified by `id` from storage `s` and returns a
  new and fully initialised DLiteEntity object.

  Its `meta` member will be NULL, so it will not refer to any
  meta-metadata.

  On error, NULL is returned.
 */
DLiteEntity *dlite_entity_load(DLiteStorage *s, const char *id);

/*
  Saves Entity to storage `s`.  Returns non-zero on error.
 */
int dlite_entity_save(DLiteStorage *s, const DLiteEntity *e)
{
  int retval=1;
  DLiteDataModel *d = dlite_datamodel(s, e->uuid);
  size_t propdims[] = {e->nprops, e->npropdims_max};

  if (dlite_datamodel_set_metadata(d, e->url)) goto fail;

  if (dlite_datamodel_set_dimension_size(d, "ndims", e->ndims)) goto fail;
  if (dlite_datamodel_set_dimension_size(d, "nprops", e->nprops)) goto fail;
  if (dlite_datamodel_set_dimension_size(d, "npropdims_max",
                                         e->npropdims_max)) goto fail;

  if (dlite_datamodel_set_property(d, "name", e->name,
                                   dliteStringPtr, sizeof(char *),
                                   1, NULL)) goto fail;
  if (dlite_datamodel_set_property(d, "version", e->version,
                                   dliteStringPtr, sizeof(char *),
                                   1, NULL)) goto fail;
  if (dlite_datamodel_set_property(d, "namespace", e->namespace,
                                   dliteStringPtr, sizeof(char *),
                                   1, NULL)) goto fail;
  if (dlite_datamodel_set_property(d, "description",
                                   (e->description) ? e->description : "",
                                   dliteStringPtr, sizeof(char *),
                                   1, NULL)) goto fail;

  if (dlite_datamodel_set_property(d, "dimnames", e->dimnames,
                                   dliteStringPtr, sizeof(char *),
                                   1, &e->ndims)) goto fail;
  if (dlite_datamodel_set_property(d, "dimdescr", e->dimdescr,
                                   dliteStringPtr, sizeof(char *),
                                   1, &e->ndims)) goto fail;

  if (dlite_datamodel_set_property(d, "propnames", e->propnames,
                                   dliteStringPtr, sizeof(char *),
                                   1, &e->nprops)) goto fail;
  if (dlite_datamodel_set_property(d, "proptypes", e->proptypes,
                                   dliteInt, sizeof(DLiteType),
                                   1, &e->nprops)) goto fail;
  if (dlite_datamodel_set_property(d, "propsizes", e->propsizes,
                                   dliteUInt, sizeof(size_t),
                                   1, &e->nprops)) goto fail;
  if (dlite_datamodel_set_property(d, "propndims", e->propndims,
                                   dliteUInt, sizeof(size_t),
                                   2, propdims)) goto fail;
  if (dlite_datamodel_set_property(d, "propdescr",
                                   (e->propdescr) ? e->propdescr : "",
                                   dliteStringPtr, sizeof(char *),
                                   1, &e->nprops)) goto fail;

  retval = 0;  /* done, the rest is cleanup */
 fail:
  dlite_datamodel_free(d);
  return retval;
}


/*
  Increase reference count to Entity.
 */
void dlite_entity_incref(DLiteEntity *entity);

/*
  Decrease reference count to Entity.  If the reference count reaches
  zero, the Entity is free'ed.
 */
void dlite_entity_decref(DLiteEntity *entity);
