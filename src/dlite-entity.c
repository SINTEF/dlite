#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "utils/err.h"
#include "utils/map.h"
#include "utils/fileutils.h"
#include "utils/infixcalc.h"

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-type.h"
#include "dlite-store.h"
#include "dlite-mapping.h"
#include "dlite-entity.h"
#include "dlite-datamodel.h"
#include "dlite-schemas.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) >= (y)) ? (x) : (y))


/* Forward declerations */
int dlite_meta_init(DLiteMeta *meta);
DLiteInstance *_instance_load_casted(const DLiteStorage *s, const char *id,
                                     const char *metaid, int lookup);



/********************************************************************
 *  Global in-memory instance store
 *
 *  This store holds (weak) references to all instances that have been
 *  instansiated in DLite.  The current implementation treats data
 *  instances and metadata different, by keeping a hard reference to
 *  metadata (by increasing its refcount) and a weak reference to data
 *  instances.  This makes metadata persistent, while allowing data
 *  instances to be free'ed when they are no longer needed.
 *
 *  It is still possible to remove metadata by explicitly decreasing
 *  the refcount hold by the instance store.
 ********************************************************************/

/* Forward declarations */
static void _instance_store_create(void);
static void _instance_store_free(void);
static int _instance_store_add(const DLiteInstance *inst);
static int _instance_store_remove(const char *id);
static DLiteInstance *_instance_store_get(const char *id);

typedef map_t(DLiteInstance *) instance_map_t;

/* Global store (hash table) with references to all instansiated
   instances in DLite. */
static instance_map_t *_instance_store = NULL;


/* Frees up a global instance store.  Will be called at program exit,
   but can be called at any time. */
static void _instance_store_create(void)
{
  if (!_instance_store) {
    _instance_store = malloc(sizeof(instance_map_t));
    map_init(_instance_store);
    atexit(_instance_store_free);
    _instance_store_add((DLiteInstance *)dlite_get_basic_metadata_schema());
    _instance_store_add((DLiteInstance *)dlite_get_entity_schema());
    _instance_store_add((DLiteInstance *)dlite_get_collection_schema());
  }
}

/* Frees up a global instance store.  Will be called at program exit,
   but can be called at any time. */
static void _instance_store_free(void)
{
  const char *uuid;
  map_iter_t iter;
  DLiteInstance **del=NULL;
  int i, ndel=0, delsize=0;
  if (!_instance_store) return;

  /* Remove all instances (to decrease the reference count for metadata) */
  iter = map_iter(_instance_store);
  while ((uuid = map_next(_instance_store, &iter))) {
    DLiteInstance *inst, **q;
    if ((q = map_get(_instance_store, uuid)) && (inst = *q) &&
        dlite_instance_is_meta(inst) && inst->_refcount > 0) {
      if (delsize <= ndel) {
        delsize += 64;
        del = realloc(del, delsize*sizeof(DLiteInstance *));
      }
      del[ndel++] = inst;
    }
  }

  for (i=0; i<ndel; i++) dlite_instance_decref(del[i]);
  if (del) free(del);

  map_deinit(_instance_store);
  free(_instance_store);
  _instance_store = NULL;
}

/* Adds instance to global instance store.  Returns zero on success, 1
   if instance is already in the store and a negative number of other errors.
*/
static int _instance_store_add(const DLiteInstance *inst)
{
  if (!_instance_store) _instance_store_create();
  assert(_instance_store);
  assert(inst);
  if (map_get(_instance_store, inst->uuid)) return 1;
  map_set(_instance_store, inst->uuid, (DLiteInstance *)inst);

  /* Increase reference  count for metadata that is kept in the store */
  if (dlite_instance_is_meta(inst))
    dlite_instance_incref((DLiteInstance *)inst);

  return 0;
}

/* Removes instance `uuid` from global instance store.  Returns non-zero
   on error.*/
static int _instance_store_remove(const char *uuid)
{
  DLiteInstance *inst, **q;
  if (!_instance_store)
    return errx(-1, "cannot remove %s from unallocated store", uuid);
  if (!(q = map_get(_instance_store, uuid)))
    return errx(-1, "cannot remove %s since it is not in store", uuid);
  inst = *q;
  map_remove(_instance_store, uuid);

  if (dlite_instance_is_meta(inst) && inst->_refcount > 0)
    dlite_instance_decref(inst);
  return 0;
}

/* Returns pointer to instance for id `id` or NULL if `id` cannot be found. */
static DLiteInstance *_instance_store_get(const char *id)
{
  int uuidver;
  char uuid[DLITE_UUID_LENGTH+1];
  DLiteInstance **instp;
  if (!_instance_store) _instance_store_create();
  if ((uuidver = dlite_get_uuid(uuid, id)) != 0 && uuidver != 5)
    return errx(1, "id '%s' is neither a valid UUID or a convertable string",
                id), NULL;
  if (!(instp = map_get(_instance_store, uuid))) return NULL;
    //return err(-1, "no such id in instance store: %s", id), NULL;
  return *instp;
}



/********************************************************************
 *  Instances
 ********************************************************************/

/*
  Prints instance to stdout. Intended for debugging.
 */
