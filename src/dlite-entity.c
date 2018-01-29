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
DLiteEntity *dlite_entity_load(DLiteStorage *s, const char *id)
{
  DLiteEntity *e;
  DLiteDataModel *d;
  const char *p;
  char uuid[DLITE_UUID_LENGTH+1];
  int uuidver;
  size_t i;

  if (!(e = calloc(1, sizeof(DLiteEntity)))) FAIL("allocation failure");
  if ((uuidver = dlite_get_uuid(uuid, id)) < 0) goto fail;
  if (!(d = dlite_datamodel(s, uuid))) goto fail;

  memcpy(e->uuid, uuid, sizeof(e->uuid));
  if (uuidver == 5)
    e->url = strdup(id);
  else if ((p = dlite_datamodel_get_metadata(d)))
    e->url = strdup(p);

  if (!(e->ndims = dlite_datamodel_get_dimension_size(d, "ndims"))) goto fail;
  if (!(e->nprops = dlite_datamodel_get_dimension_size(d, "nprops"))) goto fail;
  e->nrelsets = 0;

  if (dlite_datamodel_get_property(d, "name", &e->name,
                                   dliteStringPtr, sizeof(char *),
                                   1, NULL)) goto fail;
  if (dlite_datamodel_get_property(d, "version", &e->version,
                                   dliteStringPtr, sizeof(char *),
                                   1, NULL)) goto fail;
  if (dlite_datamodel_get_property(d, "namespace", &e->namespace,
                                   dliteStringPtr, sizeof(char *),
                                   1, NULL)) goto fail;
  if (dlite_datamodel_has_property(d, "description") > 0 &&
      dlite_datamodel_get_property(d, "description", &e->description,
                                   dliteStringPtr, sizeof(char *),
                                   1, NULL)) goto fail;

  if (dlite_datamodel_get_property(d, "dimnames", e->dimnames,
                                   dliteStringPtr, sizeof(char *),
                                   1, &e->ndims)) goto fail;
  if (dlite_datamodel_has_property(d, "dimdescr") > 0 &&
      dlite_datamodel_get_property(d, "dimdescr", e->dimdescr,
                                   dliteStringPtr, sizeof(char *),
                                   1, &e->ndims)) goto fail;

  if (dlite_datamodel_get_property(d, "propnames", e->propnames,
                                   dliteStringPtr, sizeof(char *),
                                   1, &e->nprops)) goto fail;
  if (dlite_datamodel_get_property(d, "proptypes", e->proptypes,
                                   dliteInt, sizeof(int),
                                   1, &e->nprops)) goto fail;
  if (dlite_datamodel_get_property(d, "propsizes", e->propsizes,
                                   dliteUInt, sizeof(size_t),
                                   1, &e->nprops)) goto fail;
  if (dlite_datamodel_get_property(d, "propndims", e->propndims,
                                   dliteUInt, sizeof(size_t),
                                   1, &e->nprops)) goto fail;
  for (i=0; i<e->nprops; i++) {
    if (e->propndims[i]) {
      char buf[256];
      snprintf(buf, sizeof(buf), "propdims_%s", e->propnames[i]);
      if (dlite_datamodel_get_property(d, buf, e->propdims[i],
                                       dliteUInt, sizeof(size_t),
                                       1, &e->propndims[i])) goto fail;
    }
  }
  if (dlite_datamodel_has_property(d, "propdescr") > 0 &&
      dlite_datamodel_get_property(d, "propdescr", e->propdescr,
                                   dliteStringPtr, sizeof(char *),
                                   1, &e->nprops)) goto fail;
  dlite_datamodel_free(d);
  return e;
 fail:
  if (d) dlite_datamodel_free(d);
  if (e) {
    dlite_entity_clear(e);
    free(e);
  }
  return NULL;
}


/*
  Saves Entity to storage `s`.  Returns non-zero on error.
 */
