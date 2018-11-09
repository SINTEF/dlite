#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "err.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-type.h"
#include "dlite-store.h"
#include "dlite-entity.h"
#include "dlite-datamodel.h"
#include "dlite-schemas.c"

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) >= (y)) ? (x) : (y))


/* Prototypes */
int dlite_meta_init(DLiteMeta *meta);



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
DLiteInstance *dlite_instance_create(const DLiteEntity *entity,
                                     const size_t *dims,
                                     const char *id)
{
  DLiteMeta *meta = (DLiteMeta *)entity;
  char uuid[DLITE_UUID_LENGTH+1];
  size_t i, size;
  DLiteInstance *inst=NULL;
  int j, uuid_version;

  /* Make sure that metadata is initialised */
  if (!meta->propoffsets && dlite_meta_init(meta)) goto fail;
  if (dlite_metastore_add(meta)) goto fail;

  /* Allocate instance */
  size = meta->pooffset;
  if (dlite_meta_is_metameta(meta)) {
    size_t nproperties;
    if ((j = dlite_meta_get_dimension_index(meta, "nproperties")) < 0)
      goto fail;
    nproperties = dims[j];
    size += nproperties*sizeof(size_t);
  }
  size += padding_at(DLiteInstance, size);  /* add final padding */
  if (!(inst = calloc(1, size))) FAIL("allocation failure");

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
    DLiteProperty *p = DLITE_PROP_DESCR(inst, i);
    void **ptr = DLITE_PROP(inst, i);
    if (p->ndims > 0 && p->dims) {
      size_t nmemb=1, size=p->size;
      for (j=0; j<p->ndims; j++) nmemb *= dims[p->dims[j]];
      if (p->type == dliteFixString) size += 1;
      if (!(*ptr = calloc(nmemb, size))) goto fail;
    }
  }

  /* Additional initialisation */
  if (meta->init && meta->init(inst)) goto fail;

  /* Increase reference counts */
  dlite_meta_incref(meta);      /* increase refcount of metadata */
  dlite_instance_incref(inst);  /* increase refcount of the new instance */

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
  const DLiteMeta *meta;
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
      DLiteProperty *p = (DLiteProperty *)meta->properties + i;
      char **ptr = (char **)DLITE_PROP(inst, i);

      if (dlite_type_is_allocated(p->type)) {
        int j;
        size_t n, nmemb=1, *dims=(size_t *)((char *)inst + meta->dimoffset);
        if (p->ndims > 0 && p->dims) ptr = *((char ***)ptr);
        for (j=0; j<p->ndims; j++) nmemb *= dims[p->dims[j]];
        for (n=0; n<nmemb; n++)
          dlite_type_clear((char *)ptr + n*p->size, p->type, p->size);
        if (p->ndims > 0 && p->dims) free(ptr);
      } else if (p->ndims > 0 && p->dims) {
        free(*ptr);
      }
    }
  }
  free(inst);

  dlite_meta_decref((DLiteMeta *)meta);  /* decrease metadata refcount */
}


/*
  Increases reference count on `inst`.

  Returns the new reference count.
 */
int dlite_instance_incref(DLiteInstance *inst)
{
  return ++inst->refcount;
}


/*
  Decrease reference count to `inst`.  If the reference count reaches
  zero, the instance is free'ed.

  Returns the new reference count.
 */
int dlite_instance_decref(DLiteInstance *inst)
{
  int count;
  assert(inst->refcount > 0);
  if ((count = --inst->refcount <= 0)) dlite_instance_free(inst);
  return count;
}


/*
  Loads instance identified by `id` from storage `s` and returns a
  new and fully initialised dlite instance.

  On error, NULL is returned.
 */