void dlite_instance_print(const DLiteInstance *inst)
{
  size_t i;
  int j;
  char *sep;
  FILE *fp = stdout;
  char *insttype =
    (dlite_instance_is_data(inst)) ? "Data" :
    (dlite_instance_is_metameta(inst)) ? "Meta-metadata" :
    (dlite_instance_is_meta(inst)) ? "Metadata" : "???";
  fprintf(fp, "\n");
  fprintf(fp, "%s instance (%p)\n", insttype, (void *)inst);
  fprintf(fp, "  _uuid: %s\n", inst->uuid);
  fprintf(fp, "  _uri: %s\n", inst->uri);
  fprintf(fp, "  _refcount: %d\n", inst->_refcount);
  fprintf(fp, "  _meta: (%p) %s\n", (void *)inst->meta, inst->meta->uri);
  fprintf(fp, "  _iri: %s\n", inst->iri);

  if (dlite_instance_is_meta(inst)) {
    DLiteMeta *meta = (DLiteMeta *)inst;
    fprintf(fp, "  _ndimensions: %lu\n", meta->_ndimensions);
    fprintf(fp, "  _nproperties: %lu\n", meta->_nproperties);
    fprintf(fp, "  _nrelations:  %lu\n", meta->_nrelations);

    fprintf(fp, "  _dimensions -> %p\n", (void *)meta->_dimensions);
    fprintf(fp, "  _properties -> %p\n", (void *)meta->_properties);
    fprintf(fp, "  _relations  -> %p\n", (void *)meta->_relations);

    fprintf(fp, "  _headersize: %lu\n", meta->_headersize);
    fprintf(fp, "  _init: %p\n", *(void **)&meta->_init);
    fprintf(fp, "  _deinit: %p\n", *(void **)&meta->_deinit);

    fprintf(fp, "  _npropdims: %lu\n", meta->_npropdims);
    fprintf(fp, "  _propdiminds -> %+ld:%p\n",
            (char *)meta->_propdiminds - (char *)inst,
            (void *)meta->_propdiminds);

    fprintf(fp, "  _dimoffset: %lu\n", meta->_dimoffset);
    fprintf(fp, "  _propoffsets -> %+ld:%p\n",
            (char *)meta->_propoffsets - (char *)inst,
            (void *)meta->_propoffsets);
    fprintf(fp, "  _reloffset: %lu\n", meta->_reloffset);
    fprintf(fp, "  _propdimsoffset: %lu\n", meta->_propdimsoffset);
    fprintf(fp, "  _propdimindsoffset: %lu\n", meta->_propdimindsoffset);
  }

  fprintf(fp, "  __dimensions(%+ld:%p):\n",
          inst->meta->_dimoffset,
          (void *)((char *)inst + inst->meta->_dimoffset));
  for (i=0; i < inst->meta->_ndimensions; i++)
    fprintf(fp, "    %2lu. %-12s (%+4ld:%p) %lu\n",
            i, inst->meta->_dimensions[i].name,
            inst->meta->_dimoffset + i*sizeof(size_t),
            (void *)((char *)inst + inst->meta->_dimoffset + i*sizeof(size_t)),
            DLITE_DIM(inst, i));

  fprintf(fp, "  __properties(+%lu:%p):\n",
          inst->meta->_propoffsets[0],
          (void *)((char *)inst + inst->meta->_propoffsets[0]));
  for (i=0; i < inst->meta->_nproperties; i++) {
    DLiteProperty *p = inst->meta->_properties + i;
    fprintf(fp, "    %2lu. %-12s (%+4ld:%p) %s:%lu [",
            i, p->name, inst->meta->_propoffsets[i], DLITE_PROP(inst, i),
            dlite_type_get_dtypename(p->type), p->size);
    for (j=0, sep=""; j < p->ndims; j++, sep=", ")
      fprintf(fp, "%s%s=%lu", sep, p->dims[j], DLITE_PROP_DIM(inst, i, j));
    fprintf(fp, "]\n");
  }

  fprintf(fp, "  __relations(%+ld:%p):\n",
          inst->meta->_reloffset,
          (void *)((char *)inst + inst->meta->_reloffset));
  for (i=0; i<inst->meta->_nrelations; i++) {
    DLiteRelation *r = DLITE_REL(inst, i);
    fprintf(fp, "    %lu. (%s, %s, %s) : %s\n", i, r->s, r->p, r->o, r->id);
  }

  fprintf(fp, "  __propdims(%+ld:%p): [",
          inst->meta->_propdimsoffset,
          (void *)((char *)inst + inst->meta->_propdimsoffset));
  for (i=0, sep=""; i < inst->meta->_npropdims; i++, sep=", ") {
    size_t *propdims = (size_t *)((char *)inst + inst->meta->_propdimsoffset);
    fprintf(fp, "%s%lu", sep, propdims[i]);
  }
  fprintf(fp, "]\n");

  if (dlite_instance_is_meta(inst)) {
    DLiteMeta *meta = (DLiteMeta *)inst;
    fprintf(fp, "  __propdiminds(%+ld:%p): [",
            inst->meta->_propdimindsoffset,
            (void *)((char *)inst + inst->meta->_propdimindsoffset));
    for (i=0, sep=""; i < meta->_nproperties; i++, sep=", ") {
      fprintf(fp, "%s%lu", sep, meta->_propdiminds[i]);
    }
    fprintf(fp, "]\n");

    fprintf(fp, "  __propoffsets(%+ld:%p): [",
            DLITE_PROPOFFSETSOFFSET(inst),
            (void *)((char *)inst + DLITE_PROPOFFSETSOFFSET(inst)));
    for (i=0, sep=""; i < meta->_nproperties; i++, sep=", ") {
      size_t *propoffsets =
        (size_t *)((char *)inst + DLITE_PROPOFFSETSOFFSET(inst));
      fprintf(fp, "%s%lu", sep, propoffsets[i]);
    }
    fprintf(fp, "]\n");
  }
}


/*
  Help function that evaluates array of instance property dimension
  values, where `dims` is the instance dimensions.  It assigns the
  `propdims` memory section of `inst`.

  Returns non-zero in error.
 */
static int _instance_propdims_eval(DLiteInstance *inst, const size_t *dims)
{
  int retval = 1;
  const DLiteMeta *meta = inst->meta;
  size_t *propdims = (size_t *)((char *)inst + meta->_propdimsoffset);
  size_t i, n=0;
  InfixCalcVariable *vars=NULL;

  if (!(vars = calloc(meta->_ndimensions, sizeof(InfixCalcVariable))))
    FAIL("allocation failure");
  for (i=0; i < meta->_ndimensions; i++) {
    vars[i].name = meta->_dimensions[i].name;
    vars[i].value = dims[i];
  }

  for (i=0; i < meta->_nproperties; i++) {
    DLiteProperty *p = meta->_properties + i;
    int j;
    char errmsg[256] = "";
    for (j=0; j < p->ndims; j++)
      propdims[n++] = infixcalc(p->dims[j], vars, meta->_ndimensions,
                                errmsg, sizeof(errmsg));
    if (errmsg[0]) FAIL1("invalid property dimension expression: %s", errmsg);
  }
  assert(n == meta->_npropdims);

  retval = 0;
 fail:
  if (vars) free(vars);
  return retval;
}


/*
  Help function for dlite_instance_create().  If `lookup` is true,
  a check will be done to see if the instance already exists.
 */
