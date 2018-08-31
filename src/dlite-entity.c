#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "err.h"
#include "dlite.h"
#include "dlite-type.h"
#include "dlite-store.h"
#include "dlite-entity.h"
#include "dlite-datamodel.h"


/* FIXME - add basic_metadata_schema */


/* schema_entity */
static size_t schema_entity_propoffsets[] = {
  offsetof(DLiteEntity, dimensions),
  offsetof(DLiteEntity, properties)
};
static DLiteSchemaDimension schema_entity_dimensions[] = {
  {"ndimensions", "Number of dimensions."},
  {"nproperties", "Number of properties."}
};
static DLiteSchemaProperty schema_entity_prop0 = {
  "dimensions",                              /* name */
  dliteSchemaDimension,                      /* type */
  sizeof(dliteSchemaDimension),              /* size */
  0,                                         /* ndims */
  NULL,                                      /* dims */
  "Name and description of each dimension."  /* description */
};
static DLiteSchemaProperty schema_entity_prop1 = {
  "properties",                              /* name */
  dliteSchemaProperty,                       /* type */
  sizeof(dliteSchemaProperty),               /* size */
  0,                                         /* ndims */
  NULL,                                      /* dims */
  "Name and description of each dimension."  /* description */
};
static DLiteSchemaProperty *schema_entity_properties[] = {
  &schema_entity_prop0,
  &schema_entity_prop1
};
static DLiteMeta schema_entity = {
  "d1ce1dc7-e2b1-51a1-8957-3277815aed18",     /* uuid (corresponds to uri) */
  "http://meta.sintef.no/0.1/schema-entity",  /* uri  */
  1,                                          /* refcount, never free */
  NULL,                                       /* meta */
  "Schema for Entities",                      /* description */
  sizeof(DLiteEntity),                        /* size */
  offsetof(DLiteEntity, ndimensions),         /* dimoffset */
  schema_entity_propoffsets,                  /* propoffsets */
  0,                                          /* reloffset */
  NULL,                                       /* init */
  NULL,                                       /* deinit */
  //NULL,                                       /* loadprop */
  //NULL,                                       /* saveprop */
  0,                                          /* flags */
  schema_entity_dimensions,                   /* dimensions */
  schema_entity_properties,                   /* properties */
  NULL,                                       /* relations */
  2,                                          /* ndimensions */
  2,                                          /* nproperties */
  0,                                          /* nrelations */
};


/* Convenient macros for failing */
#define FAIL(msg) do { err(1, msg); goto fail; } while (0)
#define FAIL1(msg, a1) do { err(1, msg, a1); goto fail; } while (0)
#define FAIL2(msg, a1, a2) do { err(1, msg, a1, a2); goto fail; } while (0)


/********************************************************************
 *  Metadata cache
 ********************************************************************/

static DLiteStore *_metastore = NULL;

/* Frees up a global instance store */
static void _metastore_free()
{
  dlite_store_free(_metastore);
  _metastore = NULL;
}

/* Returns a new initializes instance store */
static DLiteStore *_metastore_init()
{
  DLiteStore *store = dlite_store_create();
  atexit(_metastore_free);
  return store;
}


/********************************************************************
 *  Instances
 ********************************************************************/

/*
  Returns a new dlite instance from Entiry `meta` and dimensions
  `dims`.  The lengths of `dims` is found in `meta->ndims`.

  The `id` argment may be NULL, a valid UUID or an unique identifier
  to this instance (e.g. an uri).  In the first case, a random UUID
  will be generated. In the second case, the instance will get the
  provided UUID.  In the third case, an UUID will be generated from
  `id`.  In addition, the instanc's uri member will be assigned to
  `id`.

  All properties are initialised to zero and arrays for all dimensional
  properties are allocated and initialised to zero.

  On error, NULL is returned.
 */