DLiteInstance *dlite_instance_load(const DLiteStorage *s, const char *id,
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
  if (!meta)
    meta = dlite_metastore_get(uri);

  /* ...otherwise try to load it from storage */
  if (!meta) {
    char uuid[DLITE_UUID_LENGTH];
    dlite_get_uuid(uuid, uri);
    if (!id || strcmp(uuid, id))
      meta = (DLiteMeta *)dlite_instance_load(s, uuid, NULL);
  }

  /* ...otherwise give up */
  if (!meta) FAIL1("cannot load metadata: %s", uri);

  /* make sure that metadata is initialised */
  if (!meta->pooffset && dlite_meta_init(meta)) goto fail;

  /* check metadata uri */
  if (strcmp(uri, meta->uri) != 0)
    FAIL3("metadata uri (%s) does not correspond to that in storage (%s): %s",
	  meta->uri, uri, s->uri);

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
    DLiteProperty *p = (DLiteProperty *)meta->properties + i;
    if (p->ndims > max_pndims) max_pndims = p->ndims;
  }
  pdims = malloc(max_pndims * sizeof(size_t));
  for (i=0; i<meta->nproperties; i++) {
    void *ptr;
    DLiteProperty *p = (DLiteProperty *)meta->properties + i;
    ptr = (void *)dlite_instance_get_property_by_index(inst, i);
    for (j=0; j<p->ndims; j++) pdims[j] = dims[p->dims[j]];
    if (dlite_datamodel_get_property(d, p->name, ptr, p->type, p->size,
				     p->ndims, pdims)) goto fail;
  }

  /* initiates metadata if the new instance is metadata */
  if (dlite_meta_is_metameta(inst->meta) && dlite_meta_init((DLiteMeta *)inst))
    goto fail;

  if (!inst->uri) {
    if (dlite_meta_is_metameta(inst->meta)) {
      char **name = dlite_instance_get_property(inst, "name");
      char **version = dlite_instance_get_property(inst, "version");
      char **namespace = dlite_instance_get_property(inst, "namespace");
      if (name && version && namespace)
        inst->uri = dlite_join_meta_uri(*name, *version, *namespace);
      else
        FAIL2("metadata %s loaded from %s has no name, version and namespace",
             id, s->uri);
    } else {
      FILE *old = err_set_stream(NULL);
      char **dataname = dlite_instance_get_property(inst, "dataname");
      err_set_stream(old);
      if (dataname) inst->uri = strdup(*dataname);
    }
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
  const DLiteMeta *meta;
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
    DLiteProperty *p = (DLiteProperty *)meta->properties + i;
    if (p->ndims > max_pndims) max_pndims = p->ndims;
  }
  pdims = malloc(max_pndims * sizeof(size_t));

  for (i=0; i<meta->nproperties; i++) {
    const void *ptr;
    DLiteProperty *p = (DLiteProperty *)inst->meta->properties + i;
    //if (meta->saveprop) {
    //  int stat = meta->saveprop(d, inst, p->name);
    //  if (stat < 0) FAIL2("error saving special property %s of metadata %s",
    //                      p->name, meta->uri);
    //  if (stat == 1) continue;
    //}
    for (j=0; j<p->ndims; j++) pdims[j] = dims[p->dims[j]];
    ptr = dlite_instance_get_property_by_index(inst, i);
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
  if (i >= inst->meta->nproperties)
    return errx(-1, "no property with index %lu in %s", i, inst->meta->uri);
  dimensions = (size_t *)((char *)inst + inst->meta->dimoffset);
  return dimensions[i];
}


/*
  Returns a pointer to data for property `i` or NULL on error.

  The returned pointer points to the actual data and should not be
  dereferred for arrays.
 */
void *dlite_instance_get_property_by_index(const DLiteInstance *inst, size_t i)
{
  void *ptr;
  if (!inst->meta)
    return errx(-1, "no metadata available"), NULL;
  if (i >= inst->meta->nproperties)
    return errx(1, "index %lu exceeds number of properties (%lu) in %s",
		i, inst->meta->nproperties, inst->meta->uri), NULL;
  ptr = DLITE_PROP(inst, i);
  if (inst->meta->properties[i].ndims > 0)
    ptr = *(void **)ptr;
  return ptr;
}


/*
  Copies memory pointed to by `ptr` to property `i`.
  Returns non-zero on error.
*/
int dlite_instance_set_property_by_index(DLiteInstance *inst, size_t i,
					 const void *ptr)
{
  const DLiteMeta *meta = inst->meta;
  DLiteProperty *p = meta->properties + i;
  void *dest;

  if (p->ndims > 0) {
    int j;
    size_t n, nmemb=1;
    size_t *dims = DLITE_DIMS(inst);
    dest = *((void **)DLITE_PROP(inst, i));
    for (j=0; j<p->ndims; j++) nmemb *= dims[p->dims[j]];
    if (dlite_type_is_allocated(p->type)) {
      for (n=0; n<nmemb; n++) {
        void *v = (char *)ptr + n*p->size;
        if (!dlite_type_copy((char *)dest + n*p->size,
                             v, p->type, p->size)) return -1;
      }
    } else if (nmemb) {
      memcpy(dest, ptr, nmemb*p->size);
    }
  } else {
    dest = DLITE_PROP(inst, i);
    if (!dlite_type_copy(dest, ptr, p->type, p->size)) return -1;
  }
  return 0;
}