static DLiteInstance *_instance_create(const DLiteMeta *meta,
                                       const size_t *dims,
                                       const char *id, int lookup)
{
  char uuid[DLITE_UUID_LENGTH+1];
  size_t i, size;
  DLiteInstance *inst=NULL;
  int j, uuid_version;

  /* Check if we are trying to create an instance with an already
     existing id. */
  if (lookup && id && (inst = _instance_store_get(id))) {
    dlite_instance_incref(inst);
    warn("trying to create new instance with id '%s' - creates a new "
        "reference instead (refcount=%d)", id, inst->_refcount);
    return inst;
  }

  /* Make sure that metadata is initialised */
  if (!meta->_propoffsets && dlite_meta_init((DLiteMeta *)meta)) goto fail;
  if (_instance_store_add((DLiteInstance *)meta) < 0) goto fail;

  /* Allocate instance */
  size = meta->_propdimindsoffset;
  if (dlite_meta_is_metameta(meta)) {
    size_t nproperties;
    if ((j = dlite_meta_get_dimension_index(meta, "nproperties")) < 0)
      goto fail;
    nproperties = dims[j];
    size += 2*nproperties*sizeof(size_t);
  }
  size += padding_at(DLiteInstance, size);  /* add final padding */

  if (!(inst = calloc(1, size))) FAIL("allocation failure");
  dlite_instance_incref(inst);  /* increase refcount of the new instance */

  /* Initialise header */
  if ((uuid_version = dlite_get_uuid(uuid, id)) < 0) goto fail;
  memcpy(inst->uuid, uuid, sizeof(uuid));
  if (uuid_version == 5) inst->uri = strdup(id);
  inst->meta = (DLiteMeta *)meta;

  /* Set dimensions */
  if (meta->_ndimensions) {
    size_t *dimensions = (size_t *)((char *)inst + meta->_dimoffset);
    memcpy(dimensions, dims, meta->_ndimensions*sizeof(size_t));
  }

  /* Additional initialisation */
  if (meta->_init && meta->_init(inst)) goto fail;
  if (_instance_propdims_eval(inst, dims)) goto fail;

  /* Allocate arrays for dimensional properties */
  for (i=0; i<meta->_nproperties; i++) {
    DLiteProperty *p = DLITE_PROP_DESCR(inst, i);
    void **ptr = DLITE_PROP(inst, i);
    if (p->ndims > 0 && p->dims) {
      size_t nmemb=1, size=p->size;
      for (j=0; j<p->ndims; j++)
        nmemb *= DLITE_PROP_DIM(inst, i, j);
      if (nmemb > 0) {
        if (!(*ptr = calloc(nmemb, size))) goto fail;
      } else {
        *ptr = NULL;
      }
    }
  }

  /* Add to instance cache */
  if (_instance_store_add(inst)) goto fail;

  /* Increase reference counts */
  dlite_meta_incref((DLiteMeta *)meta);  /* increase refcount of metadata */

  return inst;
 fail:
  if (inst) {
    /* `dlite_instance_decref(inst)` will decrease the reference count
       to the metadata, but on failure we haven't increased it yet, so
       we have to do it now... */
    if (inst->meta) dlite_meta_incref((DLiteMeta *)inst->meta);
    dlite_instance_decref(inst);
  }
  return NULL;
}

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
DLiteInstance *dlite_instance_create(const DLiteMeta *meta,
                                     const size_t *dims,
                                     const char *id)
{
  return _instance_create(meta, dims, id, 1);
}


/*
  Like dlite_instance_create() but takes the uri or uuid if the
  metadata as the first argument.  `dims`.  The lengths of `dims` is
  found in `meta->ndims`.

  On error, NULL is returned.
*/
DLiteInstance *dlite_instance_create_from_id(const char *metaid,
                                             const size_t *dims,
                                             const char *id)
{
  DLiteMeta *meta;
  DLiteInstance *inst=NULL;
  if (!(meta = (DLiteMeta *)dlite_instance_get(metaid)))
    FAIL1("cannot find metadata '%s'", metaid);
  inst = dlite_instance_create(meta, dims, id);
 fail:
  if (meta) dlite_meta_decref(meta);
  return inst;
}


/*
  Free's an instance and all arrays associated with dimensional properties.
 */