int dlite_entity_save(DLiteStorage *s, const DLiteEntity *e)
{
  int retval=1;
  size_t i;
  DLiteDataModel *d;
  const char *description, *empty="";

  if (!(d = dlite_datamodel(s, e->uuid))) goto fail;

  if (dlite_datamodel_set_metadata(d, e->url)) goto fail;

  if (dlite_datamodel_set_dimension_size(d, "ndims", e->ndims)) goto fail;
  if (dlite_datamodel_set_dimension_size(d, "nprops", e->nprops)) goto fail;

  if (dlite_datamodel_set_property(d, "name", &e->name,
                                   dliteStringPtr, sizeof(char *),
                                   1, NULL)) goto fail;
  if (dlite_datamodel_set_property(d, "version", &e->version,
                                   dliteStringPtr, sizeof(char *),
                                   1, NULL)) goto fail;
  if (dlite_datamodel_set_property(d, "namespace", &e->namespace,
                                   dliteStringPtr, sizeof(char *),
                                   1, NULL)) goto fail;
  description = (e->description) ? &e->description : (void *)&empty;
  if (dlite_datamodel_set_property(d, "description", description,
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
  for (i=0; i<e->nprops; i++) {
    if (e->propndims[i]) {
      char buf[256];
      snprintf(buf, sizeof(buf), "propdims_%s", e->propnames[i]);
      if (dlite_datamodel_set_property(d, buf, e->propdims[i],
                                       dliteUInt, sizeof(size_t),
                                       1, &e->propndims[i])) goto fail;
    }
  }
  if (dlite_datamodel_set_property(d, "propdescr", e->propdescr,
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
void dlite_entity_incref(DLiteEntity *entity)
{
  entity->refcount++;
}

/*
  Decrease reference count to Entity.  If the reference count reaches
  zero, the Entity is free'ed.
 */
void dlite_entity_decref(DLiteEntity *entity)
{
  if (--entity->refcount <= 0) {
    dlite_entity_clear(entity);
    free(entity);
  }
}

/*
  Free's all memory used by `entity` and clear all data.
 */
void dlite_entity_clear(DLiteEntity *entity)
{
  size_t i;
  if (entity->propunits) {
    for (i=0; i<entity->nprops; i++)
      if (entity->propunits[i]) free((char *)entity->propunits[i]);
    free(entity->propunits);
  }
  entity->propunits = NULL;
  dlite_meta_clear((DLiteMeta *)entity);
}


/********************************************************************
 *  Meta data
 *
 *  These functions are mainly used internally or by code generators.
 *  Do not waist time on them...
 ********************************************************************/

/*
  Initialises internal properties of `meta`.  This function should
  not be called before the non-internal properties has been initialised.

  Returns non-zero on error.
 */
int dlite_meta_postinit(DLiteMeta *meta)
{
  int retval=1;
  size_t maxalign;     /* max alignment of any member */
  size_t i, align, propsize, padding, offset, size;
  DLiteType proptype;

  if (!(meta->dimoffsets = calloc(meta->ndims, sizeof(size_t))))
    FAIL("allocation failure");
  if (!(meta->propoffsets = calloc(meta->nprops, sizeof(size_t))))
    FAIL("allocation failure");
  if (!(meta->reloffsets = calloc(meta->nrelsets, sizeof(size_t))))
    FAIL("allocation failure");

  /* Header */
  offset = offsetof(DLiteInstance, meta);
  size = sizeof(DLiteMeta *);
  if (!(maxalign = dlite_type_get_alignment(dliteStringPtr, sizeof(char *))))
    goto fail;

  /* Dimensions */
  for (i=0; i<meta->ndims; i++) {
    offset = dlite_type_get_member_offset(offset, size,
                                          dliteInt, sizeof(size_t));
    size = sizeof(int);
    meta->dimoffsets[i] = offset;
  }
  if (meta->ndims &&
      (align = dlite_type_get_alignment(dliteUInt, sizeof(size_t))) > maxalign)
    maxalign = align;

  /* Properties */
  for (i=0; i<meta->nprops; i++) {
    if (meta->propndims[i] > 0 && meta->propdims[i]) {
      proptype = dliteStringPtr;  /* pointer */
      propsize = sizeof(void *);
    } else {
      proptype = meta->proptypes[i];
      propsize = meta->propsizes[i];
    }
    offset = dlite_type_get_member_offset(offset, size, proptype, propsize);
    size = meta->propsizes[i];
    meta->propoffsets[i] = offset;

    if ((align = dlite_type_get_alignment(proptype, propsize)) > maxalign)
      maxalign = align;
  }

  /* Relations */
  for (i=0; i<meta->nrelsets; i++) {
    offset = dlite_type_get_member_offset(offset, size, dliteStringPtr,
                                          sizeof(DLiteTriplet *));
    size = sizeof(DLiteTriplet *);
    meta->reloffsets[i] = offset;
  }

  padding = (maxalign - (offset & (maxalign - 1))) & (maxalign - 1);
  meta->size = offset + padding;
  meta->refcount = 0;

  retval = 0;
 fail:
  return retval;
}


/*
  Free's all memory used by `meta` and clear all data.
 */
void dlite_meta_clear(DLiteMeta *meta)
{
  size_t i;
  if (meta->url) free((char *)meta->url);

  if (meta->name) free((char *)meta->name);
  if (meta->version) free((char *)meta->version);
  if (meta->namespace) free((char *)meta->namespace);
  if (meta->description) free((char *)meta->description);

  if (meta->dimnames) {
    for (i=0; i<meta->ndims; i++)
      if (meta->dimnames[i]) free((char *)meta->dimnames[i]);
    free(meta->dimnames);
  }
  if (meta->dimdescr) {
    for (i=0; i<meta->ndims; i++)
      if (meta->dimdescr[i]) free((char *)meta->dimdescr[i]);
    free(meta->dimdescr);
  }

  if (meta->propnames) {
    for (i=0; i<meta->nprops; i++)
      if (meta->propnames[i]) free((char *)meta->propnames[i]);
    free(meta->propnames);
  }
  if (meta->proptypes) free(meta->proptypes);
  if (meta->propsizes) free(meta->propsizes);
  if (meta->propndims) free(meta->propndims);
  if (meta->propdims) {
    for (i=0; i<meta->nprops; i++)
      if (meta->propdims[i]) free(meta->propdims[i]);
    free(meta->propdims);
  }
  if (meta->propdescr) {
    for (i=0; i<meta->nprops; i++)
      if (meta->propdescr[i]) free((char *)meta->propdescr[i]);
    free(meta->propdescr);
  }

  if (meta->dimoffsets) free(meta->dimoffsets);
  if (meta->propoffsets) free(meta->propoffsets);
  if (meta->reloffsets) free(meta->reloffsets);

  if (meta->relcounts) free(meta->relcounts);
  if (meta->reldescr) {
    for (i=0; i<meta->nrelsets; i++)
      if (meta->reldescr[i]) free((char *)meta->reldescr[i]);
    free(meta->reldescr);
  }

  memset(meta, 0, sizeof(DLiteMeta));
}