/*
  Returns number of dimensions of property with index `i` or -1 on error.
 */
int dlite_instance_get_property_ndims_by_index(const DLiteInstance *inst,
					       size_t i)
{
  const DLiteProperty *p;
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
  const DLiteProperty *p;
  size_t *dims;
  if (!inst->meta)
    return errx(-1, "no metadata available");
  dims = DLITE_DIMS(inst);
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
  if ((i = dlite_meta_get_dimension_index(inst->meta, name)) < 0) return -1;
  if (i >= (int)inst->meta->nproperties)
    return errx(-1, "no property with index %d in %s", i, inst->meta->uri);
  dimensions = (size_t *)((char *)inst + inst->meta->dimoffset);
  return dimensions[i];
}

/*
  Returns a pointer to data corresponding to `name` or NULL on error.
 */
void *dlite_instance_get_property(const DLiteInstance *inst, const char *name)
{
  int i;
  if (!inst->meta)
    return errx(-1, "no metadata available"), NULL;
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return NULL;
  return dlite_instance_get_property_by_index(inst, i);
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
  const DLiteProperty *p;
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
  DLiteInstance *e;
  char *name=NULL, *version=NULL, *namespace=NULL;
  size_t dims[] = {ndimensions, nproperties};

  if (dlite_split_meta_uri(uri, &name, &version, &namespace)) goto fail;
  if (!(e=dlite_instance_create((DLiteEntity *)dlite_EntitySchema, dims, uri)))
    goto fail;

  if (dlite_instance_set_property(e, "name", &name)) goto fail;
  if (dlite_instance_set_property(e, "version", &version)) goto fail;
  if (dlite_instance_set_property(e, "namespace", &namespace)) goto fail;
  if (dlite_instance_set_property(e, "description", &description)) goto fail;
  if (dlite_instance_set_property(e, "dimensions", dimensions)) goto fail;
  if (dlite_instance_set_property(e, "properties", properties)) goto fail;

  if (dlite_meta_init((DLiteMeta *)e)) goto fail;

  entity = (DLiteEntity *)e;
 fail:
  if (name) free(name);
  if (version) free(version);
  if (namespace) free(namespace);
  if (!entity) dlite_instance_decref(e);
  return entity;
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
  dlite_meta_decref((DLiteMeta *)entity);
}


/*
  Returns a new Entity loaded from storage `s`.  The `id` may be either
  an URI to the Entity (typically of the form "namespace/version/name")
  or an UUID.

  Returns NULL on error.
 */
DLiteEntity *dlite_entity_load(const DLiteStorage *s, const char *id)
{
  return (DLiteEntity *)
    dlite_instance_load(s, id, (DLiteEntity *)dlite_EntitySchema);
}


/*
  Saves an Entity to storage `s`.  Returns non-zero on error.
 */
int dlite_entity_save(DLiteStorage *s, const DLiteEntity *e)
{
  return dlite_instance_save(s, (DLiteInstance *)e);
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
  return (const DLiteProperty *)entity->properties + i;
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
  return (const DLiteProperty *)entity->properties + i;
}


/********************************************************************
 *  Meta data
 *
 *  These functions are mainly used internally or by code generators.
 *
 ********************************************************************/

/*
  Initialises internal data of `meta` describing its instances.

  Returns non-zero on error.
 */