static void dlite_instance_free(DLiteInstance *inst)
{
  size_t i, nprops;
  const DLiteMeta *meta = inst->meta;
  assert(meta);

  /* Additional deinitialisation */
  if (meta->_deinit) meta->_deinit(inst);

  /* Remove from instance cache */
  _instance_store_remove(inst->uuid);

  /* Standard free */
  nprops = meta->_nproperties;
  if (inst->uri) free((char *)inst->uri);
  if (inst->iri) free((char *)inst->iri);
  if (meta->_properties) {
    for (i=0; i<nprops; i++) {
      DLiteProperty *p = (DLiteProperty *)meta->_properties + i;
      void *ptr = DLITE_PROP(inst, i);
      if (p->ndims > 0 && p->dims) {
        if (dlite_type_is_allocated(p->type)) {
          int j;
          size_t n, nmemb=1;
          for (j=0; j<p->ndims; j++)
            nmemb *= DLITE_PROP_DIM(inst, i, j);
          for (n=0; n<nmemb; n++)
            dlite_type_clear(*(char **)ptr + n*p->size, p->type, p->size);
        }
        free(*(void **)ptr);
      } else {
        dlite_type_clear(ptr, p->type, p->size);
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
  DEBUG_LOG("+++ incref: %2d -> %2d : %s\n",
            inst->_refcount, inst->_refcount+1,
            (inst->uri) ? inst->uri : inst->uuid);
  return ++inst->_refcount;
}

/*
  Decrease reference count to `inst`.  If the reference count reaches
  zero, the instance is free'ed.

  Returns the new reference count.
 */
int dlite_instance_decref(DLiteInstance *inst)
{
  int count;
  DEBUG_LOG("--- decref: %2d -> %2d : %s\n",
            inst->_refcount, inst->_refcount-1,
            (inst->uri) ? inst->uri : inst->uuid);
  assert(inst->_refcount > 0);
  if ((count = --inst->_refcount) <= 0) dlite_instance_free(inst);
  return count;
}


/*
  Returns a new reference to instance with given `id` or NULL if no such
  instance can be found.
*/
DLiteInstance *dlite_instance_get(const char *id)
{
  DLiteInstance *inst=NULL;
  DLiteStoragePathIter *iter;
  const char *url;

  /* check if instance `id` is already instansiated... */
  if ((inst = _instance_store_get(id))) {
    dlite_instance_incref(inst);
    return inst;
  }

  /* ...otherwise look it up in storages */
  if (!(iter = dlite_storage_paths_iter_start())) return NULL;

  assert(iter);
  while ((url = dlite_storage_paths_iter_next(iter))) {
    DLiteStorage *s;
    char *copy, *driver, *location, *options;

    if (!(copy = strdup(url))) return err(1, "allocation failure"), NULL;
#ifdef _WIN32
    /* Hack: on Window, don't interpreat the "C" in urls starting with
       "C:\" or "C:/" as a driver, but rather as a part of the location...

       The concequence of this is that we cannot have a driver named
       "C" on Windows.
    */
    dlite_split_url_winpath(copy, &driver, &location, &options, NULL, 1);
#else
    dlite_split_url(copy, &driver, &location, &options, NULL);
#endif

    /* If driver is not given, infer it from file extension */
    if (!driver) driver = (char *)fu_fileext(location);

    /* Set read-only as default mode (all drivers should support this) */
    if (!options) options = "mode=r";
    if ((s = dlite_storage_open(driver, location, options))) {
      /* url is a storage we can open... */
      ErrTry:
        inst = _instance_load_casted(s, id, NULL, 0);
      ErrCatch(dliteStorageLoadError):  // suppressed error
        break;
      ErrEnd;
      dlite_storage_close(s);
    } else {
      /* ...otherwise it may be a glob pattern */
      FUIter *fiter;
      if ((fiter = fu_glob(location))) {
        const char *path;
        while (!inst && (path = fu_globnext(fiter))) {
	  driver = (char *)fu_fileext(path);
	  if ((s = dlite_storage_open(driver, path, options))) {
            ErrTry:
              inst = _instance_load_casted(s, id, NULL, 0);
            ErrCatch(dliteStorageLoadError):  // suppressed error
              break;
            ErrEnd;
	    dlite_storage_close(s);
	  }
        }
        fu_globend(fiter);
      }
    }
    free(copy);
    if (inst) {
      dlite_storage_paths_iter_stop(iter);
      return inst;
    }
  }
  dlite_storage_paths_iter_stop(iter);
  return NULL;
}

/*
  Like dlite_instance_get(), but maps the instance with the given id
  to an instance of `metaid`.  If `metaid` is NULL, it falls back to
  dlite_instance_get().  Returns NULL on error.
 */
DLiteInstance *dlite_instance_get_casted(const char *id, const char *metaid)
{
  DLiteInstance *inst, *instances;
  if (!(inst = dlite_instance_get(id))) return NULL;
  if (metaid) {
    instances = dlite_mapping(metaid, (const DLiteInstance **)&inst, 1);
    dlite_instance_decref(inst);
  } else {
    instances = inst;
  }
  return instances;
}

/*
  Loads instance identified by `id` from storage `s` and returns a
  new and fully initialised dlite instance.

  In case the storage only contains one instance, it is possible to
  set `id` to NULL.  However, it is an error to set `id` to NULL if the
  storage contains more than one instance.

  On error, NULL is returned.
 */
DLiteInstance *dlite_instance_load(const DLiteStorage *s, const char *id)
{
  return dlite_instance_load_casted(s, id, NULL);
}

/*
  A convinient function that loads an instance given an URL of the form

      driver://location?options#id

  where `location` corresponds to the `uri` argument of
  dlite_storage_open().  If `location` is not given, the instance is
  loaded from the metastore using `id`.

  Returns the instance or NULL on error.
 */
DLiteInstance *dlite_instance_load_url(const char *url)
{
  char *str=NULL, *driver=NULL, *location=NULL, *options=NULL, *id=NULL;
  DLiteStorage *s=NULL;
  DLiteInstance *inst=NULL;
  assert(url);
  if (!(str = strdup(url))) FAIL("allocation failure");
  if (dlite_split_url(str, &driver, &location, &options, &id)) goto fail;
  if (id && (inst = _instance_store_get(id))) {
    dlite_instance_incref(inst);
  } else {
    err_clear();
    if (!(s = dlite_storage_open(driver, location, options))) goto fail;
    if (!(inst = dlite_instance_load(s, id))) goto fail;
  }
 fail:
  if (s) dlite_storage_close(s);
  if (str) free(str);
  return inst;
}

/*
  Help function for dlite_instance_load_casted().

  Some storages accept that `id` is NULL if the storage only contain
  one instance.  In that case that instance is returned.

  If `lookup` is non-zero, a check will be done to see if the instance
  already exists.  This is the normal case.
 */
DLiteInstance *_instance_load_casted(const DLiteStorage *s, const char *id,
                                     const char *metaid, int lookup)
{
  DLiteMeta *meta;
  DLiteInstance *inst=NULL, *instance=NULL;
  DLiteDataModel *d=NULL;
  size_t i, *dims=NULL;
  const char *uri=NULL;

  /* check if id is already loaded */
  if (lookup && id && (inst = _instance_store_get(id))) {
    dlite_instance_incref(inst);
    warn("trying to load existing instance from storage \"%s\": %s"
         " - creates a new reference", s->uri, id);
    return inst;
  }

  /* check if storage implements the instance api */
  if (s->api->loadInstance) {
    if (!(inst = s->api->loadInstance(s, id))) goto fail;
    if (metaid)
      return dlite_mapping(metaid, (const DLiteInstance **)&inst, 1);
    else
      return inst;
  }

  /* create datamodel and get metadata uri */
  if (!(d = dlite_datamodel(s, id))) goto fail;
  if (!id || !*id) id = d->uuid;

  if (!(uri = dlite_datamodel_get_meta_uri(d))) goto fail;

  /* If metadata is not given, try to load it from cache... */
  meta = (DLiteMeta *)dlite_instance_get(uri);

  /* ...otherwise try to load it from storage */
  if (!meta) {
    char uuid[DLITE_UUID_LENGTH];
    dlite_get_uuid(uuid, uri);
    meta = (DLiteMeta *)dlite_instance_load(s, uuid);
  }

  /* ...otherwise give up */
  if (!meta) FAIL1("cannot load metadata: %s", uri);

  /* Make sure that metadata is initialised */
  if (dlite_meta_init(meta)) goto fail;

  /* check metadata uri */
  if (strcmp(uri, meta->uri) != 0)
    FAIL3("metadata uri (%s) does not correspond to that in storage (%s): %s",
	  meta->uri, uri, s->uri);

  /* read dimensions */
  if (!(dims = calloc(meta->_ndimensions, sizeof(size_t))))
    FAIL("allocation failure");
  for (i=0; i<meta->_ndimensions; i++)
    if ((int)(dims[i] =
         dlite_datamodel_get_dimension_size(d, meta->_dimensions[i].name)) < 0)
      goto fail;

  /* Create instance
     This increases the refcount of meta, but we already own a reference
     to meta that we want to hand over to `inst`.  Therefore, decrease
     the additional refcount after calling dlite_instance_create()...
   */
  if (!(inst = _instance_create(meta, dims, id, lookup))) goto fail;
  dlite_meta_decref(meta);

  /* assign properties */
  for (i=0; i<meta->_nproperties; i++) {
    DLiteProperty *p = (DLiteProperty *)meta->_properties + i;
    void *ptr = (void *)dlite_instance_get_property_by_index(inst, i);
    size_t *pdims = DLITE_PROP_DIMS(inst, i);
    if (dlite_datamodel_get_property(d, p->name, ptr, p->type, p->size,
				     p->ndims, pdims)) goto fail;
  }

  /* initiates metadata of the new instance is metadata */
  if (dlite_meta_is_metameta(inst->meta) && dlite_meta_init((DLiteMeta *)inst))
    goto fail;

  if (!inst->uri) {
    if (dlite_meta_is_metameta(inst->meta)) {
      char **name = dlite_instance_get_property(inst, "name");
      char **version = dlite_instance_get_property(inst, "version");
      char **namespace = dlite_instance_get_property(inst, "namespace");
      if (name && version && namespace) {
        inst->uri = dlite_join_meta_uri(*name, *version, *namespace);
        dlite_get_uuid(inst->uuid, inst->uri);
      } else {
        FAIL2("metadata %s loaded from %s has no name, version and namespace",
             id, s->uri);
      }
    //} else {
    //  char **dataname;
    //  ErrTry:
    //    dataname = dlite_instance_get_property(inst, "dataname");
    //    break;
    //  ErrOther:
    //    break;
    //  ErrEnd;
    //  if (dataname) {
    //    inst->uri = strdup(*dataname);
    //    dlite_get_uuid(inst->uuid, inst->uri);
    //  }
    }
  }

  /* cast if `metaid` is not NULL */
  if (inst && metaid)
    instance = dlite_mapping(metaid, (const DLiteInstance **)&inst, 1);
  else
    instance = inst;

 fail:
  if (!inst && !err_geteval())
    err(1, "cannot load id '%s' from storage '%s'", id, s->uri);
  if (!instance && inst) dlite_instance_decref(inst);
  if (d) dlite_datamodel_free(d);
  if (uri) free((char *)uri);
  if (dims) free(dims);
  err_update_eval(dliteStorageLoadError);
  return instance;
}

/*
  Like dlite_instance_load(), but allows casting the loaded instance
  into an instance of metadata identified by `metaid`.  If `metaid` is
  NULL, no casting is performed.

  For the cast to be successful requires that the correct mappings
  have been registered.

  Returns NULL of error or if no mapping can be found.
 */
DLiteInstance *dlite_instance_load_casted(const DLiteStorage *s,
                                          const char *id,
                                          const char *metaid)
{
  return _instance_load_casted(s, id, metaid, 1);
}

/*
  Saves instance `inst` to storage `s`.  Returns non-zero on error.
 */
int dlite_instance_save(DLiteStorage *s, const DLiteInstance *inst)
{
  int retval=1;
  DLiteDataModel *d=NULL;
  const DLiteMeta *meta;
  size_t i, *dims;

  if (!(meta = inst->meta)) return errx(-1, "no metadata available");

  /* check if storage implements the instance api */
  if (s->api->saveInstance)
    return s->api->saveInstance(s, inst);

  /* proceede with the datamodel api... */
  if (!(d = dlite_datamodel(s, inst->uuid))) goto fail;
  if (dlite_datamodel_set_meta_uri(d, meta->uri)) goto fail;

  dims = (size_t *)((char *)inst + inst->meta->_dimoffset);
  for (i=0; i<meta->_ndimensions; i++) {
    char *dimname = inst->meta->_dimensions[i].name;
    if (dlite_datamodel_set_dimension_size(d, dimname, dims[i])) goto fail;
  }

  for (i=0; i<meta->_nproperties; i++) {
    DLiteProperty *p = (DLiteProperty *)inst->meta->_properties + i;
    const void *ptr = dlite_instance_get_property_by_index(inst, i);
    size_t *pdims = DLITE_PROP_DIMS(inst, i);
    if (dlite_datamodel_set_property(d, p->name, ptr, p->type, p->size,
				     p->ndims, pdims)) goto fail;
  }
  retval = 0;
 fail:
  if (d) dlite_datamodel_free(d);
  return retval;
}

/*
  A convinient function that saves instance `inst` to the storage specified
  by `url`, which should be of the form

      driver://location?options

  Returns non-zero on error.
 */
int dlite_instance_save_url(const char *url, const DLiteInstance *inst)
{
  int retval;
  char *str=NULL, *driver=NULL, *loc=NULL, *options=NULL;
  DLiteStorage *s=NULL;
  if (!(str = strdup(url))) FAIL("allocation failure");
  if (dlite_split_url(str, &driver, &loc, &options, NULL)) goto fail;
  if (!(s = dlite_storage_open(driver, loc, options))) goto fail;
  retval = dlite_instance_save(s, inst);
 fail:
  if (s) dlite_storage_close(s);
  if (str) free(str);
  return retval;
}


/*
  Returns true if instance has a dimension with the given name.
 */
bool dlite_instance_has_dimension(DLiteInstance *inst, const char *name)
{
  size_t i;
  for (i=0; i < inst->meta->_ndimensions; i++)
    if (strcmp(inst->meta->_dimensions[i].name, name) == 0) return true;
  return false;
}


/*
  Returns number of dimensions or -1 on error.
 */
int dlite_instance_get_ndimensions(const DLiteInstance *inst)
{
  if (!inst->meta)
    return errx(-1, "no metadata available");
  return inst->meta->_ndimensions;
}


/*
  Returns number of properties or -1 on error.
 */
int dlite_instance_get_nproperties(const DLiteInstance *inst)
{
  if (!inst->meta)
    return errx(-1, "no metadata available");
  return inst->meta->_nproperties;
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
  if (i >= inst->meta->_nproperties)
    return errx(-1, "no property with index %zu in %s", i, inst->meta->uri);
  dimensions = (size_t *)((char *)inst + inst->meta->_dimoffset);
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
  if (i >= inst->meta->_nproperties)
    return errx(1, "index %zu exceeds number of properties (%zu) in %s",
		i, inst->meta->_nproperties, inst->meta->uri), NULL;
  ptr = DLITE_PROP(inst, i);
  if (inst->meta->_properties[i].ndims > 0)
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
  DLiteProperty *p = meta->_properties + i;
  void *dest;

  if (p->ndims > 0) {
    int j;
    size_t n, nmemb=1;
    dest = *((void **)DLITE_PROP(inst, i));
    for (j=0; j<p->ndims; j++) nmemb *= DLITE_PROP_DIM(inst, i, j);
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
  if (!inst->meta)
    return errx(-1, "no metadata available");
  if (!(p = dlite_meta_get_property_by_index(inst->meta, i)))
    return -1;
  if (j >= (size_t)p->ndims)
    return errx(-1, "dimension index j=%zu is our of range", j);
  return DLITE_PROP_DIM(inst, i, j);
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
  if (i >= (int)inst->meta->_nproperties)
    return errx(-1, "no property with index %d in %s", i, inst->meta->uri);
  dimensions = (size_t *)((char *)inst + inst->meta->_dimoffset);
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
  Returns true if instance has a property with the given name.
 */
bool dlite_instance_has_property(DLiteInstance *inst, const char *name)
{
  size_t i;
  for (i=0; i < inst->meta->_nproperties; i++)
    if (strcmp(inst->meta->_properties[i].name, name) == 0) return true;
  return false;
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

/*
  Returns non-zero if `inst` is a data instance.
 */
int dlite_instance_is_data(const DLiteInstance *inst)
{
  if (!dlite_meta_is_metameta(inst->meta)) return 1;
  return 0;
}

/*
  Returns non-zero if `inst` is metadata.

  This is simply the inverse of dlite_instance_is_data().
 */
int dlite_instance_is_meta(const DLiteInstance *inst)
{
  if (dlite_meta_is_metameta(inst->meta)) return 1;
  return 0;
}

/**
  Returns non-zero if `inst` is meta-metadata.

  Meta-metadata contains either a "properties" property (of type
  DLiteProperty) or a "relations" property (of type DLiteRelation) in
  addition to a "dimensions" property (of type DLiteDimension).
 */
int dlite_instance_is_metameta(const DLiteInstance *inst)
{
  if (dlite_meta_is_metameta(inst->meta) &&
      dlite_meta_is_metameta((DLiteMeta *)inst)) return 1;
  return 0;
}


/*
  Updates the size of all dimensions in `inst`.  The new dimension
  sizes are provided in `dims`, which must be of length
  `inst->_ndimensions`.  Dimensions corresponding to negative elements
  in `dims` will remain unchanged.

  All properties whos dimension are changed will be reallocated and
  new memory will be zeroed.  The values of properties with two or
  more dimensions, where any but the first dimension is updated,
  should be considered invalidated.

  Returns non-zero on error.
 */
int dlite_instance_set_dimension_sizes(DLiteInstance *inst, const int *dims)
{
  int retval=1, i;
  size_t n;
  size_t *xdims=NULL;
  size_t *oldpropdims=NULL;
  int *oldmembs=NULL;

  if (!dlite_instance_is_data(inst))
    return err(1, "it is not possible to change dimensions of metadata");

  if (!(xdims = calloc(inst->meta->_ndimensions, sizeof(size_t))))
    FAIL("Allocation failure");
  for (n=0; n < inst->meta->_ndimensions; n++)
    xdims[n] = (dims[n] >= 0) ? (size_t)dims[n] : DLITE_DIM(inst, n);

  /* save old propdims and property members (oldmembs) */
  if (!(oldpropdims = calloc(inst->meta->_npropdims, sizeof(size_t))))
    FAIL("Allocation failure");
  memcpy(oldpropdims, DLITE_PROP_DIMS(inst, 0),
         inst->meta->_npropdims * sizeof(size_t));

  if (!(oldmembs = calloc(inst->meta->_nproperties, sizeof(int))))
    FAIL("Allocation failure");
  for (n=0; n < inst->meta->_nproperties; n++) {
    DLiteProperty *p = inst->meta->_properties + n;
    oldmembs[n] = 1;
    for (i=0; i < p->ndims; i++)
      oldmembs[n] *= DLITE_PROP_DIM(inst, n, i);
  }

  /* evaluate new propdims */
  if (_instance_propdims_eval(inst, xdims)) goto fail;

  /* reallocate properties */
  for (n=0; n < inst->meta->_nproperties; n++) {
    DLiteProperty *p = inst->meta->_properties + n;
    int newmembs=1, oldsize, newsize;
    void **ptr = DLITE_PROP(inst, n);
    if (p->ndims <= 0) continue;
    for (i=0; i < p->ndims; i++)
      newmembs *= DLITE_PROP_DIM(inst, n, i);

    oldsize = oldmembs[n] * p->size;
    newsize = newmembs * p->size;
    if (newmembs == oldmembs[n]) {
      continue;
    } else if (newmembs > 0) {
      void *q;
      if (newmembs < oldmembs[n])
        for (i=newmembs; i < oldmembs[n]; i++)
          dlite_type_clear((char *)(*ptr) + i*p->size, p->type, p->size);
      if (!(*ptr = realloc((q = *ptr), newsize))) {
        if (q) free(q);
        return err(1, "error reallocating '%s' to size %d", p->name, newsize);
      }
      if (newmembs > oldmembs[n])
        memset((char *)(*ptr) + oldsize, 0, newsize - oldsize);
    } else if (*ptr) {
      for (i=0; i < oldmembs[n]; i++)
        dlite_type_clear((char *)(*ptr) + i*p->size, p->type, p->size);
      free(*ptr);
      *ptr = NULL;
    } else {
      assert(oldsize == 0);
    }
  }

  /* update dimensions */
  for (n=0; n < inst->meta->_ndimensions; n++)
    if (dims[n] >= 0) DLITE_DIM(inst, n) = dims[n];

  retval = 0;
 fail:
  if (retval && oldpropdims)
    memcpy(DLITE_PROP_DIMS(inst, 0), oldpropdims,
           inst->meta->_npropdims * sizeof(size_t));
  if (xdims) free(xdims);
  if (oldpropdims) free(oldpropdims);
  if (oldmembs) free(oldmembs);
  return retval;
}

/*
  Like dlite_instance_set_dimension_sizes(), but only updates the size of
  dimension `i` to size `size`.  Returns non-zero on error.
 */
int dlite_instance_set_dimension_size_by_index(DLiteInstance *inst,
                                               size_t i, size_t size)
{
  size_t j;
  int retval;
  int *dims = malloc(inst->meta->_ndimensions * sizeof(int));
  for (j=0; j < inst->meta->_ndimensions; j++) dims[j] = -1;
  dims[i] = size;
  retval = dlite_instance_set_dimension_sizes(inst, dims);
  free(dims);
  return retval;
}

/*
  Like dlite_instance_set_dimension_sizes(), but only updates the size of
  dimension `name` to size `size`.  Returns non-zero on error.
 */
int dlite_instance_set_dimension_size(DLiteInstance *inst, const char *name,
                                      size_t size)
{
  int i;
  if ((i = dlite_meta_get_dimension_index(inst->meta, name)) < 0) return -1;
  return dlite_instance_set_dimension_size_by_index(inst, i, size);
}


/*
  Copies instance `inst` to a newly created instance.

  If `newid` is NULL, the new instance will have no URI and a random UUID.
  If `newid` is a valid UUID, the new instance will have the given
  UUID and no URI.
  Otherwise, the URI of the new instance will be `newid` and the UUID
  assigned accordingly.

  Returns NULL on error.
 */
DLiteInstance *dlite_instance_copy(const DLiteInstance *inst, const char *newid)
{
  DLiteInstance *new=NULL;
  size_t n;
  int i;
  if (!(new = dlite_instance_create(inst->meta, DLITE_DIMS(inst), newid)))
    return NULL;
  for (n=0; n < inst->meta->_nproperties; n++) {
    DLiteProperty *p = inst->meta->_properties + n;
    void *src = dlite_instance_get_property_by_index(inst, n);
    void *dst = dlite_instance_get_property_by_index(new, n);
   if (p->ndims > 0) {
      int nmembs=1;
      for (i=0; i < p->ndims; i++)
        nmembs *= DLITE_PROP_DIM(inst, n, i);
      for (i=0; i < nmembs; i++)
        if (!dlite_type_copy((char *)dst + i*p->size,
                             (char *)src + i*p->size,
                             p->type, p->size)) goto fail;
    } else {
      if (!dlite_type_copy(dst, src, p->type, p->size)) goto fail;
    }
  }
  return new;
 fail:
  if (new) dlite_instance_decref(new);
  return NULL;
}


/*
  Returns a new DLiteArray object for property number `i` in instance `inst`.

  The returned array object only describes, but does not own the
  underlying array data, which remains owned by the instance.

  Scalars are treated as a one-dimensional array or length one.

  Returns NULL on error.
 */
DLiteArray *
dlite_instance_get_property_array_by_index(const DLiteInstance *inst, size_t i)
{
  void *ptr;
  int ndims=1, dim=1, *dims=&dim;
  DLiteProperty *p = DLITE_PROP_DESCR(inst, i);
  DLiteArray *arr = NULL;
  if (!(ptr = dlite_instance_get_property_by_index(inst, i))) goto fail;
  if (p->ndims > 0) {
    int j;
    if (!(dims = malloc(p->ndims*sizeof(size_t)))) goto fail;
    ndims = p->ndims;
    for (j=0; j < p->ndims; j++)
      dims[j] = DLITE_PROP_DIM(inst, i, j);
  }
  arr = dlite_array_create(ptr, p->type, p->size, ndims, dims);
 fail:
  if (dims && dims != &dim) free(dims);
  return arr;
}


/*
  Returns a new DLiteArray object for property `name` in instance `inst`.

  The returned array object only describes, but does not own the
  underlying array data, which remains owned by the instance.

  Scalars are treated as a one-dimensional array or length one.

  Returns NULL on error.
 */
DLiteArray *dlite_instance_get_property_array(const DLiteInstance *inst,
                                              const char *name)
{
  int i;
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return NULL;
  return dlite_instance_get_property_array_by_index(inst, i);
}




/********************************************************************
 *  Metadata
 ********************************************************************/

/*
  Returns a new Entity created from the given arguments.
 */
DLiteMeta *
dlite_entity_create(const char *uri, const char *iri,
                    const char *description,
		    size_t ndimensions, const DLiteDimension *dimensions,
		    size_t nproperties, const DLiteProperty *properties)
{
  DLiteMeta *entity=NULL;
  DLiteInstance *e=NULL;
  char *name=NULL, *version=NULL, *namespace=NULL;
  size_t dims[] = {ndimensions, nproperties};

  if ((e = dlite_instance_get(uri)))
    return (DLiteMeta *)e;
  if (dlite_split_meta_uri(uri, &name, &version, &namespace)) goto fail;
  if (!(e=dlite_instance_create(dlite_get_entity_schema(), dims, uri)))
    goto fail;

  if (iri) e->iri = strdup(iri);
  if (dlite_instance_set_property(e, "name", &name)) goto fail;
  if (dlite_instance_set_property(e, "version", &version)) goto fail;
  if (dlite_instance_set_property(e, "namespace", &namespace)) goto fail;
  if (dlite_instance_set_property(e, "description", &description)) goto fail;
  if (dlite_instance_set_property(e, "dimensions", dimensions)) goto fail;
  if (dlite_instance_set_property(e, "properties", properties)) goto fail;

  if (dlite_meta_init((DLiteMeta *)e)) goto fail;

  entity = (DLiteMeta *)e;
 fail:
  if (name) free(name);
  if (version) free(version);
  if (namespace) free(namespace);
  if (!entity && e) dlite_instance_decref(e);
  return entity;
}

/*
  Initialises internal data of metadata `meta`.

  Returns non-zero on error.
 */
int dlite_meta_init(DLiteMeta *meta)
{
  size_t i, size;
  int j;
  int idim_dim=-1, idim_prop=-1, idim_rel=-1;
  int iprop_dim=-1, iprop_prop=-1, iprop_rel=-1;
  int ismeta = dlite_meta_is_metameta(meta);

  /* Initiate meta-metadata */
  if (!meta->meta->_dimoffset && dlite_meta_init((DLiteMeta *)meta->meta))
    goto fail;

  DEBUG_LOG("\n*** dlite_meta_init(\"%s\")\n", meta->uri);

  /* Assign: ndimensions, nproperties and nrelations */
  for (i=0; i<meta->meta->_ndimensions; i++) {
    if (strcmp(meta->meta->_dimensions[i].name, "ndimensions") == 0 ||
	strcmp(meta->meta->_dimensions[i].name, "n-dimensions") == 0)
      idim_dim = i;
    if (strcmp(meta->meta->_dimensions[i].name, "nproperties") == 0 ||
	strcmp(meta->meta->_dimensions[i].name, "n-properties") == 0)
      idim_prop = i;
    if (strcmp(meta->meta->_dimensions[i].name, "nrelations") == 0 ||
	strcmp(meta->meta->_dimensions[i].name, "n-relations") == 0)
      idim_rel = i;
  }
  if (idim_dim < 0) return err(1, "dimensions are expected in metadata");
  if (!meta->_ndimensions && idim_dim >= 0)
    meta->_ndimensions = DLITE_DIM(meta, idim_dim);
  if (!meta->_nproperties && idim_prop >= 0)
    meta->_nproperties = DLITE_DIM(meta, idim_prop);
  if (meta->_nrelations && idim_rel >= 0)
    meta->_nrelations = DLITE_DIM(meta, idim_rel);

  /* Assign pointers: dimensions, properties and relations */
  for (i=0; i<meta->meta->_nproperties; i++) {
    if (strcmp(meta->meta->_properties[i].name, "dimensions") == 0)
      iprop_dim = i;
    if (strcmp(meta->meta->_properties[i].name, "properties") == 0)
      iprop_prop = i;
    if (strcmp(meta->meta->_properties[i].name, "relations") == 0)
      iprop_rel = i;
  }
  if (!meta->_dimensions && meta->_ndimensions && idim_dim >= 0)
    meta->_dimensions = *(void **)DLITE_PROP(meta, iprop_dim);
  if (!meta->_properties && meta->_nproperties && idim_prop >= 0)
    meta->_properties = *(void **)DLITE_PROP(meta, iprop_prop);
  if (!meta->_relations && meta->_nrelations && idim_rel >= 0)
    meta->_relations = *(void **)DLITE_PROP(meta, iprop_rel);

  /* Assign headersize */
  if (!meta->_headersize)
    meta->_headersize = (ismeta) ? sizeof(DLiteMeta) : sizeof(DLiteInstance);
  DEBUG_LOG("    headersize=%zu\n", meta->_headersize);

  /* Assign property dimension sizes */
  meta->_npropdims = 0;
  for (i=0; i < meta->_nproperties; i++)
    meta->_npropdims += meta->_properties[i].ndims;
  meta->_propdiminds =
    (size_t *)((char *)meta + meta->meta->_propdimindsoffset);
  for (i=j=0; i < meta->_nproperties; i++) {
    meta->_propdiminds[i] = j;
    j += meta->_properties[i].ndims;
  }
  _instance_propdims_eval((DLiteInstance *)meta, DLITE_DIMS(meta));

  /* Assign memory layout of instances */
  size = meta->_headersize;

  /* -- dimension values (dimoffset) */
  if (meta->_ndimensions) {
    size += padding_at(size_t, size);
    meta->_dimoffset = size;
    size += meta->_ndimensions * sizeof(size_t);
  }
  DEBUG_LOG("    dimoffset=%zu (+ %zu * %zu)\n",
         meta->_dimoffset, meta->_ndimensions, sizeof(size_t));

  /* -- property values (propoffsets[]) */
  meta->_propoffsets = (size_t *)((char *)meta + DLITE_PROPOFFSETSOFFSET(meta));
  for (i=0; i<meta->_nproperties; i++) {
    DLiteProperty *p = meta->_properties + i;
    if (p->ndims) {
      size += padding_at(void *, size);
      meta->_propoffsets[i] = size;
      size += sizeof(void *);
    } else {
      size += dlite_type_padding_at(p->type, p->size, size);
      meta->_propoffsets[i] = size;
      size += p->size;
    }
    DEBUG_LOG("    propoffset[%zu]=%zu (pad=%d, type=%d size=%-2lu ndims=%d)"
          " + %zu\n",
          i, meta->_propoffsets[i], padding, p->type, p->size, p->ndims,
          (p->ndims) ? sizeof(size_t *) : p->size);
  }
  /* -- relation values (reloffset) */
  if (meta->_nrelations) {
    size += padding_at(void *, size);
    meta->_reloffset = size;
    size += meta->_nrelations * sizeof(void *);
  } else {
    meta->_reloffset = size;
  }
  DEBUG_LOG("    reloffset=%zu (+ %zu * %zu)\n",
        meta->_reloffset, meta->_nrelations, sizeof(size_t));

  /* -- property dimension values (propdimsoffset) */
  size += padding_at(size_t, size);
  meta->_propdimsoffset = size;
  size += meta->_npropdims * sizeof(size_t);

  /* -- first property dimension indices (propdimsoffset) */
  size += padding_at(size_t, size);
  meta->_propdimindsoffset = size;
  size += meta->_nproperties * sizeof(size_t);
  DEBUG_LOG("    propdimindsoffset=%zu\n", meta->_propdimindsoffset);

  /* -- total size */
  size += padding_at(size_t, size);
  size += meta->_nproperties * sizeof(size_t);
  size += padding_at(size_t, size);
  DEBUG_LOG("    size=%zu\n", size);

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
  Returns a new reference to metadata with given `id` or NULL if no such
  instance can be found.
*/
DLiteMeta *dlite_meta_get(const char *id)
{
  return (DLiteMeta *)dlite_instance_get(id);
}


/*
  Loads metadata identified by `id` from storage `s` and returns a new
  fully initialised meta instance.
*/
DLiteMeta *dlite_meta_load(const DLiteStorage *s, const char *id)
{
  return (DLiteMeta *)dlite_instance_load(s, id);
}

/*
  Saves metadata `meta` to storage `s`.  Returns non-zero on error.
 */
int dlite_meta_save(DLiteStorage *s, const DLiteMeta *meta)
{
  return dlite_instance_save(s, (const DLiteInstance *)meta);
}

/*
  Returns index of dimension named `name` or -1 on error.
 */
int dlite_meta_get_dimension_index(const DLiteMeta *meta, const char *name)
{
  size_t i;
  for (i=0; i<meta->_ndimensions; i++) {
    DLiteDimension *d = meta->_dimensions + i;
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
  for (i=0; i<meta->_nproperties; i++) {
    DLiteProperty *p = meta->_properties + i;
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
  /*
  if (i < 0) i += meta->ndimensions;
  if (i < 0 || i >= meta->ndimensions)
  */
  if (i >= meta->_ndimensions)
    return err(-1, "invalid dimension index %lu", i), NULL;
  return meta->_dimensions + i;
}

/*
  Returns a pointer to dimension named `name` or NULL on error.
 */
const DLiteDimension *dlite_meta_get_dimension(const DLiteMeta *meta,
                                              const char *name)
{
  int i;
  if ((i = dlite_meta_get_dimension_index(meta, name)) < 0) return NULL;
  return meta->_dimensions + i;
}


/*
  Returns a pointer to property with index \a i or NULL on error.
 */
const DLiteProperty *
dlite_meta_get_property_by_index(const DLiteMeta *meta, size_t i)
{
  return meta->_properties + i;
}

/*
  Returns a pointer to property named `name` or NULL on error.
 */
const DLiteProperty *dlite_meta_get_property(const DLiteMeta *meta,
						   const char *name)
{
  int i;
  if ((i = dlite_meta_get_property_index(meta, name)) < 0) return NULL;
  return meta->_properties + i;
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
  int has_dimensions=0, has_properties=0, has_relations=0;
  size_t i;
  for (i=0; i<meta->_nproperties; i++) {
    DLiteProperty *p = meta->_properties + i;
    if (p->type == dliteDimension && strcmp(p->name, "dimensions") == 0)
      has_dimensions = 1;
    if (p->type == dliteProperty && strcmp(p->name, "properties") == 0)
      has_properties = 1;
    if (p->type == dliteRelation && strcmp(p->name, "relations") == 0)
      has_relations = 1;
  }
  if (has_dimensions && (has_properties || has_relations)) return 1;
  return 0;
}