DLiteInstance *dlite_instance_create(DLiteEntity *entity, size_t *dims,
                                     const char *id)
{
  DLiteMeta *meta = (DLiteMeta *)entity;
  char uuid[DLITE_UUID_LENGTH+1];
  size_t i;
  DLiteInstance *inst=NULL;
  int j, uuid_version;

  /* Allocate instance */
  if (!(inst = calloc(1, meta->size))) FAIL("allocation failure");

  /* Initialise header */
  if ((uuid_version = dlite_get_uuid(uuid, id)) < 0) goto fail;
  memcpy(inst->uuid, uuid, sizeof(uuid));
  if (uuid_version == 5) inst->uri = strdup(id);
  inst->meta = (DLiteMeta *)meta;

  /* Set dimensions */
  if (meta->ndimensions) {
    size_t *dimensions = (size_t *)((char *)inst + meta->dimoffset);
    memcpy(dimensions, dims, meta->ndimensions*sizeof(size_t));
  }

  /* Allocate arrays for dimensional properties */
  for (i=0; i<meta->nproperties; i++) {
    DLiteSchemaProperty *p = meta->properties[i];
    void **ptr = (void **)((char *)inst + meta->propoffsets[i]);

    if (p->ndims > 0 && p->dims) {
      size_t nmemb=1;
      for (j=0; j<p->ndims; j++)
        nmemb *= dims[p->dims[j]];
      if (!(*ptr = calloc(nmemb, p->size))) goto fail;
    }
  }

  dlite_meta_incref(meta);      /* increase refcount of metadata */
  dlite_instance_incref(inst);  /* increase refcount of the new instance */

  /* Additional initialisation */
  if (meta->init && meta->init(inst)) goto fail;
  return inst;
 fail:
  if (inst) dlite_instance_decref(inst);
  return NULL;
}


/*
  Free's an instance and all arrays associated with dimensional properties.
 */
static void dlite_instance_free(DLiteInstance *inst)
{
  DLiteMeta *meta;
  size_t i, nprops;

  if (!(meta = inst->meta)) {
    free(inst);
    errx(-1, "no metadata available");
    return;
  }

  /* Additional deinitialisation */
  if (meta->deinit) meta->deinit(inst);

  /* Standard free */
  nprops = meta->nproperties;
  if (inst->uri) free((char *)inst->uri);
  if (meta->properties) {
    for (i=0; i<nprops; i++) {
      DLiteProperty *p = (DLiteProperty *)meta->properties[i];

      if (p->type == dliteStringPtr) {
          int j;
          size_t n, nmemb=1, *dims=(size_t *)((char *)inst + meta->dimoffset);
          char **ptr = (char **)((char *)inst + meta->propoffsets[i]);
          if (p->ndims > 0 && p->dims) ptr = *((char ***)ptr);
          if (p->dims)
            for (j=0; j<p->ndims; j++) nmemb *= dims[p->dims[j]];
          for (n=0; n<nmemb; n++)
            free((ptr)[n]);
          if (p->ndims > 0 && p->dims)
            free(ptr);
      } else if (p->ndims > 0 && p->dims) {
	void **ptr = (void *)((char *)inst + meta->propoffsets[i]);
        free(*ptr);
      }
    }
  }
  free(inst);

  dlite_meta_decref(meta);  /* decrease metadata refcount */
}


/*
  Increases reference count on `inst`.
 */
void dlite_instance_incref(DLiteInstance *inst)
{
  inst->refcount++;
}


/*
  Decrease reference count to `inst`.  If the reference count reaches
  zero, the instance is free'ed.
 */
void dlite_instance_decref(DLiteInstance *inst)
{
  if (--inst->refcount <= 0) dlite_instance_free(inst);
}


/*
  Loads instance identified by `id` from storage `s` and returns a
  new and fully initialised dlite instance.

  On error, NULL is returned.
 */