int dlite_meta_init(DLiteMeta *meta)
{
  size_t i, size;
  int idim_dim=-1, idim_prop=-1, idim_rel=-1;
  int iprop_dim=-1, iprop_prop=-1, iprop_rel=-1;
  int ismeta = dlite_meta_is_metameta(meta);
  //size_t offset, *propoffsets;

  /* Initiate meta-metadata */
  if (!meta->meta->pooffset && dlite_meta_init((DLiteMeta *)meta->meta))
    goto fail;

  DEBUG("\n*** dlite_meta_init(\"%s\")\n", meta->uri);

  /* Assign: ndimensions, nproperties and nrelations */
  for (i=0; i<meta->meta->ndimensions; i++) {
    if (strcmp(meta->meta->dimensions[i].name, "ndimensions") == 0 ||
	strcmp(meta->meta->dimensions[i].name, "n-dimensions") == 0)
      idim_dim = i;
    if (strcmp(meta->meta->dimensions[i].name, "nproperties") == 0 ||
	strcmp(meta->meta->dimensions[i].name, "n-properties") == 0)
      idim_prop = i;
    if (strcmp(meta->meta->dimensions[i].name, "nrelations") == 0 ||
	strcmp(meta->meta->dimensions[i].name, "n-relations") == 0)
      idim_rel = i;
  }
  if (idim_dim < 0) return err(1, "dimensions are expected in metadata");
  if (!meta->ndimensions && idim_dim >= 0)
    meta->ndimensions = DLITE_DIM(meta, idim_dim);
  if (!meta->nproperties && idim_prop >= 0)
    meta->nproperties = DLITE_DIM(meta, idim_prop);
  if (meta->nrelations && idim_rel >= 0)
    meta->nrelations = DLITE_DIM(meta, idim_rel);

  /* Assign pointers: dimensions, properties and relations */
  for (i=0; i<meta->meta->nproperties; i++) {
    if (strcmp(meta->meta->properties[i].name, "dimensions") == 0)
      iprop_dim = i;
    if (strcmp(meta->meta->properties[i].name, "properties") == 0)
      iprop_prop = i;
    if (strcmp(meta->meta->properties[i].name, "relations") == 0)
      iprop_rel = i;
  }
  if (!meta->dimensions && meta->ndimensions && idim_dim >= 0)
    meta->dimensions = *(void **)DLITE_PROP(meta, iprop_dim);
  if (!meta->properties && meta->nproperties && idim_prop >= 0)
    meta->properties = *(void **)DLITE_PROP(meta, iprop_prop);
  if (!meta->relations && meta->nrelations && idim_rel >= 0)
    meta->relations = *(void **)DLITE_PROP(meta, iprop_rel);

  /* Assign headersize */
  if (!meta->headersize)
    meta->headersize = (ismeta) ? sizeof(DLiteMeta) : sizeof(DLiteInstance);
  DEBUG("    headersize=%lu\n", meta->headersize);

  /* Assign memory layout of instances */
  size = meta->headersize;

  /* -- dimension values (dimoffset) */
  if (meta->ndimensions) {
    size += padding_at(size_t, size);
    meta->dimoffset = size;
    size += meta->ndimensions * sizeof(size_t);
  }
  DEBUG("    dimoffset=%lu (+ %lu * %lu)\n",
         meta->dimoffset, meta->ndimensions, sizeof(size_t));

  /* -- property values (propoffsets[]) */
  meta->propoffsets = (size_t *)((char *)meta + meta->meta->pooffset);
  for (i=0; i<meta->nproperties; i++) {
    DLiteProperty *p = meta->properties + i;
    int padding;
    if (p->ndims) {
      padding = padding_at(void *, size);
      size += padding;
      meta->propoffsets[i] = size;
      size += sizeof(void *);
    } else {
      padding = dlite_type_padding_at(p->type, p->size, size);
      size += padding;
      meta->propoffsets[i] = size;
      size += p->size;
    }
    DEBUG("    propoffset[%lu]=%lu (pad=%d, type=%d size=%-2lu ndims=%d)"
          " + %lu\n",
          i, meta->propoffsets[i], padding, p->type, p->size, p->ndims,
          (p->ndims) ? sizeof(size_t *) : p->size);
  }
  /* -- relation values (reloffset) */
  if (meta->nrelations) {
    size += padding_at(void *, size);
    meta->reloffset = size;
    size += meta->nrelations * sizeof(void *);
  } else {
    meta->reloffset = size;
  }
  DEBUG("    reloffset=%lu (+ %lu * %lu)\n",
        meta->reloffset, meta->nrelations, sizeof(size_t));

  /* -- array of property offsets (pooffset) */
  size += padding_at(size_t, size);
  meta->pooffset = size;
  DEBUG("    pooffset=%lu\n", meta->pooffset);

  return 0;
 fail:
  return 1;
}


/*
  Increase reference count to metadata.
 */
void dlite_meta_incref(DLiteMeta *meta)
{
  dlite_instance_incref((DLiteInstance *)meta);
}

/*
  Decrease reference count to metadata.  If the reference count reaches
  zero, the metadata is free'ed.
 */
void dlite_meta_decref(DLiteMeta *meta)
{
  dlite_instance_decref((DLiteInstance *)meta);
}


/*
  Returns index of dimension named `name` or -1 on error.
 */