DLiteInstance *dlite_instance_load(DLiteStorage *s, const char *id,
				   DLiteEntity *entity)
{
  DLiteMeta *meta = (DLiteMeta *)entity;
  DLiteInstance *inst=NULL, *instance=NULL;
  DLiteDataModel *d=NULL;
  size_t i, *dims=NULL, *pdims=NULL;
  int j, max_pndims=0;
  const char *uri=NULL;

  /* create datamodel and get metadata uri */
  if (!(d = dlite_datamodel(s, id))) goto fail;
  if (!(uri = dlite_datamodel_get_meta_uri(d))) goto fail;

  /* if metadata is not given, try to load it from cache... */
  if (!meta) {
    if (!_metastore) _metastore_init();
    meta = (DLiteMeta *)dlite_store_get(_metastore, uri);
  }

  /* ...otherwise try to load it from storage */
  if (!meta) {
    char uuid[DLITE_UUID_LENGTH];
    dlite_get_uuid(uuid, uri);
    meta = (DLiteMeta *)dlite_instance_load(s, uuid, NULL);
  }

  /* ...otherwise give up */
  if (!meta) FAIL1("cannot load metadata: %s", uri);

  /* check metadata uri */
  if (strcmp(uri, meta->uri) != 0)
    FAIL2("metadata (%s) does not correspond to metadata in storage (%s)",
	  meta->uri, uri);

  /* read dimensions */
  if (!(dims = calloc(meta->ndimensions, sizeof(size_t))))
    FAIL("allocation failure");
  for (i=0; i<meta->ndimensions; i++)
    if (!(dims[i] =
	  dlite_datamodel_get_dimension_size(d, meta->dimensions[i].name)))
      goto fail;

  /* create instance */
  if (!(inst = dlite_instance_create((DLiteEntity *)meta, dims, id))) goto fail;

  /* assign properties */
  for (i=0; i<meta->nproperties; i++) {
    DLiteProperty *p = (DLiteProperty *)meta->properties[i];
    if (p->ndims > max_pndims) max_pndims = p->ndims;
  }
  pdims = malloc(max_pndims * sizeof(size_t));
  for (i=0; i<meta->nproperties; i++) {
    void *ptr;
    DLiteProperty *p = (DLiteProperty *)meta->properties[i];
    //if (meta->loadprop) {
    //  int stat = meta->loadprop(d, inst, p->name);
    //  if (stat < 0) FAIL2("error loading special property %s of metadata %s",
    //                      p->name, meta->uri);
    //  if (stat == 1) continue;
    //}
    ptr = (void *)dlite_instance_get_property_by_index(inst, i);
    for (j=0; j<p->ndims; j++) pdims[j] = dims[p->dims[j]];
    if (p->ndims > 0 && p->dims) ptr = *((void **)ptr);
    if (dlite_datamodel_get_property(d, p->name, ptr, p->type, p->size,
				     p->ndims, pdims)) goto fail;
  }

  instance = inst;
 fail:
  if (!instance) {
    if (inst) dlite_instance_decref(inst);
  }
  if (d) dlite_datamodel_free(d);
  if (uri) free((char *)uri);
  if (dims) free(dims);
  if (pdims) free(pdims);
  return instance;
}


/*
  Saves instance `inst` to storage `s`.  Returns non-zero on error.
 */
int dlite_instance_save(DLiteStorage *s, const DLiteInstance *inst)
{
  DLiteDataModel *d=NULL;
  DLiteMeta *meta;
  int j, max_pndims=0, retval=1;
  size_t i, *pdims, *dims;

  if (!(meta = inst->meta)) return errx(-1, "no metadata available");
  if (!(d = dlite_datamodel(s, inst->uuid))) goto fail;
  if (dlite_datamodel_set_meta_uri(d, meta->uri)) goto fail;

  dims = (size_t *)((char *)inst + inst->meta->dimoffset);
  for (i=0; i<meta->ndimensions; i++) {
    char *dimname = inst->meta->dimensions[i].name;
    if (dlite_datamodel_set_dimension_size(d, dimname, dims[i])) goto fail;
  }

  for (i=0; i<meta->nproperties; i++) {
    DLiteProperty *p = (DLiteProperty *)meta->properties[i];
    if (p->ndims > max_pndims) max_pndims = p->ndims;
  }
  pdims = malloc(max_pndims * sizeof(size_t));

  for (i=0; i<meta->nproperties; i++) {
    const void *ptr;
    DLiteProperty *p = (DLiteProperty *)inst->meta->properties[i];
    //if (meta->saveprop) {
    //  int stat = meta->saveprop(d, inst, p->name);
    //  if (stat < 0) FAIL2("error saving special property %s of metadata %s",
    //                      p->name, meta->uri);
    //  if (stat == 1) continue;
    //}
    ptr = dlite_instance_get_property_by_index(inst, i);
    for (j=0; j<p->ndims; j++) pdims[j] = dims[p->dims[j]];
    if (p->ndims > 0 && p->dims) ptr = *((void **)ptr);

    if (dlite_datamodel_set_property(d, p->name, ptr, p->type, p->size,
				     p->ndims, pdims)) goto fail;
  }
  retval = 0;
 fail:
  if (d) dlite_datamodel_free(d);
  if (pdims) free(pdims);
  return retval;
}


/*
  Returns number of dimensions or -1 on error.
 */
int dlite_instance_get_ndimensions(const DLiteInstance *inst)
{
  if (!inst->meta)
    return errx(-1, "no metadata available");
  return inst->meta->ndimensions;
}


/*
  Returns number of properties or -1 on error.
 */
int dlite_instance_get_nproperties(const DLiteInstance *inst)
{
  if (!inst->meta)
    return errx(-1, "no metadata available");
  return inst->meta->nproperties;
}


/*
  Returns size of dimension `i` or -1 on error.
 */
int dlite_instance_get_dimension_size_by_index(const DLiteInstance *inst,
                                               size_t i)
{
  size_t *dimensions;
  if (!inst->meta)
    return errx(-1, "no metadata available");
  dimensions = (size_t *)((char *)inst + inst->meta->dimoffset);
  if (i >= inst->meta->nproperties)
    return errx(-1, "no property with index %lu in %s", i, inst->meta->uri);
  return dimensions[i];
}


/*
  Returns a pointer to data for property `i` or NULL on error.
 */
const void *dlite_instance_get_property_by_index(const DLiteInstance *inst,
						 size_t i)
{
  if (!inst->meta)
    return errx(-1, "no metadata available"), NULL;
  if (i >= inst->meta->nproperties)
    return errx(1, "no property with index %lu in %s",
		i, inst->meta->uri), NULL;
  return (char *)inst + inst->meta->propoffsets[i];
}


/*
  Copies memory pointed to by `ptr` to property `i`.
  Returns non-zero on error.
*/
int dlite_instance_set_property_by_index(DLiteInstance *inst, size_t i,
					 const void *ptr)
{
  DLiteMeta *meta;
  DLiteSchemaProperty *p;
  size_t n, nmemb=1, *dims;
  int j;
  void *dest;
  if (!(meta =inst->meta))
    return errx(-1, "no metadata available");

  p = meta->properties[i];
  dims = (size_t *)((char *)inst + meta->dimoffset);
  dest = (void *)((char *)inst + meta->propoffsets[i]);

  if (p->ndims > 0 && p->dims) dest = *((void **)dest);

  if (p->dims)
    for (j=0; j<p->ndims; j++) nmemb *= dims[p->dims[j]];

  if (p->type == dliteStringPtr) {
    char **q=(char **)dest, **src = (char **)ptr;
    for (n=0; n<nmemb; n++) {
      size_t len = strlen(src[n]) + 1;
      q[n] = realloc(q[n], len);
      memcpy(q[n], src[n], len);
    }
  } else {
    memcpy(dest, ptr, nmemb*p->size);
  }
  return 0;
}

/*
  Returns number of dimensions of property with index `i` or -1 on error.
 */
int dlite_instance_get_property_ndims_by_index(const DLiteInstance *inst,
					       size_t i)
{
  const DLiteSchemaProperty *p;
  if (!inst->meta)
    return errx(-1, "no metadata available");
  if (!(p = dlite_meta_get_property_by_index(inst->meta, i)))
    return -1;
  return p->ndims;
}

/*
  Returns size of dimension `j` in property `i` or -1 on error.
 */
int dlite_instance_get_property_dimsize_by_index(const DLiteInstance *inst,
						 size_t i, size_t j)
{
  const DLiteSchemaProperty *p;
  size_t *dims;
  if (!inst->meta)
    return errx(-1, "no metadata available");
  dims = (size_t *)((char *)inst + inst->meta->dimoffset);
  if (!(p = dlite_meta_get_property_by_index(inst->meta, i)))
    return -1;
  if (j >= (size_t)p->ndims)
    return errx(-1, "dimension index j=%lu is our of range", j);
  return dims[p->dims[j]];
}

/*
  Returns size of dimension `i` or -1 on error.
 */
int dlite_instance_get_dimension_size(const DLiteInstance *inst,
                                      const char *name)
{
  int i;
  size_t *dimensions;
  if (!inst->meta)
    return errx(-1, "no metadata available");
  dimensions = (size_t *)((char *)inst + inst->meta->dimoffset);
  if ((i = dlite_meta_get_dimension_index(inst->meta, name)) < 0) return -1;
  if (i >= (int)inst->meta->nproperties)
    return errx(-1, "no property with index %d in %s", i, inst->meta->uri);
  return dimensions[i];
}