int dlite_meta_get_dimension_index(const DLiteMeta *meta, const char *name)
{
  size_t i;
  for (i=0; i<meta->ndimensions; i++) {
    DLiteDimension *d = meta->dimensions + i;
    if (strcmp(name, d->name) == 0) return i;
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
    DLiteProperty *p = meta->properties + i;
    if (strcmp(name, p->name) == 0) return i;
  }
  return err(-1, "%s has no such property: '%s'", meta->uri, name);
}


/*
  Returns a pointer to dimension with index `i` or NULL on error.
 */
const DLiteDimension *
dlite_meta_get_dimension_by_index(const DLiteMeta *meta, size_t i)
{
  return meta->dimensions + i;
}

/*
  Returns a pointer to dimension named `name` or NULL on error.
 */
const DLiteDimension *dlite_meta_get_dimension(const DLiteMeta *meta,
                                              const char *name)
{
  int i;
  if ((i = dlite_meta_get_dimension_index(meta, name)) < 0) return NULL;
  return meta->dimensions + i;
}


/*
  Returns a pointer to property with index \a i or NULL on error.
 */
const DLiteProperty *
dlite_meta_get_property_by_index(const DLiteMeta *meta, size_t i)
{
  return meta->properties + i;
}

/*
  Returns a pointer to property named `name` or NULL on error.
 */
const DLiteProperty *dlite_meta_get_property(const DLiteMeta *meta,
						   const char *name)
{
  int i;
  if ((i = dlite_meta_get_property_index(meta, name)) < 0) return NULL;
  return meta->properties + i;
}


/*
  Returns non-zero if `meta` is meta-metadata (i.e. its instances are
  metadata).

  @note If `meta` contains either a "properties" property (of type
  DLiteProperty) or a "relations" property (of type DLiteRelation) in
  addition to a "dimensions" property (of type DLiteDimension), then
  it is able to describe metadata and is considered to be meta-metadata.
  Otherwise it is not.
*/
int dlite_meta_is_metameta(const DLiteMeta *meta)
{
  int has_dimensions=0, has_properties=0;
  size_t i;
  for (i=0; i<meta->nproperties; i++) {
    DLiteProperty *p = meta->properties + i;
    if (p->type == dliteDimension &&
        (strcmp(p->name, "schema_dimensions") == 0 ||
         strcmp(p->name, "dimensions") == 0)) has_dimensions = 1;
    if (p->type == dliteProperty &&
        (strcmp(p->name, "schema_properties") == 0 ||
         strcmp(p->name, "properties") == 0)) has_properties = 1;
  }
  if (has_dimensions && has_properties) return 1;
  return 0;
}




/********************************************************************
 *  Metadata cache
 ********************************************************************/

static DLiteStore *_metastore = NULL;

static void metastore_create()
{
  if (!_metastore) {
    _metastore = dlite_store_create();
    atexit(dlite_metastore_free);
  dlite_metastore_add(dlite_BasicMetadataSchema);
  dlite_metastore_add(dlite_EntitySchema);
  dlite_metastore_add(dlite_CollectionSchema);
  }
}

/* Frees up a global metadata store.  Will be called at program exit,
   but can be called at any time. */
void dlite_metastore_free()
{
  if (_metastore) dlite_store_free(_metastore);
  _metastore = NULL;
}

/* Returns pointer to metadata for id `id` or NULL if `id` cannot be found. */
DLiteMeta *dlite_metastore_get(const char *id)
{
  if (!_metastore) metastore_create();
  assert(_metastore);
  return (DLiteMeta *)dlite_store_get(_metastore, id);
}

/* Adds metadata to global metadata store, giving away the ownership
   of `meta` to the store.  Returns non-zero on error. */
int dlite_metastore_add_new(const DLiteMeta *meta)
{
  DLiteInstance *inst;
  FILE *old;
  if (!_metastore) metastore_create();
  assert(_metastore);

  old = err_set_stream(NULL);
  inst = dlite_store_get(_metastore, meta->uuid);
  err_set_stream(old);

  if (inst)
    dlite_meta_decref((DLiteMeta *)meta);  // give away ownership
  else
    return dlite_store_add_new(_metastore, (DLiteInstance *)meta);
  return 0;
}

/* Adds metadata to global metadata store.  The caller keeps ownership
   of `meta`.  Returns non-zero on error. */
int dlite_metastore_add(const DLiteMeta *meta)
{
  dlite_instance_incref((DLiteInstance *)meta);
  return dlite_metastore_add_new(meta);
}