/*
  Returns a pointer to data corresponding to `name` or NULL on error.
 */
const void *dlite_instance_get_property(const DLiteInstance *inst,
					const char *name)
{
  int i;
  if (!inst->meta)
    return errx(-1, "no metadata available"), NULL;
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return NULL;
  return (char *)inst + inst->meta->propoffsets[i];
}

/*
  Copies memory pointed to by `ptr` to property `name`.
  Returns non-zero on error.
*/
int dlite_instance_set_property(DLiteInstance *inst, const char *name,
				const void *ptr)
{
  int i;
  if (!inst->meta)
    return errx(-1, "no metadata available");
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return 1;
  return dlite_instance_set_property_by_index(inst, i, ptr);
}

/*
  Returns number of dimensions of property  `name` or -1 on error.
*/
int dlite_instance_get_property_ndims(const DLiteInstance *inst,
				      const char *name)
{
  const DLiteSchemaProperty *p;
  if (!inst->meta)
    return errx(-1, "no metadata available");
  if (!(p = dlite_meta_get_property(inst->meta, name)))
    return -1;
  return p->ndims;
}

/*
  Returns size of dimension `j` of property `name` or NULL on error.
*/
size_t dlite_instance_get_property_dimssize(const DLiteInstance *inst,
					    const char *name, size_t j)
{
  int i;
  if (!inst->meta)
    return errx(-1, "no metadata available");
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return -1;
  return dlite_instance_get_property_dimsize_by_index(inst, i, j);
}


/********************************************************************
 *  Entities
 ********************************************************************/

/*
  Returns a new Entity created from the given arguments.
 */
DLiteEntity *
dlite_entity_create(const char *uri, const char *description,
		    size_t ndimensions, const DLiteDimension *dimensions,
		    size_t nproperties, const DLiteProperty *properties)
{
  DLiteEntity *entity=NULL;
  char uuid[DLITE_UUID_LENGTH+1];
  int uuid_version;

  if (!(entity = calloc(1, sizeof(DLiteEntity)))) FAIL("allocation error");

  if ((uuid_version = dlite_get_uuid(uuid, uri)) < 0) goto fail;
  memcpy(entity->uuid, uuid, sizeof(uuid));
  if (uuid_version == 5) entity->uri = strdup(uri);

  entity->meta = &schema_entity;
  dlite_meta_incref(entity->meta);

  if (description) entity->description = strdup(description);

  if (ndimensions) {
    size_t i;
    if (!(entity->dimensions = calloc(ndimensions, sizeof(DLiteDimension))))
      FAIL("allocation error");
    for (i=0; i<ndimensions; i++) {
      entity->dimensions[i].name = strdup(dimensions[i].name);
      entity->dimensions[i].description = strdup(dimensions[i].description);
    }
  }

  if (nproperties) {
    size_t i, propsize = nproperties * sizeof(DLiteProperty *);
    if (!(entity->properties = malloc(propsize)))
      FAIL("allocation error");
    for (i=0; i<nproperties; i++) {
      DLiteProperty *p;
      const DLiteProperty *q = properties + i;
      if (!(p = calloc(1, sizeof(DLiteProperty))))
        FAIL("allocation error");
      memcpy(p, q, sizeof(DLiteProperty));
      if (!(p->name = strdup(q->name)))
        FAIL("allocation error");
      if (p->ndims) {
        if (!(p->dims = malloc(p->ndims*sizeof(int))))
          FAIL("allocation error");
        memcpy(p->dims, properties[i].dims, p->ndims*sizeof(int));
      } else {
        p->dims = NULL;
      }
      if (q->description && !(p->description = strdup(q->description)))
        FAIL("allocation error");
      if (q->unit && !(p->unit = strdup(q->unit)))
        FAIL("allocation error");
      entity->properties[i] = (DLiteSchemaProperty *)p;
    }
  }
  entity->ndimensions = ndimensions;
  entity->nproperties = nproperties;

  if (dlite_meta_postinit((DLiteMeta *)entity, 0)) goto fail;

  /* Set refcount to 1, since we return a reference to `entity`.
     Also increase the reference count to the meta-metadata. */
  entity->refcount = 1;
  dlite_meta_incref(entity->meta);

  return entity;
 fail:
  if (entity) {
    dlite_entity_clear(entity);
    free(entity);
  }
  return NULL;
}

/*
  Increase reference count to Entity.
 */
void dlite_entity_incref(DLiteEntity *entity)
{
  dlite_meta_incref((DLiteMeta *)entity);
}

/*
  Decrease reference count to Entity.  If the reference count reaches
  zero, the Entity is free'ed.
 */
void dlite_entity_decref(DLiteEntity *entity)
{
  if (entity) {
    if (--entity->refcount <= 0) {
      if (entity->meta) dlite_meta_decref(entity->meta);
      dlite_entity_clear(entity);
      free(entity);
    }
  }
}

/*
  Free's all memory used by `entity` and clear all data.
 */
void dlite_entity_clear(DLiteEntity *entity)
{
  size_t i;
  for (i=0; i<entity->nproperties; i++) {
    DLiteProperty *p = (DLiteProperty *)entity->properties[i];
    if (p->unit) {
      free(p->unit);
      p->unit = NULL;
    }
  }
  dlite_meta_clear((DLiteMeta *)entity);
}


/*
  Returns a new Entity loaded from storage `s`.  The `id` may be either
  an URI to the Entity (typically of the form "namespace/version/name")
  or an UUID.

  Returns NULL on error.
 */
DLiteEntity *dlite_entity_load(const DLiteStorage *s, const char *id)
{
  return s->api->getEntity(s, id);
}


/*
  Saves an Entity to storage `s`.  Returns non-zero on error.
 */
int dlite_entity_save(DLiteStorage *s, const DLiteEntity *e)
{
  if (!s->api->setEntity)
    return errx(1, "driver '%s' does not support setEntity()",
                s->api->name);

  return s->api->setEntity(s, e);
}


/*
  Returns a pointer to property with index `i` or NULL on error.
 */
const DLiteProperty *
dlite_entity_get_property_by_index(const DLiteEntity *entity, size_t i)
{
  if (i >= entity->nproperties)
    return errx(1, "no property with index %lu in %s", i , entity->meta->uri),
      NULL;
  return (const DLiteProperty *)entity->properties[i];
}

/*
  Returns a pointer to property named `name` or NULL on error.
 */
const DLiteProperty *dlite_entity_get_property(const DLiteEntity *entity,
					       const char *name)
{
  int i;
  if ((i = dlite_meta_get_property_index((DLiteMeta *)entity, name)) < 0)
    return NULL;
  return (const DLiteProperty *)entity->properties[i];
}


/********************************************************************
 *  Meta data
 *
 *  These functions are mainly used internally or by code generators.
 *  Do not waist time on them...
 ********************************************************************/

/*
  Initialises internal data of `meta`.  This function should not be
  called before the non-internal properties has been initialised.

  The `ismeta` argument indicates whether the instance described by
  `meta` is metadata itself.

  Returns non-zero on error.
 */
int dlite_meta_postinit(DLiteMeta *meta, bool ismeta)
{
  size_t maxalign;     /* max alignment of any member */
  int retval=1;

  assert(meta->meta);

  if (ismeta) {
    /* Metadata

       Since we hardcode `ndimensions`, `nproperties` and `nrelations` in
       DLiteMeta_HEAD, there will allways be at least 3 dimensions... */
    assert(meta->meta->ndimensions >= 3);
    meta->size = sizeof(DLiteMeta) +
      (meta->meta->ndimensions - 3)*sizeof(size_t);
    meta->dimoffset = offsetof(DLiteMeta, dimensions);
    meta->propoffsets = NULL;
    meta->reloffset = offsetof(DLiteMeta, relations);
  } else {
    /* Instance */
    DLiteType proptype;
    size_t i, align, propsize, padding, offset, size;

    if (!(meta->propoffsets = calloc(meta->nproperties, sizeof(size_t))))
      FAIL("allocation failure");

    /* -- header */
    offset = offsetof(DLiteInstance, meta);
    size = sizeof(DLiteMeta *);
    if (!(maxalign = dlite_type_get_alignment(dliteStringPtr, sizeof(char *))))
      goto fail;

    /* -- dimensions */
    for (i=0; i<meta->ndimensions; i++) {
      offset = dlite_type_get_member_offset(offset, size,
					    dliteInt, sizeof(size_t));
      size = sizeof(int);
      if (i == 0) meta->dimoffset = offset;
    }
    if (meta->ndimensions &&
	(align = dlite_type_get_alignment(dliteUInt,
					  sizeof(size_t))) > maxalign)
      maxalign = align;

    /* -- properties */
    for (i=0; i<meta->nproperties; i++) {
      DLiteSchemaProperty *p = *(meta->properties + i);
      if (p->ndims > 0 && p->dims) {
	proptype = dliteBlob;       /* pointer */
	propsize = sizeof(void *);
      } else {
	proptype = p->type;
	propsize = p->size;
      }
      offset = dlite_type_get_member_offset(offset, size, proptype, propsize);
      size = propsize;
      meta->propoffsets[i] = offset;

      if ((align = dlite_type_get_alignment(proptype, propsize)) > maxalign)
	maxalign = align;
    }

    /* -- relations */
    for (i=0; i<meta->nrelations; i++) {
      offset = dlite_type_get_member_offset(offset, size, dliteStringPtr,
					    sizeof(DLiteRelation *));
      size = sizeof(DLiteRelation *);
    }
    meta->reloffset = offset;

    offset += size;
    padding = (maxalign - (offset & (maxalign - 1))) & (maxalign - 1);
    meta->size = offset + padding;
  }

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
  if (meta->uri) free((char *)meta->uri);
  if (meta->description) free((char *)meta->description);

  if (meta->propoffsets) free(meta->propoffsets);

  if (meta->dimensions) {
    for (i=0; i<meta->ndimensions; i++) {
      DLiteSchemaDimension *d = meta->dimensions + i;
      if (d) {
        if (d->name) free(d->name);
        if (d->description) free(d->description);
      }
    }
    free(meta->dimensions);
  }

  if (meta->properties) {
    for (i=0; i<meta->nproperties; i++) {
      DLiteSchemaProperty *p = meta->properties[i];
      if (p) {
        if (p->name) free(p->name);
        if (p->dims) free(p->dims);
        if (p->description) free(p->description);
        free(p);
      }
    }
    free(meta->properties);
  }
  if (meta->relations) free(meta->relations);

  memset(meta, 0, sizeof(DLiteMeta));
}

/*
  Increase reference count to metadata.
 */
void dlite_meta_incref(DLiteMeta *meta)
{
  if (meta) meta->refcount++;
}

/*
  Decrease reference count to metadata.  If the reference count reaches
  zero, the metadata is free'ed.
 */
void dlite_meta_decref(DLiteMeta *meta)
{
  if (meta) {
    if (--meta->refcount <= 0) {
      if (meta->meta) dlite_meta_decref(meta->meta);  /* decrease refcount */
      dlite_meta_clear(meta);
      free(meta);
    }
  }
}


/*
  Returns index of dimension named `name` or -1 on error.
 */
int dlite_meta_get_dimension_index(const DLiteMeta *meta, const char *name)
{
  size_t i;
  for (i=0; i<meta->ndimensions; i++) {
    DLiteSchemaDimension *p = meta->dimensions + i;
    if (strcmp(name, p->name) == 0) return i;
  }
  return err(-1, "%s has no such dimension: '%s'", meta->uri, name);
}


/*
  Returns index of property named `name` or -1 on error.
 */
int dlite_meta_get_property_index(const DLiteMeta *meta, const char *name)
{
  size_t i;
  for (i=0; i<meta->nproperties; i++) {
    DLiteSchemaProperty *p = meta->properties[i];
    if (strcmp(name, p->name) == 0) return i;
  }
  return err(-1, "%s has no such property: '%s'", meta->uri, name);
}


/*
  Returns a pointer to property with index \a i or NULL on error.
 */
const DLiteSchemaProperty *
dlite_meta_get_property_by_index(const DLiteMeta *meta, size_t i)
{
  return meta->properties[i];
}


/*
  Returns a pointer to property named `name` or NULL on error.
 */
const DLiteSchemaProperty *dlite_meta_get_property(const DLiteMeta *meta,
						   const char *name)
{
  int i;
  if ((i = dlite_meta_get_property_index(meta, name)) < 0) return NULL;
  return meta->properties[i];
}
