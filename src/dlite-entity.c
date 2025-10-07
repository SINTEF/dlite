#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "utils/compat.h"
#include "utils/err.h"
#include "utils/map.h"
#include "utils/fileutils.h"
#include "utils/strutils.h"
#include "utils/infixcalc.h"
#include "utils/sha3.h"
#include "utils/rng.h"

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


/* Forward declarations */
int dlite_meta_init(DLiteMeta *meta);
DLiteInstance *_instance_load_casted(const DLiteStorage *s, const char *id,
                                     const char *metaid, int lookup);



/********************************************************************
 *  Global in-memory instance store
 *
 *  This store holds (weak) references to all instances that have been
 *  instantiated in DLite.  The current implementation treats data
 *  instances and metadata different, by keeping a hard reference to
 *  metadata (by increasing its refcount) and a weak reference to data
 *  instances.  This makes metadata persistent, while allowing data
 *  instances to be free'ed when they are no longer needed.
 *
 *  It is still possible to remove metadata by explicitly decreasing
 *  the refcount hold by the instance store.
 ********************************************************************/

typedef map_t(DLiteInstance *) instance_map_t;

/* Forward declarations */
static instance_map_t *_instance_store(void);
static void _instance_store_free(void *instance_store);
static int _instance_store_add(const DLiteInstance *inst);
static int _instance_store_remove(const char *id);
static DLiteInstance *_instance_store_get(const char *id);


/* Help function for adding metadata */
static void _instance_store_addmeta(instance_map_t *istore,
                                    const DLiteMeta *meta)
{
  int stat = map_set(istore, meta->uuid, (DLiteInstance *)meta);
  assert(stat == 0);
  dlite_instance_incref((DLiteInstance *)meta);
}

/* Returns pointer to instance store. */
static instance_map_t *_instance_store(void)
{
  instance_map_t *istore = dlite_globals_get_state("dlite-instance-store");
  if (!istore) {
    if (!(istore = malloc(sizeof(instance_map_t))))
      return err(dliteMemoryError, "allocation failure"), NULL;
    map_init(istore);
    _instance_store_addmeta(istore, dlite_get_basic_metadata_schema());
    _instance_store_addmeta(istore, dlite_get_entity_schema());
    _instance_store_addmeta(istore, dlite_get_collection_entity());
    dlite_globals_add_state("dlite-instance-store", istore,
                            _instance_store_free);
  }
  return istore;
}

/* Frees up a global instance store.  Will be called at program exit,
   but can be called at any time. */
static void _instance_store_free(void *instance_store)
{
  instance_map_t *istore = instance_store;
  const char *uuid;
  map_iter_t iter;
  DLiteInstance **del=NULL;
  int i, ndel=0, delsize=0;
  assert(istore);

  /* Remove all instances (to decrease the reference count for metadata) */
  iter = map_iter(istore);
  while ((uuid = map_next(istore, &iter))) {
    DLiteInstance *inst, **q;
    if ((q = map_get(istore, uuid)) && (inst = *q) &&
        dlite_instance_is_meta(inst) && inst->_refcount > 0) {
      if (delsize <= ndel) {
        void *ptr;
        delsize += 64;
        if (!(ptr = realloc(del, delsize*sizeof(DLiteInstance *))))
          free(del);
        del = ptr;
      }
      if (del) del[ndel++] = inst;
    }
  }

  if (del) {
    for (i=0; i<ndel; i++) dlite_instance_decref(del[i]);
    free(del);
  }
  map_deinit(istore);
  free(istore);
}

/* Adds instance to global instance store.  Returns zero on success, 1
   if instance is already in the store and a negative number of other errors.
*/
static int _instance_store_add(const DLiteInstance *inst)
{
  instance_map_t *istore = _instance_store();
  assert(istore);
  assert(inst);
  if (map_get(istore, inst->uuid)) return 1;
  map_set(istore, inst->uuid, (DLiteInstance *)inst);

  /* Increase reference  count for metadata that is kept in the store */
  if (dlite_instance_is_meta(inst))
    dlite_instance_incref((DLiteInstance *)inst);

  return 0;
}

/* Removes instance `uuid` from global instance store.  Returns non-zero
   on error.*/
static int _instance_store_remove(const char *uuid)
{
  instance_map_t *istore = _instance_store();
  DLiteInstance *inst, **q;
  assert(istore);
  if (!(q = map_get(istore, uuid)))
    return errx(dliteMissingInstanceError, "cannot remove %s since it is not in store", uuid);
  inst = *q;
  map_remove(istore, uuid);

  if (dlite_instance_is_meta(inst) && inst->_refcount > 0)
    dlite_instance_decref(inst);
  return 0;
}

/* Returns pointer to instance for id `id` or NULL if `id` cannot be found. */
static DLiteInstance *_instance_store_get(const char *id)
{
  instance_map_t *istore = _instance_store();
  DLiteIdType idtype;
  char uuid[DLITE_UUID_LENGTH+1];
  DLiteInstance **instp;
  if ((idtype = dlite_get_uuid(uuid, id)) < 0 || idtype == dliteIdRandom)
    return errx(dliteValueError,
                "id '%s' is neither a valid UUID or a convertable string",
                id), NULL;
  if (!(instp = map_get(istore, uuid))) return NULL;
  return *instp;
}



/********************************************************************
 *  Framework internals and debugging
 ********************************************************************/

/*
  Prints instance to stdout. Intended for debugging.
 */
void dlite_instance_debug(const DLiteInstance *inst)
{
  size_t i;
  int j;
  char *sep;
  FILE *fp = stdout;
  char *insttype =
    (dlite_instance_is_data(inst)) ? "Data" :
    (dlite_instance_is_metameta(inst)) ? "Meta-metadata" :
    (dlite_instance_is_meta(inst)) ? "Metadata" : "???";

  dlite_instance_sync_to_dimension_sizes((DLiteInstance *)inst);

  fprintf(fp, "\n");
  fprintf(fp, "%s instance (%p)\n", insttype, (void *)inst);
  fprintf(fp, "  _uuid: %s\n", inst->uuid);
  fprintf(fp, "  _uri: %s\n", inst->uri);
  fprintf(fp, "  _refcount: %d\n", inst->_refcount);
  fprintf(fp, "  _meta: (%p) %s\n", (void *)inst->meta, inst->meta->uri);

  if (dlite_instance_is_meta(inst)) {
    DLiteMeta *meta = (DLiteMeta *)inst;
    fprintf(fp, "  _ndimensions: %lu\n", (unsigned long)meta->_ndimensions);
    fprintf(fp, "  _nproperties: %lu\n", (unsigned long)meta->_nproperties);
    fprintf(fp, "  _nrelations:  %lu\n", (unsigned long)meta->_nrelations);

    fprintf(fp, "  _dimensions -> %p\n", (void *)meta->_dimensions);
    fprintf(fp, "  _properties -> %p\n", (void *)meta->_properties);
    fprintf(fp, "  _relations  -> %p\n", (void *)meta->_relations);

    fprintf(fp, "  _headersize: %lu\n", (unsigned long)meta->_headersize);
    fprintf(fp, "  _init: %p\n", *(void **)&meta->_init);
    fprintf(fp, "  _deinit: %p\n", *(void **)&meta->_deinit);

    fprintf(fp, "  _npropdims: %lu\n", (unsigned long)meta->_npropdims);
    fprintf(fp, "  _propdiminds -> %+d:%p\n",
            (int)((char *)meta->_propdiminds - (char *)inst),
            (void *)meta->_propdiminds);

    fprintf(fp, "  _dimoffset: %lu\n", (unsigned long)meta->_dimoffset);
    fprintf(fp, "  _propoffsets -> %+d:%p\n",
            (int)((char *)meta->_propoffsets - (char *)inst),
            (void *)meta->_propoffsets);
    fprintf(fp, "  _reloffset: %lu\n", (unsigned long)meta->_reloffset);
    fprintf(fp, "  _propdimsoffset: %lu\n",
            (unsigned long)meta->_propdimsoffset);
    fprintf(fp, "  _propdimindsoffset: %lu\n",
            (unsigned long)meta->_propdimindsoffset);
  }

  fprintf(fp, "  __dimensions(%+d:%p):\n",
          (int)inst->meta->_dimoffset,
          (void *)((char *)inst + inst->meta->_dimoffset));
  for (i=0; i < inst->meta->_ndimensions; i++)
    fprintf(fp, "    %2lu. %-12s (%+4d:%p) %lu\n",
            (unsigned long)i, inst->meta->_dimensions[i].name,
            (int)(inst->meta->_dimoffset + i*sizeof(size_t)),
            (void *)((char *)inst + inst->meta->_dimoffset + i*sizeof(size_t)),
            (unsigned long)DLITE_DIM(inst, i));

  fprintf(fp, "  __properties(+%lu:%p):\n",
          (unsigned long)inst->meta->_propoffsets[0],
          (void *)((char *)inst + inst->meta->_propoffsets[0]));
  for (i=0; i < inst->meta->_nproperties; i++) {
    DLiteProperty *p = inst->meta->_properties + i;
    fprintf(fp, "    %2lu. %-12s (%+4d:%p) %s:%lu [",
            (unsigned long)i,
            p->name,
            (int)inst->meta->_propoffsets[i],
            DLITE_PROP(inst, i),
            dlite_type_get_dtypename(p->type),
            (unsigned long)p->size);
    for (j=0, sep=""; j < p->ndims; j++, sep=", ")
      fprintf(fp, "%s%s=%lu", sep, p->shape[j],
              (unsigned long)DLITE_PROP_DIM(inst, i, j));
    fprintf(fp, "]\n");
  }

  fprintf(fp, "  __relations(%+d:%p):\n",
          (int)inst->meta->_reloffset,
          (void *)((char *)inst + inst->meta->_reloffset));
  for (i=0; i<inst->meta->_nrelations; i++) {
    DLiteRelation *r = DLITE_REL(inst, i);
    fprintf(fp, "    %d. (%s, %s, %s) : %s\n", (int)i, r->s, r->p, r->o, r->id);
  }

  fprintf(fp, "  __propdims(%+d:%p): [",
          (int)inst->meta->_propdimsoffset,
          (void *)((char *)inst + inst->meta->_propdimsoffset));
  for (i=0, sep=""; i < inst->meta->_npropdims; i++, sep=", ") {
    size_t *propdims = (size_t *)((char *)inst + inst->meta->_propdimsoffset);
    fprintf(fp, "%s%lu", sep, (unsigned long)propdims[i]);
  }
  fprintf(fp, "]\n");

  if (dlite_instance_is_meta(inst)) {
    DLiteMeta *meta = (DLiteMeta *)inst;
    fprintf(fp, "  __propdiminds(%+d:%p): [",
            (int)inst->meta->_propdimindsoffset,
            (void *)((char *)inst + inst->meta->_propdimindsoffset));
    if (meta->_propdiminds)
      for (i=0, sep=""; i < meta->_nproperties; i++, sep=", ")
        fprintf(fp, "%s%lu", sep, (unsigned long)meta->_propdiminds[i]);
    else
      fprintf(fp, "(nil)");
    fprintf(fp, "]\n");

    fprintf(fp, "  __propoffsets(%+d:%p): [",
            (int)DLITE_PROPOFFSETSOFFSET(inst),
            (void *)((char *)inst + DLITE_PROPOFFSETSOFFSET(inst)));
    if (meta->_propoffsets) {
      for (i=0, sep=""; i < meta->_nproperties; i++, sep=", ") {
        size_t *propoffsets =
          (size_t *)((char *)inst + DLITE_PROPOFFSETSOFFSET(inst));
        fprintf(fp, "%s%lu", sep, (unsigned long)propoffsets[i]);
      }
    } else
      fprintf(fp, "(nil)");
    fprintf(fp, "]\n");
  }
}

/*
  Returns the allocated size of an instance with metadata `meta` and
  dimensions `dims`.  The length of `dims` is given by
  ``meta->_ndimensions``.  Mostly intended for internal use.

  Returns zero on error.
 */
size_t dlite_instance_size(const DLiteMeta *meta, const size_t *dims)
{
  int j;
  size_t size = meta->_propdimindsoffset;
  if (dlite_meta_is_metameta(meta)) {
    size_t nproperties;
    if ((j = dlite_meta_get_dimension_index(meta, "nproperties")) < 0)
      return 0;
    nproperties = dims[j];
    size += 2*nproperties*sizeof(size_t);
  }
  size += padding_at(DLiteInstance, size);  /* add final padding */
  return size;
}

/* Return of newly allocated NULL-terminated array of string pointers
   with uuids available in the internal storage (istore) */
char** dlite_istore_get_uuids(int* nuuids)
{
  instance_map_t* istore = _instance_store();
  assert(istore);
  const char* uuid;
  map_iter_t iter;

  iter = map_iter(istore);
  *nuuids = 0;
  while ((uuid = map_next(istore, &iter))) (*nuuids)++;

  char** uuids;
  if (!(uuids = calloc((*nuuids + 1), sizeof(char*))))
    FAILCODE(dliteMemoryError, "allocation failure");

  int i = 0;
  iter = map_iter(istore);
  while ((uuid = map_next(istore, &iter))) {
    if (!(uuids[i] = malloc(DLITE_UUID_LENGTH + 1)))
      FAILCODE(dliteMemoryError, "allocation failure");
    strcpy(uuids[i], uuid);
    i++;
  }
  uuids[i] = NULL;
  return uuids;

 fail:
  if (uuids) {
    for (i=0; uuid[i]; i++) free(uuids[i]);
    free(uuids);
  }
  return NULL;
}

/********************************************************************
 *  Instances
 ********************************************************************/

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
    FAILCODE(dliteMemoryError, "allocation failure");
  for (i=0; i < meta->_ndimensions; i++) {
    vars[i].name = meta->_dimensions[i].name;
    vars[i].value = dims[i];
  }

  for (i=0; i < meta->_nproperties; i++) {
    DLiteProperty *p = meta->_properties + i;
    int j;
    char errmsg[256] = "";
    for (j=0; j < p->ndims; j++)
      propdims[n++] = infixcalc(p->shape[j], vars, meta->_ndimensions,
                                errmsg, sizeof(errmsg));
    if (errmsg[0]) FAILCODE1(dliteSyntaxError, "invalid property dimension expression: %s", errmsg);
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
  int j, idtype;

  /* Check if we are trying to create an instance with an already
     existing id. */
  if (lookup && id && *id && (inst = _instance_store_get(id))) {
    dlite_instance_incref(inst);
    warn("trying to create new instance with id '%s' - creates a new "
        "reference instead (refcount=%d)", id, inst->_refcount);

    /* Check that `dims` corresponds to the dims of the existing instance. */
    for (i=0; i < meta->_ndimensions; i++) {
      if (dims[i] != dlite_instance_get_dimension_size_by_index(inst, i))
        FAILCODE3(dliteIndexError, "mismatch of dimension %d. Trying to create with size %d "
              "but existing instance has size %d", (int)i, (int)dims[i],
              (int)dlite_instance_get_dimension_size_by_index(inst, i));
    }
    return inst;
  }

  /* Make sure that metadata is initialised */
  if (!meta->_propoffsets && dlite_meta_init((DLiteMeta *)meta)) goto fail;
  if (_instance_store_add((DLiteInstance *)meta) < 0) goto fail;

  /* Allocate instance */
  if (!(size = dlite_instance_size(meta, dims))) goto fail;
  if (!(inst = calloc(1, size))) FAILCODE(dliteMemoryError, "allocation failure");
  dlite_instance_incref(inst);  /* increase refcount of the new instance */

  /* Initialise header */
  if ((idtype = dlite_get_uuid(uuid, id)) < 0) goto fail;
  memcpy(inst->uuid, uuid, sizeof(uuid));
  if (idtype == dliteIdHash) inst->uri = strdup(id);
  inst->meta = (DLiteMeta *)meta;

  /* Set dimensions */
  if (meta->_ndimensions) {
    size_t *dimensions = DLITE_DIMS(inst);
    memcpy(dimensions, dims, meta->_ndimensions*sizeof(size_t));
  }

  /* Evaluate property dimensions */
  if (_instance_propdims_eval(inst, dims)) goto fail;

  /* Allocate arrays for dimensional properties */
  for (i=0; i<meta->_nproperties; i++) {
    DLiteProperty *p = DLITE_PROP_DESCR(inst, i);
    void **ptr = DLITE_PROP(inst, i);
    if (p->ndims > 0 && p->shape) {
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

  /* Further initialisation if metadata..,  */
  if (dlite_meta_is_metameta(meta) && dlite_meta_init((DLiteMeta *)inst))
    goto fail;

  /* Initialisation of extended metadata.
     Note that we do not call _setdim() and _loadprop() here.  If
     needed, they should be called by _init(). */
  if (meta->_init && meta->_init(inst)) goto fail;

  /* Add to instance cache */
  if (_instance_store_add(inst)) goto fail;

  /* Increase reference count of metadata */
  dlite_meta_incref((DLiteMeta *)meta);

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
    FAILCODE1(dliteMissingMetadataError, "cannot find metadata '%s'", metaid);
  inst = dlite_instance_create(meta, dims, id);
 fail:
  if (meta) dlite_meta_decref(meta);
  return inst;
}


/*
  Free's an instance and all arrays associated with dimensional properties.
 */
static int dlite_instance_free(DLiteInstance *inst)
{
  size_t i, nprops;
  int stat;
  const DLiteMeta *meta = inst->meta;
  assert(meta);

  /* Additional deinitialisation */
  if (meta->_deinit) meta->_deinit(inst);

  /* Remove from instance cache */
  stat = _instance_store_remove(inst->uuid);

  /* For transactions, decrease refcount of parent */
  if (inst->_parent) {
    if (inst->_parent->parent)
      dlite_instance_decref((DLiteInstance *)(inst->_parent->parent));
    free(inst->_parent);
  }

  /* Standard free */
  nprops = meta->_nproperties;
  if (inst->uri) free((char *)inst->uri);
  if (meta->_properties) {
    for (i=0; i<nprops; i++) {
      DLiteProperty *p = (DLiteProperty *)meta->_properties + i;
      void *ptr = DLITE_PROP(inst, i);
      if (p->ndims > 0 && p->shape) {
        if (dlite_type_is_allocated(p->type)) {
          int j;
          size_t n, nmemb=1;
          char *memptr = *(char **)ptr;
          for (j=0; j<p->ndims; j++)
            nmemb *= DLITE_PROP_DIM(inst, i, j);
          if (memptr)
            for (n=0; n<nmemb; n++)
              dlite_type_clear(memptr + n*p->size, p->type, p->size);
        }
        free(*(void **)ptr);
      } else {
        dlite_type_clear(ptr, p->type, p->size);
      }
    }
  }
  free(inst);

  dlite_meta_decref((DLiteMeta *)meta);  /* decrease metadata refcount */
  return stat;
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
  if ((count = --inst->_refcount) == 0) {
      if (dlite_instance_free(inst) == -1) return -1;
  }
  return count;
}


/*
  Checks whether an instance with the given `id` exists.

  If `check_storages` is true, the storage plugin path is searched if
  the instance is not found in the in-memory store.  This is
  equivalent to dlite_instance_get(), except that a borrowed
  reference is returned and no error is reported if the instance
  cannot be found.

  Returns a pointer to the instance (borrowed reference) if it exists
  or NULL otherwise.
 */
DLiteInstance *dlite_instance_has(const char *id, bool check_storages)
{
  DLiteInstance *inst;
  if (!(inst = _instance_store_get(id)) && check_storages) {
    ErrTry:
      if ((inst = dlite_instance_get(id))) {
        dlite_instance_decref(inst);
        assert(inst->_refcount > 0);
      }
    ErrOther:
      break;
    ErrEnd;
  }
  return inst;
}


/*
  Returns a new reference to instance with given `id` or NULL if no such
  instance can be found.

  If the instance exists in the in-memory store it is returned (with
  its refcount increased by one).  Otherwise it is searched for in the
  storage plugin path (initiated from the DLITE_STORAGES environment
  variable).

  It is an error message if the instance cannot be found.
*/
DLiteInstance *dlite_instance_get(const char *id)
{
  DLiteInstance *inst=NULL;
  DLiteStorageHotlistIter hiter;
  const DLiteStorage *hs;
  DLiteStoragePathIter *iter;
  const char *url;

  /* check if instance `id` is already instantiated... */
  if ((inst = _instance_store_get(id))) {
    dlite_instance_incref(inst);
    return inst;
  }

  /* ...otherwise look it up in hotlisted storages */
  dlite_storage_hotlist_iter_init(&hiter);
  while ((hs = dlite_storage_hotlist_iter_next(&hiter))) {
    DLiteInstance *inst;
    ErrTry:
      inst = _instance_load_casted(hs, id, NULL, 0);
    ErrCatch(dliteStorageLoadError):  // suppressed error
      break;  // breaks ErrCatch, not the while loop
    ErrEnd;
    if (inst) {
      dlite_storage_hotlist_iter_deinit(&hiter);
      return inst;
    }
  }
  dlite_storage_hotlist_iter_deinit(&hiter);

  /* ...otherwise look it up in storages */
  if (!(iter = dlite_storage_paths_iter_start())) return NULL;

  assert(iter);
  while ((url = dlite_storage_paths_iter_next(iter))) {
    DLiteStorage *s;
    char *copy, *driver, *location, *options;

    if (!(copy = strdup(url))) {
      err(dliteMemoryError, "allocation failure");
      break;
    }
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
    if (!driver || !*driver) driver = (char *)fu_fileext(location);

    /* Set read-only as default mode (all drivers should support this) */
    if (!options) options = "mode=r";
    ErrTry:
      s = dlite_storage_open(driver, location, options);
    ErrCatch(dliteStorageOpenError):  // suppressed error
    ErrCatch(dliteStorageLoadError):  // suppressed error
      break;
    ErrEnd;
    if (s) {

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
      if ((fiter = fu_glob(location, "|"))) {
        const char *path;
        while (!inst && (path = fu_globnext(fiter))) {
	  driver = (char *)fu_fileext(path);
          ErrTry:
            s = dlite_storage_open(driver, path, options);
          ErrCatch(dliteStorageOpenError):  // suppressed error
            break;
          ErrEnd;
          if (s) {
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

  /* Try to fetch the instance from http://onto-ns.com/ */
  if (strncmp(id, "http://", 7) == 0 || strncmp(id, "https://", 8) == 0) {
    static const char *saved_id = NULL;
    if (!(saved_id && strcmp(saved_id, id) == 0)) {
      /* FIXME: There is a small chance for a race-condition when used
         in a multi-threaded environment. Add thread synchronisation here! */
      saved_id = id;
      ErrTry:
        inst = dlite_instance_load_loc("http", id, NULL, NULL);
      ErrCatch(dliteStorageOpenError):  // suppressed error
        break;
      ErrEnd;
      saved_id = NULL;
      if (inst) return inst;
    }
  }

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
  A convenient function for loading an instance with given id from a
  storage specified with the `driver`, `location` and `options`
  arguments (see dlite_storage_open()).

  The `id` argument may be NULL if the storage contains only one instance.

  Returns the instance or NULL on error.
 */
DLiteInstance *dlite_instance_load_loc(const char *driver, const char *location,
                                       const char *options, const char *id)
{
  DLiteStorage *s=NULL;
  DLiteInstance *inst=NULL;
 ErrTry:
  if (id && *id) inst = _instance_store_get(id);
 ErrOther:
  err_clear();
 ErrEnd;

 if (inst) {
   dlite_instance_incref(inst);
  } else {
    if (!(s = dlite_storage_open(driver, location, options))) goto fail;
    if (!(inst = dlite_instance_load(s, id))) goto fail;
  }
 fail:
  if (s) dlite_storage_close(s);
  return inst;
}

/*
  A convenient function that loads an instance given an URL of the form

      driver://location?options#id

  where `location` corresponds to the `uri` argument of
  dlite_storage_open().  If `location` is not given, the instance is
  loaded from the metastore using `id`.

  Returns the instance or NULL on error.
 */
DLiteInstance *dlite_instance_load_url(const char *url)
{
  char *str=NULL, *driver=NULL, *location=NULL, *options=NULL, *id=NULL;
  DLiteInstance *inst=NULL;
  assert(url);
  if (!(str = strdup(url))) FAILCODE(dliteMemoryError, "allocation failure");
  if (dlite_split_url(str, &driver, &location, &options, &id)) goto fail;
  inst = dlite_instance_load_loc(driver, location, options, id);

 fail:
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

  if (!s) FAILCODE(dliteStorageLoadError,
                   "invalid storage, see previous errors");

  /* check if id is already loaded */
  if (lookup && id && *id && (inst = _instance_store_get(id))) {
    dlite_instance_incref(inst);
    //warn("trying to load existing instance from storage \"%s\": %s"
    //     " - create a new reference", s->location, id);
    return inst;
  }

  /* check if storage implements the instance api */
  if (s->api->loadInstance) {
    if (!(inst = dlite_storage_load(s, id))) goto fail;
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
  if (!meta) FAILCODE1(dliteMissingMetadataError,
                       "cannot load metadata: %s", uri);

  /* Make sure that metadata is initialised */
  if (dlite_meta_init(meta)) goto fail;

  /* check metadata uri */
  if (strcmp(uri, meta->uri) != 0)
    FAILCODE3(dliteMissingMetadataError,
              "metadata uri (%s) does not correspond to that in storage "
              "(%s): %s",
              meta->uri, uri, s->location);

  /* read dimensions */
  dlite_datamodel_resolve_dimensions(d, meta);
  if (!(dims = calloc(meta->_ndimensions, sizeof(size_t))))
    FAILCODE(dliteMemoryError, "allocation failure");
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
				     p->ndims, pdims)) {
      dlite_type_clear(ptr, p->type, p->size);
      goto fail;
    }
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
        FAILCODE2(dliteMissingMetadataError, "metadata %s loaded from %s has no name, version and namespace",
             id, s->location);
      }
    } else {
      inst->uri = dlite_instance_default_uri(inst);
    }
  }

  /* cast if `metaid` is not NULL */
  if (inst && metaid)
    instance = dlite_mapping(metaid, (const DLiteInstance **)&inst, 1);
  else
    instance = inst;

 fail:
  if (!inst && !err_geteval())
    err(dliteIOError, "cannot load id '%s' from storage '%s'", id, s->location);
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

  if (!(meta = inst->meta)) return errx(dliteMissingMetadataError, "no metadata available");
  if (dlite_instance_sync_to_properties((DLiteInstance *)inst)) goto fail;

  /* check if storage implements the instance api */
  if (s->api->saveInstance)
    return s->api->saveInstance(s, inst);

  /* proceede with the datamodel api... */
  if (!(d = dlite_datamodel(s, inst->uuid))) goto fail;
  if (dlite_datamodel_set_meta_uri(d, meta->uri)) goto fail;

  dims = DLITE_DIMS(inst);
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
  by `driver`, `location` and `options`.

  Returns non-zero on error.
 */
int dlite_instance_save_loc(const char *driver, const char *location,
                            const char *options, const DLiteInstance *inst)
{
  int retval=1;
  DLiteStorage *s=NULL;

  if (!(s = dlite_storage_open(driver, location, options))) goto fail;
  retval = dlite_instance_save(s, inst);
 fail:
  if (s) dlite_storage_close(s);
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
  int retval=1;
  char *str=NULL, *driver=NULL, *loc=NULL, *options=NULL;
  if (!(str = strdup(url))) FAILCODE(dliteMemoryError, "allocation failure");
  if (dlite_split_url(str, &driver, &loc, &options, NULL)) goto fail;
  retval = dlite_instance_save_loc(driver, loc, options, inst);
 fail:
  if (str) free(str);
  return retval;
}


/*
  Loads instance `id` from memory buffer `buf` of size `size` and return it.
  Returns NULL on error.
 */
DLiteInstance *dlite_instance_memload(const char *driver,
                                      const unsigned char *buf, size_t size,
                                      const char *id, const char *options,
                                      const char *metaid)
{
  const DLiteStoragePlugin *api;
  DLiteInstance *inst;
  if (!(api = dlite_storage_plugin_get(driver))) return NULL;
  if (!api->memLoadInstance)
    return err(dliteUnsupportedError, "driver does not support memload: %s",
               api->name), NULL;

  if (!(inst = api->memLoadInstance(api, buf, size, id, options)))
    return NULL;
  if (metaid)
    return dlite_mapping(metaid, (const DLiteInstance **)&inst, 1);
  else
    return inst;
}

/*
  Stores instance `inst` to memory buffer `buf` of size `size`.

  Returns number of bytes written to `buf` (or would have been written
  to `buf` if `buf` is not large enough).
  Returns a negative error code on error.
 */
int dlite_instance_memsave(const char *driver, unsigned char *buf, size_t size,
                           const DLiteInstance *inst, const char *options)
{
  const DLiteStoragePlugin *api;
  if (!(api = dlite_storage_plugin_get(driver))) return dliteStorageSaveError;
  if (!api->memSaveInstance)
    return err(dliteUnsupportedError, "driver does not support memsave: %s",
               api->name);
  return api->memSaveInstance(api, buf, size, inst, options);
}

/*
  Saves instance to a newly allocated memory buffer.
  Returns NULL on error.
 */
unsigned char *dlite_instance_to_memory(const char *driver,
                                        const DLiteInstance *inst,
                                        const char *options)
{
  const DLiteStoragePlugin *api;
  int n=0, m;
  unsigned char *buf=NULL;
  if (!(api = dlite_storage_plugin_get(driver))) return NULL;
  if (!api->memSaveInstance)
    return err(dliteUnsupportedError, "driver does not support memsave: %s",
               api->name), NULL;

  if ((n = api->memSaveInstance(api, buf, n, inst, options)) < 0) return NULL;
  if (!(buf = malloc(n)))
    return err(dliteMemoryError, "allocation failure"), NULL;
  if ((m = api->memSaveInstance(api, buf, n, inst, options)) != n) {
    assert(m < 0);  // On success, should `m` always be equal to `n`!
    free(buf);
    return NULL;
  }
  return buf;
}


/*
  Returns a pointer to instance UUID.
 */
const char *dlite_instance_get_uuid(const DLiteInstance *inst)
{
  return inst->uuid;
}

/*
  Returns a pointer to instance URI.
 */
const char *dlite_instance_get_uri(const DLiteInstance *inst)
{
  return inst->uri;
}

/*
  Returns a pointer to the UUID of the instance metadata.
 */
const char *dlite_instance_get_meta_uuid(const DLiteInstance *inst)
{
  return inst->meta->uuid;
}

/*
  Returns a pointer to the URI of the instance metadata.
 */
const char *dlite_instance_get_meta_uri(const DLiteInstance *inst)
{
  return inst->meta->uri;
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
size_t dlite_instance_get_ndimensions(const DLiteInstance *inst)
{
  if (!inst->meta)
    return errx(dliteMissingMetadataError, "no metadata available");
  return inst->meta->_ndimensions;
}


/*
  Returns number of properties or -1 on error.
 */
size_t dlite_instance_get_nproperties(const DLiteInstance *inst)
{
  if (!inst->meta)
    return errx(dliteMissingMetadataError, "no metadata available");
  return inst->meta->_nproperties;
}


/*
  Returns size of dimension `i` or -1 on error.
 */
size_t dlite_instance_get_dimension_size_by_index(const DLiteInstance *inst,
                                                  size_t i)
{
  if (!inst->meta)
    return errx(dliteMissingMetadataError, "no metadata available");
  if (i >= inst->meta->_ndimensions)
    return errx(dliteIndexError, "no dimension with index %d in %s",
                (int)i, inst->meta->uri);
  if (inst->meta->_getdim) {
    size_t n = inst->meta->_getdim(inst, i);
    if (n != DLITE_DIM(inst, i))
      dlite_instance_sync_to_dimension_sizes((DLiteInstance *)inst);
  }
  return DLITE_DIM(inst, i);
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
    return errx(dliteMissingMetadataError, "no metadata available"), NULL;
  if (i >= inst->meta->_nproperties)
    return errx(dliteIndexError, "index %d exceeds number of properties (%d) in %s",
		(int)i, (int)inst->meta->_nproperties, inst->meta->uri), NULL;
  if (dlite_instance_sync_to_dimension_sizes((DLiteInstance *)inst))
    return NULL;
  if (inst->meta->_saveprop &&
      inst->meta->_saveprop((DLiteInstance *)inst, i)) return NULL;
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
  size_t nmemb=1;
  int j;

  if (inst->_flags & dliteImmutable)
    return err(1, "cannot set property on immutable instance: %s",
               (inst->uri) ? inst->uri : inst->uuid);

  /* Count members */
  if (p->ndims)
    for (j=0; j<p->ndims; j++) nmemb *= DLITE_PROP_DIM(inst, i, j);

  /* Consistency check for dliteRef */
  if (p->type == dliteRef && p->ref) {
    if (p->ndims) {
      size_t n;
      DLiteInstance **q = (DLiteInstance **)ptr;
      for (n=0; n<nmemb; n++, q++) {
        const char *ref = (*q)->meta->uri;
        if (strcmp(ref, p->ref))
          return err(dliteValueError, "Invalid reference. Expected %s, but got %s",
                     p->ref, ref);
      }
    } else {
      const char *ref = (*(DLiteInstance **)ptr)->meta->uri;
      if (strcmp(ref, p->ref))
        return err(dliteValueError, "Invalid reference. Expected %s, but got %s",
                   p->ref, ref);
    }
  }

  /* Copy */
  if (p->ndims > 0) {
    size_t n;
    dest = *((void **)DLITE_PROP(inst, i));
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

  if (inst->meta->_setdim &&
      dlite_instance_sync_from_dimension_sizes((DLiteInstance *)inst))
    return -1;
  if (inst->meta->_loadprop && inst->meta->_loadprop(inst, i)) return -1;

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
    return errx(dliteMissingMetadataError, "no metadata available");
  if (!(p = dlite_meta_get_property_by_index(inst->meta, i)))
    return -1;
  return p->ndims;
}

/*
  Returns size of shape `j` in property `i` or -1 on error.
 */
int dlite_instance_get_property_shape_by_index(const DLiteInstance *inst,
                                               size_t i, size_t j)
{
  const DLiteProperty *p;
  if (!inst->meta)
    return errx(dliteMissingMetadataError, "no metadata available");
  if (!(p = dlite_meta_get_property_by_index(inst->meta, i)))
    return -1;
  if (j >= (size_t)p->ndims)
    return errx(dliteIndexError, "dimension index j=%d is our of range", (int)j);
  return DLITE_PROP_DIM(inst, i, j);
}

/*
  Returns a malloc'ed array of dimensions of property `i` or NULL on error.
 */
size_t *dlite_instance_get_property_dims_by_index(const DLiteInstance *inst,
                                                  size_t i)
{
  size_t *dims;
  const DLiteProperty *p;
  if (!inst->meta)
    return errx(dliteMissingMetadataError, "no metadata available"), NULL;
  if (!(p = dlite_meta_get_property_by_index(inst->meta, i)))
    return NULL;
  if (dlite_instance_sync_to_dimension_sizes((DLiteInstance *)inst))
    return NULL;
  if (!(dims = malloc(p->ndims * sizeof(size_t))))
    return NULL;
  memcpy(dims, DLITE_PROP_DIMS(inst, i), p->ndims*sizeof(size_t));
  return dims;
}


/*
  Returns size of dimension `i` or -1 on error.
 */
int dlite_instance_get_dimension_size(const DLiteInstance *inst,
                                      const char *name)
{
  int i;
  if (!inst->meta)
    return errx(dliteMissingMetadataError, "no metadata available");
  if ((i = dlite_meta_get_dimension_index(inst->meta, name)) < 0) return -1;
  assert(i < (int)inst->meta->_nproperties);
  return dlite_instance_get_dimension_size_by_index(inst, i);
}

/*
  Returns a pointer to data corresponding to `name` or NULL on error.
 */
void *dlite_instance_get_property(const DLiteInstance *inst, const char *name)
{
  int i;
  if (!inst->meta)
    return errx(dliteMissingMetadataError, "no metadata available"), NULL;
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
    return errx(dliteMissingMetadataError, "no metadata available");
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return 1;
  return dlite_instance_set_property_by_index(inst, i, ptr);
}

/*
  Returns true if instance has a property with the given name.
 */
bool dlite_instance_has_property(const DLiteInstance *inst, const char *name)
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
    return errx(dliteMissingMetadataError, "no metadata available");
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
    return errx(dliteMissingMetadataError, "no metadata available");
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return -1;
  return dlite_instance_get_property_shape_by_index(inst, i, j);
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

/*
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
  Write a string representation of property `name` to `dest`.

  Arrays will be written with a JSON-like syntax.

  No more than `n` bytes are written to `dest` (incl. the terminating
  NUL).

  The `width` and `prec` arguments corresponds to the printf() minimum
  field width and precision/length modifier.  If you set them to -1, a
  suitable value will selected according to `type`.  To ignore their
  effect, set `width` to zero or `prec` to -2.

  The `flags` provides some format options.  If zero (default) bools
  and strings are expected to be quoted.

  Returns number of bytes written to `dest`.  If the output is
  truncated because it exceeds `n`, the number of bytes that would
  have been written if `n` was large enough is returned.  On error, a
  negative value is returned.
*/
int dlite_instance_print_property(char *dest, size_t n,
                                  const DLiteInstance *inst, const char *name,
                                  int width, int prec, DLiteTypeFlag flags)
{
  int i;
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return -1;
  return dlite_instance_print_property_by_index(dest, n, inst, i,
                                                width, prec, flags);
}

/*
  Lite dlite_print_property() but takes property index `i` as argument
  instead of property name.
*/
int dlite_instance_print_property_by_index(char *dest, size_t n,
                                           const DLiteInstance *inst,
                                           size_t i,
                                           int width, int prec,
                                           DLiteTypeFlag flags)
{
  void *ptr;
  const DLiteProperty *p;
  size_t *shape;
  if (i >= inst->meta->_nproperties)
    return errx(dliteIndexError, "index %d exceeds number of properties (%d) in %s",
                (int)i, (int)inst->meta->_nproperties, inst->meta->uri);
  if (!(ptr = dlite_instance_get_property_by_index(inst, i))) return -1;
  if (!(p = dlite_meta_get_property_by_index(inst->meta, i))) return -1;
  shape = DLITE_PROP_DIMS(inst, i);
  assert(shape);
  return dlite_property_print(dest, n, ptr, p, shape, width, prec, flags);
}

/*
  Lite dlite_print_property() but prints to allocated buffer.

  Prints to position `pos` in `*dest`, which should point to a buffer
  of size `*n`.  `*dest` is reallocated if needed.

  Returns number or bytes written or a negative number on error.
*/
int dlite_instance_aprint_property(char **dest, size_t *n, size_t pos,
                                   const DLiteInstance *inst,
                                   const char *name,
                                   int width, int prec, DLiteTypeFlag flags)
{
  int i;
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return -1;
  return dlite_instance_aprint_property_by_index(dest, n, pos, inst, i,
                                                 width, prec, flags);
}

/*
  Lite dlite_aprint_property() but takes property index `i` as argument
  instead of property name.
*/
int dlite_instance_aprint_property_by_index(char **dest, size_t *n,
                                            size_t pos,
                                            const DLiteInstance *inst,
                                            size_t i,
                                            int width, int prec,
                                            DLiteTypeFlag flags)
{
  void *ptr;
  const DLiteProperty *p;
  size_t *shape;
  if (i >= inst->meta->_nproperties)
    return errx(dliteIndexError, "index %d exceeds number of properties (%d) in %s",
                (int)i, (int)inst->meta->_nproperties, inst->meta->uri);
  if (!(ptr = dlite_instance_get_property_by_index(inst, i))) return -1;
  if (!(p = dlite_meta_get_property_by_index(inst->meta, i))) return -1;
  shape = DLITE_PROP_DIMS(inst, i);
  assert(shape);
  return dlite_property_aprint(dest, n, pos, ptr, p, shape, width, prec,
                               flags);
}

/*
  Scans property `name` from `src`.

  The `flags` provides some format options.  If zero (default) bools
  and strings are expected to be quoted.

  Returns number of characters consumed from `src` or a negative
  number on error.
 */
int dlite_instance_scan_property(const char *src, const DLiteInstance *inst,
                                 const char *name, DLiteTypeFlag flags)
{
  int i;
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return -1;
  return dlite_instance_scan_property_by_index(src, inst, i, flags);
}

/*
  Lite dlite_scan_property() but takes property index `i` as argument
  instead of property name.
*/
int dlite_instance_scan_property_by_index(const char *src,
                                          const DLiteInstance *inst, size_t i,
                                          DLiteTypeFlag flags)
{
  void *ptr;
  const DLiteProperty *p;
  const size_t *shape;
  if (i >= inst->meta->_nproperties)
    return errx(dliteIndexError, "index %d exceeds number of properties (%d) in %s",
                (int)i, (int)inst->meta->_nproperties, inst->meta->uri);
  if (!(ptr = dlite_instance_get_property_by_index(inst, i))) return -1;
  if (!(p = dlite_meta_get_property_by_index(inst->meta, i))) return -1;
  shape = DLITE_PROP_DIMS(inst, i);
  assert(shape);
  return dlite_property_scan(src, ptr, p, shape, flags);
}


/*
  Updates dimension sizes from internal state by calling the getdim()
  method of extended metadata.  Does nothing, if the metadata has no
  getdim() method.

  Returns non-zero on error.
 */
int dlite_instance_sync_to_dimension_sizes(DLiteInstance *inst)
{
  int n, retval=1, update=0, *newdims=NULL;
  size_t i, *dims=DLITE_DIMS(inst);
  if (!inst->meta->_getdim) return 0;
  for (i=0; i<inst->meta->_ndimensions; i++) {
    if ((n = inst->meta->_getdim(inst, i)) < 0) goto fail;
    if (n != (int)dims[i]) update=1;
  }
  if (update) {
    if (!(newdims = calloc(inst->meta->_ndimensions, sizeof(int))))
      return err(dliteMemoryError, "allocation failure");
    for (i=0; i<inst->meta->_ndimensions; i++)
      newdims[i] = inst->meta->_getdim(inst, i);
    if (dlite_instance_set_dimension_sizes(inst, newdims)) goto fail;
  }
  retval = 0;
 fail:
  if (newdims) free(newdims);
  return retval;
}

/*
  Updates internal state of extended metadata from instance dimensions
  using setdim().  Does nothing, if the metadata has no setdim().

  Returns non-zero on error.
 */
int dlite_instance_sync_from_dimension_sizes(DLiteInstance *inst)
{
  size_t i;
  if (!inst->meta->_setdim) return 0;
  for (i=0; i<inst->meta->_ndimensions; i++)
    if (inst->meta->_setdim(inst, i, DLITE_DIM(inst, i))) return 1;
  return 0;
}

/*
  Help function that update properties from the saveprop() method of
  extended metadata.  Does nothing, if the metadata has no saveprop() method.

  Returns non-zero on error.
 */
int dlite_instance_sync_to_properties(DLiteInstance *inst)
{
  size_t i;
  if (!inst->meta->_saveprop) return 0;
  if (dlite_instance_sync_to_dimension_sizes(inst)) return 1;
  for (i=0; i<inst->meta->_nproperties; i++)
    if (inst->meta->_saveprop(inst, i)) return 1;
  return 0;
}

/*
  Updates internal state of extended metadata from instance properties
  using loadprop().  Does nothing, if the metadata has no loadprop().

  Returns non-zero on error.
 */
int dlite_instance_sync_from_properties(DLiteInstance *inst)
{
  size_t i;
  if (!inst->meta->_loadprop) return 0;
  if (dlite_instance_sync_from_dimension_sizes(inst)) return 1;
  for (i=0; i<inst->meta->_nproperties; i++)
    if (inst->meta->_loadprop(inst, i)) return 1;
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

  if (inst->_flags & dliteImmutable)
    return err(1, "cannot set property on immutable instance: %s",
               (inst->uri) ? inst->uri : inst->uuid);

  if (!dlite_instance_is_data(inst))
    return err(dliteIndexError, "it is not possible to change dimensions of metadata");

  if (inst->meta->_setdim)
    for (n=0; n < inst->meta->_ndimensions; n++)
      if (inst->meta->_setdim(inst, n, dims[n]) < 0) goto fail;

  if (!(xdims = calloc(inst->meta->_ndimensions, sizeof(size_t))))
    FAILCODE(dliteMemoryError, "Allocation failure");
  for (n=0; n < inst->meta->_ndimensions; n++)
    xdims[n] = (dims[n] >= 0) ? (size_t)dims[n] : DLITE_DIM(inst, n);

  /* save old propdims and property members (oldmembs) */
  if (!(oldpropdims = calloc(inst->meta->_npropdims, sizeof(size_t))))
    FAILCODE(dliteMemoryError, "Allocation failure");
  memcpy(oldpropdims, DLITE_PROP_DIMS(inst, 0),
         inst->meta->_npropdims * sizeof(size_t));

  if (!(oldmembs = calloc(inst->meta->_nproperties, sizeof(int))))
    FAILCODE(dliteMemoryError, "Allocation failure");
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
      void **q;
      if (newmembs < oldmembs[n])
        for (i=newmembs; i < oldmembs[n]; i++)
          dlite_type_clear((char *)(*ptr) + i*p->size, p->type, p->size);
      if (!(q = realloc(*ptr, newsize))) {
        free(*ptr);
        return err(dliteMemoryError, "error reallocating '%s' to size %d", p->name, newsize);
      }
      *ptr = q;

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

  if (dlite_instance_sync_from_dimension_sizes(inst)) goto fail;

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
  if (!dims) return err(dliteMemoryError, "allocation failure");
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
  size_t i;
  if (dlite_instance_sync_to_properties((DLiteInstance *)inst)) return NULL;
  if (!(new = dlite_instance_create(inst->meta, DLITE_DIMS(inst), newid)))
    return NULL;
  if (inst->_parent) {
    if (!(new->_parent = malloc(sizeof(DLiteParent))))
      FAIL("allocation failure");
    memcpy(new->_parent, inst->_parent, sizeof(DLiteParent));
    if (new->_parent->parent)
      dlite_instance_incref((DLiteInstance *)new->_parent->parent);
  }
  for (i=0; i < inst->meta->_nproperties; i++) {
    void *src = dlite_instance_get_property_by_index(inst, i);
    assert(src);
    if (dlite_instance_set_property_by_index(new, i, src)) goto fail;
  }
  return new;
 fail:
  if (new) dlite_instance_decref(new);
  return NULL;
}


/*
  Returns a new DLiteArray object for property number `i` in instance `inst`.

  `order` can be 'C' for row-major (C-style) order and 'F' for column-manor
  (Fortran-style) order.

  The returned array object only describes, but does not own the
  underlying array data, which remains owned by the instance.

  Scalars are treated as a one-dimensional array or length one.

  Returns NULL on error.
 */
DLiteArray *
dlite_instance_get_property_array_by_index(const DLiteInstance *inst,
                                           size_t i, int order)
{
  void *ptr;
  int ndims=1;
  size_t dim=1, *shape=&dim;
  DLiteProperty *p = DLITE_PROP_DESCR(inst, i);
  DLiteArray *arr = NULL;
  if (!(ptr = dlite_instance_get_property_by_index(inst, i))) goto fail;
  if (p->ndims > 0) {
    int j;
    if (!(shape = malloc(p->ndims*sizeof(size_t)))) goto fail;
    ndims = p->ndims;
    for (j=0; j < p->ndims; j++)
      shape[j] = DLITE_PROP_DIM(inst, i, j);
  }
  arr = dlite_array_create_order(ptr, p->type, p->size, ndims, shape, order);
 fail:
  if (shape && shape != &dim) free(shape);
  return arr;
}


/*
  Returns a new DLiteArray object for property `name` in instance `inst`.

  `order` can be 'C' for row-major (C-style) order and 'F' for column-manor
  (Fortran-style) order.

  The returned array object only describes, but does not own the
  underlying array data, which remains owned by the instance.

  Scalars are treated as a one-dimensional array or length one.

  Returns NULL on error.
 */
DLiteArray *dlite_instance_get_property_array(const DLiteInstance *inst,
                                              const char *name, int order)
{
  int i;
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return NULL;
  return dlite_instance_get_property_array_by_index(inst, i, order);
}


/*
  Copy value of property `name` to memory pointed to by `dest`.  It must be
  large enough to hole all the data.  The order argument `order` has the
  following meaning:
    'C':  row-major (C-style) order, no reordering.
    'F':  coloumn-major (Fortran-style) order, transposed order.

  Return non-zero on error.
 */
int dlite_instance_copy_property(const DLiteInstance *inst, const char *name,
                                 int order, void *dest)
{
  int i;
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return 1;
  return dlite_instance_copy_property_by_index(inst, i, order, dest);
}


/*
  Like dlite_instance_copy_property(), but the property is specified by
  its index `i` instead of name.
 */
int dlite_instance_copy_property_by_index(const DLiteInstance *inst, int i,
                                          int order, void *dest)
{
  int retval=1;
  DLiteProperty *p = inst->meta->_properties + i;
  DLiteArray *arr = dlite_instance_get_property_array(inst, p->name, order);
  if (!arr) return 1;
  retval = dlite_instance_cast_property_by_index(inst, i, p->type, p->size,
                                                 arr->shape, arr->strides,
                                                 dest, NULL);
  dlite_array_free(arr);
  return retval;
}


/*
  Copies and possible type-cast value of property number `i` to memory
  pointed to by `dest` using `castfun`.  The destination memory is
  described by arguments `type`, `size` `shape` and `strides`.  It must
  be large enough to hole all the data.

  If `castfun` is NULL, it defaults to dlite_type_copy_cast().

  Return non-zero on error.
 */
int dlite_instance_cast_property_by_index(const DLiteInstance *inst,
                                          int i,
                                          DLiteType type,
                                          size_t size,
                                          const size_t *shape,
                                          const int *strides,
                                          void *dest,
                                          DLiteTypeCast castfun)
{
  void *src = dlite_instance_get_property_by_index(inst, i);
  DLiteProperty *p = inst->meta->_properties + i;
  size_t *sdims = DLITE_PROP_DIMS(inst, i);
  return dlite_type_ndcast(p->ndims,
                           dest, type, size, shape, strides,
                           src, p->type, p->size, sdims, NULL,
                           castfun);
}


/*
  Assigns property `name` to memory pointed to by `src`. The meaning
  of `order` is:
    'C':  row-major (C-style) order, no reordering.
    'F':  coloumn-major (Fortran-style) order, transposed order.

  Return non-zero on error.
 */
int dlite_instance_assign_property(const DLiteInstance *inst, const char *name,
                                   int order, const void *src)
{
  int i, retval=1;
  DLiteProperty *p;
  DLiteArray *arr;
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return 1;
  p = inst->meta->_properties + i;
  if (!(arr = dlite_instance_get_property_array(inst, p->name, order)))
    return 1;
  retval =
    dlite_instance_assign_casted_property_by_index(inst, i, p->type, p->size,
                                                   arr->shape, arr->strides,
                                                   src, NULL);
  dlite_array_free(arr);
  return retval;
}


/*
  Assigns property `i` by copying and possible type-cast memory
  pointed to by `src` using `castfun`.  The memory pointed to by `src`
  is described by arguments `type`, `size` `shape` and `strides`.

  If `castfun` is NULL, it defaults to dlite_type_copy_cast().

  Return non-zero on error.
 */
int dlite_instance_assign_casted_property_by_index(const DLiteInstance *inst,
                                                   int i,
                                                   DLiteType type,
                                                   size_t size,
                                                   const size_t *shape,
                                                   const int *strides,
                                                   const void *src,
                                                   DLiteTypeCast castfun)
{
  void *dest = dlite_instance_get_property_by_index(inst, i);
  DLiteProperty *p = inst->meta->_properties + i;
  size_t *dims = DLITE_PROP_DIMS(inst, i);
  return dlite_type_ndcast(p->ndims,
                           dest, p->type, p->size, dims, NULL,
                           src, type, size, shape, strides,
                           castfun);
}


/*
  Return a newly allocated default instance URI constructed from
  the metadata URI and the UUID of the instance, as `<meta_uri>/<uuid>`.

  Returns NULL on error.
 */
char *dlite_instance_default_uri(const DLiteInstance *inst)
{
  int n = strlen(inst->meta->uri);
  char *buf = malloc(n + DLITE_UUID_LENGTH + 2);
  if (!buf) return err(dliteMemoryError, "allocation failure"), NULL;
  memcpy(buf, inst->meta->uri, n);
  buf[n] = '/';
  memcpy(buf+n+1, inst->uuid, DLITE_UUID_LENGTH);
  buf[n+DLITE_UUID_LENGTH+1] = '\0';
  return buf;
}


/*
  Calculates a hash of instance `inst`.  The calculated hash is stored
  in `hash`, where `hashsize` is the size of `hash` in bytes.  It should
  be 32, 48 or 64.

  Return non-zero on error.
 */
int dlite_instance_get_hash(const DLiteInstance *inst,
                            uint8_t *hash, int hashsize)
{
  size_t i;
  sha3_context c;
  const uint8_t *buf;
  unsigned bitsize = hashsize * 8;
  int retval = 0;

  /* If metadata defines a custom hash function, use that instead of
     this implementation. */
  if (inst->meta->_gethash)
    return inst->meta->_gethash(inst, hash, hashsize);

  sha3_Init(&c, bitsize);
  sha3_SetFlags(&c, SHA3_FLAGS_KECCAK);

  if (inst->_parent) {
    sha3_Update(&c, inst->_parent->uuid, DLITE_UUID_LENGTH);
    sha3_Update(&c, inst->_parent->hash, DLITE_HASH_SIZE);
  }
  sha3_Update(&c, inst->meta->uri, strlen(inst->meta->uri));
  for (i=0; i<DLITE_NDIM(inst); i++) {
    uint64_t n = DLITE_DIM(inst, i);
    sha3_Update(&c, &n, sizeof(uint64_t));
  }
  for (i=0; i<DLITE_NPROP(inst); i++) {
    void *ptr = dlite_instance_get_property_by_index(inst, i);
    DLiteProperty *p = DLITE_PROP_DESCR(inst, i);
    size_t j, len = 1;
    for (j=0; (int)j<p->ndims; j++) len *= DLITE_PROP_DIM(inst, i, j);
    if (dlite_type_is_allocated(p->type)) {
      char *v = ptr;
      for (j=0; j<len; j++, v+=p->size)
        if ((retval = dlite_type_update_sha3(&c, v, p->type, p->size))) {
          err(1, "error updating hash for property \"%s\" of instance \"%s\"",
              p->name, (inst->uri) ? inst->uri : inst->uuid);
          break;
        }
    } else {
      sha3_Update(&c, ptr, len*p->size);
    }
  }

  buf = sha3_Finalize(&c);
  memcpy(hash, buf, hashsize);
  return retval;
}

/********************************************************************
 *  Transactions
 ********************************************************************/

/*
  Mark the instance as immutable.  This can never be reverted.

  **Note**
  Immutability of the underlying data cannot be enforced in
  C as long as the someone has a pointer to the instance.  However,
  functions like dlite_instance_set_property() and
  dlite_instance_set_dimension_size() will refuse to change the
  instance if it is immutable.  Furthermore, if the instance is used
  as a parent in a transaction, any changes to the underlying data
  will be detected by calling dlite_instance_verify().
 */
void dlite_instance_freeze(DLiteInstance *inst)
{
  inst->_flags |= dliteImmutable;
}

/*
  Returns non-zero if instance is immutable (frozen).
 */
int dlite_instance_is_frozen(const DLiteInstance *inst)
{
  return inst->_flags & dliteImmutable;
}

/* Number of random characters in shapshot id (sid) */
#define SID_LEN 12

/*
  Make a snapshop of mutable instance `inst`.

  The `inst` will be a transaction whos parent is the snapshot.  If
  `inst` already has a parent, that will now be the parent of the
  snapshot.

  The reason that `inst` must be mutable, is that its hash will change
  due to change in its parent.

  The snapshot will be assigned an URI of the form
  "snapshot-XXXXXXXXXXXX" (or inst->uri#snapshot-XXXXXXXXXXXX if
  inst->uri is not NULL) where each X is replaces with a random
  character.

  On error non-zero is returned and `inst` is unchanged.
 */
int dlite_instance_snapshot(DLiteInstance *inst)
{
  DLiteInstance *snapshot=NULL;
  int retval=1, i;
  const char *id = (inst->uri) ? inst->uri : inst->uuid;
  int len = strcspn(id, "#");
  char *uri = NULL;
  char sid[SID_LEN+1];
  char randchars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

  if (dlite_instance_is_frozen(inst))
    FAIL1("cannot snapshot an immutable instance: %s", id);

  /* Make sure that the random number generator is seeded */
  dlite_init();

  /* Create a random snapshot id of alphanumerical characters. */
  for (i=0; i<SID_LEN; i++) {
    uint32_t n = rand_msws32() % (sizeof(randchars)-1);
    sid[i] = randchars[n];
  }
  sid[SID_LEN] = '\0';
  if (asprintf(&uri, "%.*s#snapshot-%s", len, id, sid) < 0)
    FAIL1("error formatting uri for snapshot of %s", id);

  if (!(snapshot = dlite_instance_copy(inst, uri))) goto fail;
  dlite_instance_freeze(snapshot);
  if (dlite_instance_set_parent(inst, snapshot)) goto fail;
  retval = 0;
 fail:
  if (uri) free(uri);
  if (snapshot) dlite_instance_decref(snapshot);
  return retval;
}

/*
  Returns a borrowed reference to shapshot number `n` of `inst`, where
  `n` counts backward from `inst`.  Hence, `n=0` returns `inst`, `n=1`
  returns the parent of `inst`, etc...

  Snapshots may be pulled back into memory using dlite_instance_get().
  Use dlite_instance_pull() if you know the storage that the snapshots
  are stored in.

  Returns NULL on error.
 */
const DLiteInstance *dlite_instance_get_snapshot(const DLiteInstance *inst,
                                                 int n)
{
  int i;
  const DLiteInstance *cur = inst;
  if (n < 0) return err(1, "snapshot number must be positive"), NULL;
  for (i=0; i<n; i++) {
    DLiteParent *p = cur->_parent;
    if (!p) return err(1, "snapshot index %d exceeds number of snapsshots: %d",
                       n, i), NULL;
    if (p->parent) {
      cur = p->parent;
    } else {
      if (!(p->parent = dlite_instance_get(p->uuid))) return NULL;
      cur = p->parent;
    }
  }
  assert(cur);
  return cur;
}

/*
  Like dlite_instance_get_snapshot(), except that possible stored
  snapshots are pulled from storage `s` to memory.

  Returns a borrowed reference to snapshot `n` or NULL on error.
 */
const DLiteInstance *dlite_instance_pull_snapshot(const DLiteInstance *inst,
                                                  DLiteStorage *s, int n)
{
  int i;
  const DLiteInstance *cur = inst;
  if (n < 0) return err(1, "snapshot number must be positive"), NULL;
  if (!(s->flags & dliteTransaction))
    return err(1, "storage does not support transactions"), NULL;
  for (i=0; i<n; i++) {
    DLiteParent *p = cur->_parent;
    if (!p) return err(1, "snapshot index %d exceeds number of snapsshots: %d",
                       n, i), NULL;
    if (p->parent) {
      cur = p->parent;
    } else {
      if (!(p->parent = dlite_storage_load(s, p->uuid))) return NULL;
      cur = p->parent;
    }
  }
  assert(cur);
  return cur;
}

/*
  Push all ancestors of snapshot `n` from memory to storage `s`,
  where `n=0` corresponds to `inst`, `n=1` to the parent of `inst`, etc...

  No snapshot is pulled back from storage, so if the snapshots are
  already in storage, this function has no effect.

  This function starts pushing closest to the root to ensure that the
  transaction is in a consistent state at all times.

  Returns zero on success or the (positive) index of the snapshot that
  fails to push.
*/
int dlite_instance_push_snapshot(const DLiteInstance *inst,
                                 DLiteStorage *s, int n)
{
  int m;
  const DLiteInstance *p;
  if (!(s->flags & dliteTransaction))
    return err(1, "storage does not support transactions");
  if (!inst->_parent || !(p = inst->_parent->parent)) return 0;
  if ((m = dlite_instance_push_snapshot(p, s, n-1))) return m+1;
  if (n > 0) return 0;
  if (dlite_instance_save(s, p)) return 1;
  inst->_parent->parent = NULL;
  dlite_instance_decref((DLiteInstance *)p);
  return 0;
}

/*
  Prints transaction to stdout.  Returns non-zero on error.
 */
int dlite_instance_print_transaction(const DLiteInstance *inst)
{
  char *s=NULL;
  size_t size=0;
  int n=0, m;
  const DLiteInstance *p = inst;
  m = asnpprintf(&s, &size, n, "\n");
  n += m;
  do {
    uint8_t hash[DLITE_HASH_SIZE];
    char shash[DLITE_HASH_SIZE*2+1];
    dlite_instance_get_hash(p, hash, sizeof(hash));
    strhex_encode(shash, sizeof(shash), hash, sizeof(hash));
    m = asnpprintf(&s, &size, n, "%s\n", (p->uri) ? p->uri : p->uuid);
    n += m;
    m = asnpprintf(&s, &size, n, "  - uuid: %s\n", p->uuid);
    n += m;
    m = asnpprintf(&s, &size, n, "  - hash: %s\n", shash);
    n += m;
  } while ((p = dlite_instance_get_snapshot(p, 1)));
  printf("%s\n", s);
  if(s) free(s);
  return 0;
}

/*
  Turn instance `inst` into a transaction node with parent `parent`.

  This require that `inst` is mutable, and `parent` is immutable.

  If `inst` already has a parent, it will be replaced.

  Use dlite_instance_freeze() and dlite_instance_is_frozen() to make
  and check that an instance is immutable, respectively.

  Returns non-zero on error.
 */
int dlite_instance_set_parent(DLiteInstance *inst, const DLiteInstance *parent)
{
  DLiteParent *p = inst->_parent;
  uint8_t hash[DLITE_HASH_SIZE];
  if (inst->_flags & dliteImmutable)
    return err(-1, "Parent cannot be added to immutable instance: %s",
               (inst->uri) ? inst->uri : inst->uuid);
  if (!(parent->_flags & dliteImmutable))
    return err(-1, "Mutable instance \"%s\" cannot be added as parent",
               (parent->uri) ? parent->uri : parent->uuid);
  if (dlite_instance_get_hash(parent, hash, DLITE_HASH_SIZE))
    return err(-1, "Error calculating hash of parent instance \"%s\"",
               (parent->uri) ? parent->uri : parent->uuid);
  if (p) {
    if (p->parent) dlite_instance_decref((DLiteInstance *)p->parent);
  } else {
    if (!(p = calloc(1, sizeof(DLiteParent))))
      return err(-1, "Allocation failure");
    inst->_parent = p;
  }
  p->parent = parent;
  strncpy(p->uuid, parent->uuid, DLITE_UUID_LENGTH+1);
  memcpy(p->hash, hash, DLITE_HASH_SIZE);
  dlite_instance_incref((DLiteInstance *)p->parent);
  return 0;
}



///*
//  Returns a new reference to the parent of instance `inst` or NULL on
//  error or if `inst` has no parent.
//
//  Call dlite_instance_has_parent() to distinguish between error or that
//  `inst` has no parent.
// */
//const DLiteInstance *dlite_instance_get_parent(const DLiteInstance *inst)
//{
//  if (!inst->_parent) return NULL;
//  if (!inst->_parent->parent) {
//    DLiteInstance *parent = dlite_instance_get(inst->_parent->uuid);
//    uint8_t hash[DLITE_HASH_SIZE];
//    if (!parent) return NULL;
//
//    /* Verify the parent when it is assigned with dlite_instance_get() */
//    if (dlite_instance_get_hash(parent, hash, DLITE_HASH_SIZE))
//      goto fail;
//    if (memcmp(hash, inst->_parent->hash, DLITE_HASH_SIZE))
//      FAIL1("Invalid hash for parent of instance \"%s\"",
//            (inst->uri) ? inst->uri : inst->uuid);
//
//    inst->_parent->parent = parent;
//  }
//
//  dlite_instance_incref((DLiteInstance *)inst->_parent->parent);
//  return inst->_parent->parent;
//
// fail:
//  dlite_instance_decref((DLiteInstance *)inst->_parent->parent);
//  return NULL;
//}

/*
  Returns non-zero if `inst` has a parent.
 */
int dlite_instance_has_parent(const DLiteInstance *inst)
{
  return (inst->_parent) ? 1 : 0;
}

/*
  Verify that the hash of instance `inst`.

  If `hash` is not NULL, this function verifies that the hash of
  `inst` corresponds to the memory pointed to by `hash`.  The size of
  the memory should be `DLITE_HASH_SIZE` bytes.

  If `hash` is NULL and `inst` has a parent, this function will
  instead verify that the parent hash stored in `inst` corresponds to
  the value of the parent.

  If `recursive` is non-zero, all ancestors of `inst` are also
  included in the verification.

  Returns zero if the hash is valid.  Otherwise non-zero is returned
  and an error message is issued.

  TODO:
  If `recursive` is non-zero and `inst` is a collection or has
  properties of type `dliteRef`, we should also verify the instances
  that are referred to.  This is currently not implemented, because
  we have to detect cyclic references to avoid infinite recursion.
 */
int dlite_instance_verify_hash(const DLiteInstance *inst, uint8_t *hash,
                              int recursive)
{
  uint8_t calculated_hash[DLITE_HASH_SIZE];
  const DLiteInstance *parent;
  int stat=0;

  if (hash) {
    if (dlite_instance_get_hash(inst, calculated_hash, DLITE_HASH_SIZE))
      return 2;
    if (memcmp(hash, calculated_hash, DLITE_HASH_SIZE))
      return err(1, "hash does not correspond to the value of instance \"%s\"",
                 (inst->uri) ? inst->uri : inst->uuid);
  }
  if (!inst->_parent) return 0;
  if (!(parent = inst->_parent->parent))
    parent = dlite_instance_get(inst->_parent->uuid);

  if (!parent)
    stat = err(3, "cannot retrieve parent of instance \"%s\"",
               (inst->uri) ? inst->uri : inst->uuid);
  else if (recursive || !hash)
    stat = dlite_instance_verify_hash(parent, inst->_parent->hash, recursive);

  if (parent && !inst->_parent->parent)
    dlite_instance_decref((DLiteInstance *)parent);

  return stat;
}


/*
  Verifies a transaction.

  Equivalent to calling `dlite_instance_very_hash(inst, NULL, 1)`.

  Returns zero if the hash is valid.  Otherwise non-zero is returned
  and an error message is issued.
 */
int dlite_instance_verify_transaction(const DLiteInstance *inst)
{
  return dlite_instance_verify_hash(inst, NULL, 1);
}



/********************************************************************
 *  Metadata
 ********************************************************************/

/*
  Specialised function that returns a new metadata created from the
  given arguments.  It is an instance of DLITE_ENTITY_SCHEMA.
 */
DLiteMeta *
dlite_meta_create(const char *uri, const char *description,
                  size_t ndimensions, const DLiteDimension *dimensions,
                  size_t nproperties, const DLiteProperty *properties)
{
  DLiteMeta *entity=NULL;
  DLiteInstance *e=NULL;
  char *name=NULL, *version=NULL, *namespace=NULL;
  size_t dims[] = {ndimensions, nproperties};

  if ((e = dlite_instance_has(uri, 0))) {
    DLiteMeta *meta = (DLiteMeta *)e;
    if (!dlite_instance_is_meta(e))
      FAILCODE1(dliteMetadataExistError,
                "cannot create metadata \"%s\" since it already exists as a "
                "non-metadata instance", uri);
    if (meta->_ndimensions != ndimensions ||
        meta->_nproperties != nproperties)
      FAILCODE1(dliteMetadataExistError,
                "cannot create metadata \"%s\" since it already exists with "
                "different number of dimensions and/or properties", uri);
    // TODO: check that dimensions and properties matches
    dlite_meta_incref(meta);
    return meta;
  }

  if (dlite_split_meta_uri(uri, &name, &version, &namespace)) goto fail;
  if (!(e=dlite_instance_create(dlite_get_entity_schema(), dims, uri)))
    goto fail;

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

  Note, even though this function is called internally in
  dlite_instance_create(), it has to be called again after properties
  has been assigned to the metadata.  This because `_npropdims` and
  `__propdiminds` depends on the property dimensions.

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
    if (strcmp(meta->meta->_dimensions[i].name, "ndimensions") == 0)
      idim_dim = i;
    if (strcmp(meta->meta->_dimensions[i].name, "nproperties") == 0)
      idim_prop = i;
    if (strcmp(meta->meta->_dimensions[i].name, "nrelations") == 0)
      idim_rel = i;
  }
  if (idim_dim < 0) return err(dliteIndexError, "dimensions are expected in metadata");
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
  DEBUG_LOG("    headersize=%d\n", (int)meta->_headersize);

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
  DEBUG_LOG("    dimoffset=%d (+ %d * %d)\n",
            (int)meta->_dimoffset, (int)meta->_ndimensions,
            (int)sizeof(size_t));

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
    DEBUG_LOG("    propoffset[%d]=%d (type=%d size=%-2d ndims=%d)"
              " + %d\n",
              (int)i, (int)meta->_propoffsets[i], (int)p->type, (int)p->size,
              (int)p->ndims,
              (int)((p->ndims) ? sizeof(size_t *) : p->size));
  }
  /* -- relation values (reloffset) */
  if (meta->_nrelations) {
    size += padding_at(void *, size);
    meta->_reloffset = size;
    size += meta->_nrelations * sizeof(void *);
  } else {
    meta->_reloffset = size;
  }
  DEBUG_LOG("    reloffset=%d (+ %d * %d)\n",
            (int)meta->_reloffset, (int)meta->_nrelations, (int)sizeof(size_t));

  /* -- property dimension values (propdimsoffset) */
  size += padding_at(size_t, size);
  meta->_propdimsoffset = size;
  size += meta->_npropdims * sizeof(size_t);

  /* -- first property dimension indices (propdimsoffset) */
  size += padding_at(size_t, size);
  meta->_propdimindsoffset = size;
  size += meta->_nproperties * sizeof(size_t);
  DEBUG_LOG("    propdimindsoffset=%d\n", (int)meta->_propdimindsoffset);

  /* -- total size of instance (metadata has more fields) */
  size += padding_at(size_t, size);
  size += meta->_nproperties * sizeof(size_t);
  size += padding_at(size_t, size);
  DEBUG_LOG("    size=%d\n", (int)size);

  return 0;
 fail:
  return 1;
}


/*
  Increase reference count to metadata.

  Returns the new reference count.
 */
int dlite_meta_incref(DLiteMeta *meta)
{
  return dlite_instance_incref((DLiteInstance *)meta);
}

/*
  Decrease reference count to metadata.  If the reference count reaches
  zero, the metadata is free'ed.

  Returns the new reference count.
 */
int dlite_meta_decref(DLiteMeta *meta)
{
  return dlite_instance_decref((DLiteInstance *)meta);
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
  DLiteInstance *inst = dlite_instance_load(s, id);
  if (!inst) return NULL;
  if (!dlite_instance_is_meta(inst))
    return err(dliteTypeError, "not metadata: %s (%s)", s->location, id), NULL;
  return (DLiteMeta *)inst;
}

/*
  Like dlite_instance_load_url(), but loads metadata instead.
  Returns the metadata or NULL on error.
 */
DLiteMeta *dlite_meta_load_url(const char *url)
{
  DLiteInstance *inst;
  if (!(inst = dlite_instance_load_url(url))) return NULL;
  if (!dlite_instance_is_meta(inst))
    return err(dliteTypeError, "not metadata: %s", url), NULL;
  return (DLiteMeta *)inst;
}

/*
  Saves metadata `meta` to storage `s`.  Returns non-zero on error.
 */
int dlite_meta_save(DLiteStorage *s, const DLiteMeta *meta)
{
  return dlite_instance_save(s, (const DLiteInstance *)meta);
}

/*
  Saves metadata `meta` to `url`.  Returns non-zero on error.
 */
int dlite_meta_save_url(const char *url, const DLiteMeta *meta)
{
  return dlite_instance_save_url(url, (const DLiteInstance *)meta);
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
  return err(dliteIndexError, "%s has no such dimension: '%s'", meta->uri, name);
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
  return err(dliteAttributeError, "%s has no such property: '%s'", meta->uri, name);
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
    return err(dliteIndexError, "invalid dimension index %d", (int)i), NULL;
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


/*
  Returns true if `meta` has a dimension with the given name.
 */
bool dlite_meta_has_dimension(const DLiteMeta *meta, const char *name)
{
  size_t i;
  for (i=0; i<meta->_ndimensions; i++) {
    DLiteDimension *d = meta->_dimensions + i;
    if (strcmp(name, d->name) == 0) return true;
  }
  return false;
}

/*
  Returns true if `meta` has a property with the given name.
 */
bool dlite_meta_has_property(const DLiteMeta *meta, const char *name)
{
  size_t i;
  for (i=0; i<meta->_nproperties; i++) {
    DLiteProperty *p = meta->_properties + i;
    if (strcmp(name, p->name) == 0) return true;
  }
  return false;
}

/*
  Safe type casting from instance to metadata.
 */
DLiteMeta *dlite_meta_from_instance(DLiteInstance *inst)
{
  if (dlite_instance_is_data(inst))
    return err(dliteValueError, "cannot cast instance %s to metadata", inst->uuid), NULL;
  return (DLiteMeta *)inst;
}

/*
  Type cast metadata to instance - always possible.
 */
DLiteInstance *dlite_meta_to_instance(DLiteMeta *meta)
{
  return (DLiteInstance *)meta;
}


/********************************************************************
 *  Dimension
 ********************************************************************/

/*
  Returns a newly malloc'ed DLiteDimension or NULL on error.

  The `description` arguments may be NULL.
*/
DLiteDimension *dlite_dimension_create(const char *name,
                                       const char *description)
{
  DLiteDimension *dim;
  if (!(dim = calloc(1, sizeof(DLiteDimension)))) goto fail;
  if (!(dim->name = strdup(name))) goto fail;
  if (description && !(dim->description = strdup(description))) goto fail;
  return dim;
 fail:
  if (dim) dlite_dimension_free(dim);
  return err(dliteMemoryError, "allocation failure"), NULL;
}

/*
  Frees a DLiteDimension.
*/
void dlite_dimension_free(DLiteDimension *dim)
{
  if (dim->name) free(dim->name);
  if (dim->description) free(dim->description);
  free(dim);
}


/********************************************************************
 *  Property
 ********************************************************************/

/*
  Returns a newly malloc'ed DLiteProperty or NULL on error.

  It is created with no dimensions.  Use dlite_property_add_dim() to
  add dimensions to the property.

  The arguments `unit`, `iri` and `description` may be NULL.
*/
DLiteProperty *dlite_property_create(const char *name,
                                     DLiteType type,
                                     size_t size,
                                     const char *unit,
                                     const char *description)
{
  DLiteProperty *prop;
  if (!(prop = calloc(1, sizeof(DLiteProperty)))) goto fail;
  if (!(prop->name = strdup(name))) goto fail;
  prop->type = type;
  prop->size = size;
  if (unit && !(prop->unit = strdup(unit))) goto fail;
  if (description && !(prop->description = strdup(description))) goto fail;
  return prop;
 fail:
  if (prop) dlite_property_free(prop);
  return err(dliteMemoryError, "allocation failure"), NULL;
}


/*
  Clear DLiteProperty.

  Free all memory referred to by the property and it, but free `prop` itself.
 */
void dlite_property_clear(DLiteProperty *prop)
{
  int i;
  for (i=0; i < prop->ndims; i++) free(prop->shape[i]);
  if (prop->name)        free(prop->name);
  if (prop->ref)         free(prop->ref);
  if (prop->shape)       free(prop->shape);
  if (prop->unit)        free(prop->unit);
  if (prop->description) free(prop->description);
  memset(prop, 0, sizeof(DLiteProperty));
}

/*
  Frees a DLiteProperty.
*/
void dlite_property_free(DLiteProperty *prop)
{
  dlite_property_clear(prop);
  free(prop);
}

/*
  Add dimension expression `expr` to property.  Returns non-zero on error.
 */
int dlite_property_add_dim(DLiteProperty *prop, const char *expr)
{
  if (!(prop->shape = realloc(prop->shape, sizeof(char *)*(prop->ndims+1))))
    goto fail;
  if (!(prop->shape[prop->ndims] = strdup(expr))) goto fail;
  prop->ndims++;
  return 0;
 fail:
  return err(dliteMemoryError, "allocation failure");
}


/* Expands to `a - b` if `a > b` else to `0`. */
#define PDIFF(a, b) (((size_t)(a) > (size_t)(b)) ? (a) - (b) : 0)

/*
  Recursive help function for dlite_property_print() for handling
  n-dimensional arrays.

  Arguments:
     d      current dimension
     dest   buffer to write to
     n      size of `dest`
     pptr   pointer to pointer to memory with the data to be written
     p      property describing the data
     shape  array of property dimension values
     width  printf() field width
     prec   printf() precision

  Return number of bytes written to `dest` or would have been written
  to `dest` if it is not big enough.  Returns -1 on error.
*/
static int writedim(int d, char *dest, size_t n, const void **pptr,
                    const DLiteProperty *p, const size_t *shape,
                    int width, int prec, DLiteTypeFlag flags)
{
  int N=0, m;
  size_t i;
  int compact = (p->type != dliteRelation) || (flags & dliteFlagCompactRel);
  char *start = (compact) ? "["  : "[\n        ";
  char *sep   = (compact) ? ", " : ",\n        ";
  char *end   = (compact) ? "]"  : "\n      ]";
  if (d < p->ndims) {
    if ((m = snprintf(dest+N, PDIFF(n, N), "%s", start)) < 0) goto fail;
    N += m;
    for (i=0; i < shape[d]; i++) {
      if ((m = writedim(d+1, dest+N, PDIFF(n, N), pptr, p, shape,
                        width, prec, flags)) < 0) return -1;
      N += m;
      if (i < shape[d]-1) {
        if ((m = snprintf(dest+N, PDIFF(n, N), "%s", sep)) < 0) goto fail;
        N += m;
      }
    }
    if ((m = snprintf(dest+N, PDIFF(n, N), "%s", end)) < 0) goto fail;
    N += m;
  } else {
    if ((m = dlite_type_print(dest+N, PDIFF(n, N), *pptr, p->type, p->size,
                              width, prec, flags)) < 0) return m;
    N += m;
    *((char **)pptr) += p->size;
  }
  return N;
 fail:
  return err(dliteIOError, "failed to write string representation of array");
}

/*
  Writes a string representation of data for property `p` to `dest`.

  The pointer `ptr` should point to the memory where the data is stored.
  The meaning and layout of the data is described by property `p`.
  The actual sizes of the property dimension is provided by `shape`.  Use
  dlite_instance_get_property_dims_by_index() or the DLITE_PROP_DIMS macro
  for accessing `shape`.

  No more than `n` bytes are written to `dest` (incl. the terminating
  NUL).  Arrays will be written with a JSON-like syntax.

  The `width` and `prec` arguments corresponds to the printf() minimum
  field width and precision/length modifier.  If you set them to -1, a
  suitable value will selected according to `type`.  To ignore their
  effect, set `width` to zero or `prec` to -2.

  The `flags` provides some format options.  If zero (default) bools
  and strings are quoted.

  Returns number of bytes written to `dest`.  If the output is
  truncated because it exceeds `n`, the number of bytes that would
  have been written if `n` was large enough is returned.  On error, a
  negative value is returned.
 */
int dlite_property_print(char *dest, size_t n, const void *ptr,
                         const DLiteProperty *p, const size_t *shape,
                         int width, int prec, DLiteTypeFlag flags)
{
  if (flags == dliteFlagDefault) flags = dliteFlagQuoted;
  if (p->ndims)
    return writedim(0, dest, n, &ptr, p, shape, width, prec, flags);
  else
    return dlite_type_print(dest, n, ptr, p->type, p->size, width, prec, flags);
}

/*
  Like dlite_type_print(), but prints to allocated buffer.

  Prints to position `pos` in `*dest`, which should point to a buffer
  of size `*n`.  `*dest` is reallocated if needed.

  Returns number or bytes written or a negative number on error.
 */
int dlite_property_aprint(char **dest, size_t *n, size_t pos, const void *ptr,
                          const DLiteProperty *p, const size_t *shape,
                          int width, int prec, DLiteTypeFlag flags)
{
  int m;
  void *q;
  size_t newsize;
  if (!dest && !*dest) *n = 0;
  m = dlite_property_print(*dest + pos, PDIFF(*n, pos), ptr, p, shape,
                           width, prec, flags);
  if (m < 0) return m;  /* failure */
  if (m < (int)PDIFF(*n, pos)) return m;  // success, buffer is large enough

  /* Reallocate buffer to required size. */
  newsize = m + pos + 1;
  if (!(q = realloc(*dest, newsize))) return -1;
  *dest = q;
  *n = newsize;
  m = dlite_property_print(*dest + pos, PDIFF(*n, pos), ptr, p, shape,
                           width, prec, flags);
  assert(0 <= m && m < (int)*n);
  return m;
}


/*
  Recursive help function for dlite_property_scan() for handling
  n-dimensional arrays.

  Arguments:
   - d      current dimension
   - src    buffer to read from
   - pptr   pointer to pointer to memory to write to
   - p      property describing the data
   - shape  array of property dimension values
   - t      pointer to a jsmn token

  Returns zero on success and a negative number on error.
*/
static int scandim(int d, const char *src, void **pptr,
                   const DLiteProperty *p, const size_t *shape,
                   DLiteTypeFlag flags, jsmntok_t **t)
{
  int m;
  size_t i;
  if (d < p->ndims) {
    if ((*t)->type != JSMN_ARRAY)
      return err(dliteValueError, "expected JSON array");
    if ((*t)->size != (int)shape[d])
      return err(dliteIndexError, "for dimension %d, expected %d elements, got %d",
                 d, (int)shape[d], (*t)->size);
    for (i=0; i < shape[d]; i++) {
      (*t)++;
      if (scandim(d+1, src, pptr, p, shape, flags, t)) goto fail;
    }
  } else {
    if ((m = dlite_type_scan(src+(*t)->start, (*t)->end-(*t)->start, *pptr,
                             p->type, p->size, flags)) < 0) return m;
    *((char **)pptr) += p->size;
    *t += jsmn_count(*t);
  }
  return 0;
 fail:
  return err(dliteIOError, "failed to scan string representation of array");
}

/*
  Help function for scanning dimensions and properties expressed as
  JSON objects (SOFT7 format).

  Arguments:
    - src: JSON data to scan.
    - item: Pointer into array of JSMN tokens corresponding to the item
        to scan.
    - key: The key corresponding to the scanned value when scanning from
        a JSON object.  May be NULL otherwise.
    - ptr: Pointer to memory that the scanned value will be written to.
        For arrays, `ptr` should points to the first element and will not
        be not dereferenced.
    - p: DLite property describing the data to scan.

  Returns:
    - Number of characters consumed from `src` or a negative number on error.
 */
int scanobj(const char *src, const jsmntok_t *item, const char *key,
            void *ptr, const DLiteProperty *p)
{
  const char *val = src + item->start;
  int len = item->end - item->start;
  int keylen = strcspn(key, "\"' \n\t");

  switch (p->type) {

  case dliteDimension:
    {
      DLiteDimension *dim = ptr;
      if (item->type == JSMN_STRING) {
        if (dim->name) free(dim->name);
        if (dim->description) free(dim->description);
        memset(dim, 0, sizeof(DLiteDimension));
        dim->name = strndup(key, keylen);
        dim->description = strndup(val, len);
      } else if (item->type == JSMN_OBJECT) {
        int i;
        for (i=0; i < item->size; i++, dim++) {
          const jsmntok_t *k = item + 2*i + 1;
          const jsmntok_t *v = item + 2*i + 2;
          if (dim->name) free(dim->name);
          if (dim->description) free(dim->description);
          memset(dim, 0, sizeof(DLiteDimension));
          dim->name = strndup(src + k->start, k->end - k->start);
          dim->description = strndup(src + v->start, v->end - v->start);
        }
      } else
        return err(dliteValueError,
                   "expected JSON string or object, got \"%.*s\"", len, val);
    }
    break;

  case dliteProperty:
    {
      DLiteProperty *prop = ptr;
      const jsmntok_t *v = item + 1;
      int i, j;
      if (item->type != JSMN_OBJECT)
        return err(dliteValueError,
                   "expected JSON object, got \"%.*s\"", len, val);

      for (i=0; i < item->size; i++, prop++) {
        const jsmntok_t *t, *d;

        dlite_property_clear(prop);

        assert(v->type == JSMN_STRING);  // key must be a string
        prop->name = strndup(src + v->start, v->end - v->start);
        v++;

        if (v->type != JSMN_OBJECT) {
          dlite_property_clear(prop);
          return err(dliteValueError, "expected JSON object, got \"%.*s\"",
                     v->end - v->start, src + v->start);
        }

        if (!(t = jsmn_item(src, v, "type"))) {
          dlite_property_clear(prop);
          return errx(dliteValueError,
                      "missing property type: '%.*s'", len, val);
        }
        if (dlite_type_set_dtype_and_size(src + t->start,
                                          &prop->type, &prop->size)) {
          dlite_property_clear(prop);
          return -1;
        }

        if ((t = jsmn_item(src, v, "$ref")))
          prop->ref = strndup(src + t->start, t->end - t->start);
        else if (prop->type == dliteRef && (t = jsmn_item(src, v, "type")))
          prop->ref = strndup(src + t->start, t->end - t->start);

        if ((t = jsmn_item(src, v, "dims")) ||
            (t = jsmn_item(src, v, "shape"))) {
          if (t->type != JSMN_ARRAY) {
            dlite_property_clear(prop);
            return errx(dliteIndexError,
                        "property \"%.*s\": shape should be an array",
                        keylen, key);
          }
          prop->ndims = t->size;
          prop->shape = calloc(prop->ndims, sizeof(char *));
          for (j=0; j < prop->ndims; j++) {
            if (!(d = jsmn_element(src, t, j))) {
              dlite_property_clear(prop);
              return err(dliteIndexError,
                         "error parsing dimensions \"%.*s\" of property "
                         "\"%.*s\"", t->end - t->start, src + t->start,
                         keylen, key);
            }
            prop->shape[j] = strndup(src + d->start, d->end - d->start);
          }
        }

        if ((t = jsmn_item(src, v, "unit")))
          prop->unit = strndup(src + t->start, t->end - t->start);

        if ((t = jsmn_item(src, v, "description")))
          prop->description = strndup(src + t->start, t->end - t->start);

        v += jsmn_count(v) + 1;
      }
    }
    break;

  default:
    return err(dliteValueError,
               "object format is not supported for property type: %s",
               dlite_type_get_dtypename(p->type));
  }
  return len;
}


/*
  Scans property from `src` and write it to memory pointed to by `ptr`.

  The property is described by `p`.

  For arrays, `ptr` should points to the first element and will not be
  not dereferenced.  Evaluated dimension sizes are given by `shape`.

  The `flags` provides some format options.  If zero (default)
  strings are expected to be quoted.

  Returns number of characters consumed from `src` or a negative
  number on error.
 */
int dlite_property_scan(const char *src, void *ptr, const DLiteProperty *p,
                        const size_t *shape, DLiteTypeFlag flags)
{
  if (p->ndims) {
    int r, n;
    void *q = ptr;
    unsigned int ntokens=0;
    jsmntok_t *tokens=NULL, *t;
    jsmn_parser parser;
    jsmn_init(&parser);
    r = jsmn_parse_alloc(&parser, src, strlen(src), &tokens, &ntokens);
    if (r < 0) return err(dliteValueError, "error parsing input: %s", jsmn_strerror(r));
    t = tokens;
    r = scandim(0, src, &q, p, shape, flags, &t);
    n = tokens[0].end;
    free(tokens);
    if (r < 0) return r;
    return n;
  } else {
    return dlite_type_scan(src, -1, ptr, p->type, p->size, flags);
  }
}


/*
  Scan property from  JSON data.

  Arguments:
    - src: JSON data to scan.
    - item: Pointer into array of JSMN tokens corresponding to the item
        to scan.
    - key: The key corresponding to the scanned value when scanning from
        a JSON object.  May be NULL otherwise.
    - ptr: Pointer to memory that the scanned value will be written to.
        For arrays, `ptr` should points to the first element and will not
        be not dereferenced.
    - p: DLite property describing the data to scan.
    - shape: Evaluated shape of property to scan.
    - flags: Format options.  If zero (default) strings are expected to be
        quoted.

  Returns:
    - Number of characters consumed from `src` or a negative number on error.
 */
int dlite_property_jscan(const char *src, const jsmntok_t *item,
                         const char *key, void *ptr, const DLiteProperty *p,
                         const size_t *shape, DLiteTypeFlag flags)
{
  if (p->ndims) {
    void *q = ptr;
    jsmntok_t *t = (jsmntok_t *)item;
    int len = item->end - item->start;
    switch (item->type) {
    case JSMN_ARRAY:
      if (scandim(0, src, &q, p, shape, flags, &t)) return -1;
      break;
    case JSMN_OBJECT:
      return scanobj(src, item, key, ptr, p);
    default:
      return errx(dliteIndexError,
                 "property \"%s\" has %d dimensions, but got a scalar %.*s",
                  p->name, p->ndims, len, src + item->start);
    }
    return len;
  } else {
    return dlite_type_scan(src + item->start, item->end - item->start,
                           ptr, p->type, p->size, flags);
  }
}



/********************************************************************
 *  MetaModel - a data model for metadata
 *
 *  An interface for easy creation of metadata programically.
 *  This is especially useful in bindings to other languages like Fortran
 *  where code generation is more difficult.
 *
 ********************************************************************/

/* Internal struct used to hold data values in DLiteMetaModel */
typedef struct {
  char *name;         /* Name of corresponding property in metadata */
  void *data;         /* Pointer to allocated memory holding the data */
  char **strp;        /* Used for adding strings. */
} Value;

struct _DLiteMetaModel {
  char *uri;
  DLiteMeta *meta;
  char *iri;

  size_t *dimvalues;  /* Array of dimension values used for instance creation */

  size_t nvalues;     /* Number of property values */
  Value *values;      /* Pointer to property values, typically description
                         for metadata and data values for data instances. */

  /* Special storage of dimension, property and relation values */
  size_t ndims;
  size_t nprops;
  size_t nrels;
  DLiteDimension *dims;
  DLiteProperty *props;
  DLiteRelation *rels;
};


/*
  Create and return a new empty metadata model.  `iri` is optional and
  may be NULL.

  Returns NULL on error.
 */
DLiteMetaModel *dlite_metamodel_create(const char *uri, const char *metaid)
{
  DLiteMetaModel *model;

  if (!(model = calloc(1, sizeof(DLiteMetaModel)))) goto fail;
  if (!(model->uri = strdup(uri))) goto fail;
  if (!(model->meta = dlite_meta_get(metaid))) goto fail;
  if (!(model->dimvalues = calloc(model->meta->_ndimensions, sizeof(size_t))))
    goto fail;
  return model;
 fail:
  if (model) {
    if (model->uri) free(model->uri);
    if (model->meta) dlite_meta_decref(model->meta);
    free(model);
  }
  return err(dliteMemoryError, "allocation failure"), NULL;
}


/*
  Frees metadata model.
 */
void dlite_metamodel_free(DLiteMetaModel *model)
{
  size_t i;
  free(model->uri);
  dlite_meta_decref(model->meta);
  if (model->iri) free(model->iri);
  if (model->dimvalues) free(model->dimvalues);
  for (i=0; i < model->nvalues; i++) {
    free(model->values[i].name);
    if (model->values[i].strp) {
      free(*(model->values[i].strp));
      free(model->values[i].strp);
    }
  }
  if (model->values) free(model->values);
  if (model->dims) free(model->dims);
  if (model->props) free(model->props);
  if (model->rels) free(model->rels);
  free(model);
}


/*
  Sets actual value of dimension `name`, where `name` must correspond to
  a named dimension in the metadata of this model.
 */
int dlite_metamodel_set_dimension_value(DLiteMetaModel *model,
                                        const char *name,
                                        size_t value)
{
  int i;
  if ((i = dlite_meta_get_dimension_index(model->meta, name)) < 0)
    return errx(dliteIndexError, "Metadata for model '%s' has no such dimension: %s",
                model->uri, name);
  model->dimvalues[i] = value;
  return 0;
}


/*
  Adds a data value to `model` corresponding to property `name` of
  the metadata for this model.

  Note that `model` only stores a pointer to `value`.  This means
  that `value` must not be reallocated or free'ed while `model` is in
  use.

  Returns non-zero on error.
*/
int dlite_metamodel_add_value(DLiteMetaModel *model, const char *name,
                                  const void *value)
{
  size_t i;
  for (i=0; i < model->nvalues; i++)
    if (strcmp(name, model->values[i].name) == 0)
      return errx(dliteAttributeError, "Meta model '%s' already has value: %s", model->uri, name);
  return dlite_metamodel_set_value(model, name, value);
}


/*
  Like dlite_metamodel_add_value(), but if a value already exists, it
  is replaced instead of added.

  Returns non-zero on error.
*/
int dlite_metamodel_set_value(DLiteMetaModel *model, const char *name,
                              const void *value)
{
  size_t i;
  Value *v=NULL;
  if (!dlite_meta_has_property(model->meta, name))
    FAILCODE2(dliteAttributeError, "Metadata '%s' has no such property: %s", model->meta->uri, name);
  for (i=0; i < model->nvalues; i++)
    if (strcmp(name, model->values[i].name) == 0)
      v = model->values + i;

  if (v) {
    if (v->name) free(v->name);
    if (v->strp) {
      free(*v->strp);
      free(v->strp);
    }
  } else {
    if (!(model->values = realloc(model->values,
                                  sizeof(Value)*(model->nvalues+1))))
      FAILCODE(dliteMemoryError, "allocation failure");
    v = model->values + model->nvalues;
  }
  memset(v, 0, sizeof(Value));

  if (!(v->name = strdup(name)))
    FAILCODE(dliteMemoryError, "allocation failure");
  v->data = (void *)value;

  model->nvalues++;
  return 0;
 fail:
  return 1;
}


/*
  Adds a string to `model` corresponding to property `name` of the
  metadata for this model.

  Returns non-zero on error.
*/
int dlite_metamodel_add_string(DLiteMetaModel *model, const char *name,
                               const char *s)
{
  size_t i;
  for (i=0; i < model->nvalues; i++)
    if (strcmp(name, model->values[i].name) == 0)
      return errx(dliteMetadataExistError, "Meta model '%s' already has string: %s",
                  model->uri, name);
  return dlite_metamodel_set_string(model, name, s);
}


/*
  Like dlite_metamodel_add_string(), but if the string already exists, it
  is replaced instead of added.

  Returns non-zero on error.
*/
int dlite_metamodel_set_string(DLiteMetaModel *model, const char *name,
                               const char *s)
{
  size_t i;
  char *p;
  if (!(p = strdup(s)))
    FAILCODE(dliteMemoryError, "allocation failure");
  if (dlite_metamodel_set_value(model, name, NULL))
    goto fail;
  for (i=0; i < model->nvalues; i++)
    if (strcmp(name, model->values[i].name) == 0) {
      Value *v = model->values + i;
      assert(v->data == NULL);
      if (!(v->strp = malloc(sizeof(char **))))
        FAILCODE(dliteMemoryError, "allocation failure");
      *(v->strp) = p;
      v->data = v->strp;
      return 0;
    }
  abort();  /* should never be reached */
 fail:
  if (p) free(p);
  return 1;
}


/*
  Adds a dimension to the property named "dimensions" of the metadata
  for `model`.

  The name and description of the new dimension is given by `name` and
  `description`, respectively.  `description` may be NULL.

  Returns non-zero on error.
*/
int dlite_metamodel_add_dimension(DLiteMetaModel *model,
                                      const char *name,
                                      const char *description)
{
  size_t i;
  int idim = dlite_meta_get_dimension_index(model->meta, "ndimensions");
  if (idim < 0)
    FAILCODE1(dliteIndexError, "Metadata for '%s' must have dimension \"ndimensions\"", model->uri);

  for (i=0; i < model->ndims; i++)
    if (strcmp(name, model->dims[i].name) == 0) break;
  if (i < model->ndims)
    FAILCODE1(dliteIndexError, "A dimension named \"%s\" is already in model", name);

  if (!(model->dims = realloc(model->dims,
                               sizeof(DLiteDimension)*(model->ndims+1))))
    FAILCODE(dliteMemoryError, "allocation failure");
  memset(model->dims + model->ndims, 0, sizeof(DLiteDimension));


  if (!(model->dims[model->ndims].name = strdup(name)))
    FAILCODE(dliteMemoryError, "allocation failure");

  if (description &&
      !(model->dims[model->ndims].description = strdup(description)))
    FAILCODE(dliteMemoryError, "allocation failure");

  model->ndims++;
  model->dimvalues[idim]++;
  return 0;
 fail:
  return 1;
}


/*
  Adds a new property to the property named "properties" of the metadata
  for `model`.

  Arguments:
    - model: datamodel that the property is added to
    - name: name of new property
    - typename: type of new property, ex. "string80", "int64", "string",...
    - unit: unit of new type. May be NULL
    - description: description of property. May be NULL

  Use dlite_metamodel_add_property_dim() to add dimensions to the
  property.

  Returns non-zero on error.
*/
int dlite_metamodel_add_property(DLiteMetaModel *model,
                                 const char *name,
                                 const char *typename,
                                 const char *unit,
                                 const char *description)
{
  size_t i;
  DLiteProperty *p;
  DLiteType type;
  size_t size;
  int iprop = dlite_meta_get_dimension_index(model->meta, "nproperties");
  if (iprop < 0)
    FAILCODE1(dliteIndexError, "Metadata for '%s' must have dimension \"nproperties\"", model->uri);
  if (!dlite_meta_has_property(model->meta, "properties"))
    FAILCODE1(dliteValueError, "Metadata for '%s' must have property \"properties\"", model->uri);
  if (dlite_type_set_dtype_and_size(typename, &type, &size)) goto fail;

  for (i=0; i < model->nprops; i++)
    if (strcmp(name, model->props[i].name) == 0)
      FAILCODE1(dliteAttributeError, "A property named \"%s\" is already in model", name);


  if (!(model->props = realloc(model->props,
                               sizeof(DLiteProperty)*(model->nprops+1))))
    FAILCODE(dliteMemoryError, "allocation failure");
  p = model->props + model->nprops;
  memset(p, 0, sizeof(DLiteProperty));

  if (!(p->name = strdup(name)))
   FAILCODE(dliteMemoryError, "allocation failure");
  p->type = type;
  p->size = size;
  if (unit && !(p->unit = strdup(unit)))
   FAILCODE(dliteMemoryError, "allocation failure");
  if (description && !(p->description = strdup(description)))
    FAILCODE(dliteMemoryError, "allocation failure");

  model->nprops++;
  model->dimvalues[iprop]++;
  return 0;
 fail:
  return 1;
}


/*
  Add dimension expression `expr` to property `name` (which must have
  been added with dlite_metamodel_add_property()).

  Returns non-zero on error.
*/
int dlite_metamodel_add_property_dim(DLiteMetaModel *model,
                                  const char *name,
                                  const char *expr)
{
  size_t i;
  for (i=0; i < model->nprops; i++)
    if (strcmp(name, model->props[i].name) == 0)
      return dlite_property_add_dim(model->props + i, expr);
  return errx(dliteAttributeError, "Model '%s' has no such property: %s", model->uri, name);
}


/*
  If `model` is missing a value described by a property in its
  metadata, return a pointer to the name of the first missing value.

  If all values are assigned, NULL is returned.
 */
const char *dlite_metamodel_missing_value(const DLiteMetaModel *model)
{
  size_t i, j;
  for (i=0; i < model->meta->_nproperties; i++) {
    DLiteProperty *p = model->meta->_properties + i;
    if (strcmp(p->name, "dimensions") == 0) {
      continue;
    } else if (strcmp(p->name, "properties") == 0) {
      if (!model->props) return p->name;
    } else if (strcmp(p->name, "relations") == 0) {
      continue;
    } else {
      for (j=0; j < model->nvalues; j++)
        if (strcmp(model->values[j].name, p->name) == 0) break;
      if (j >= model->nvalues) return p->name;
    }
  }
  return NULL;
}


/*
  Returns a pointer to the value, dimension or property added with the
  given name.

  Returns NULL on error.
 */
const void *dlite_metamodel_get_property(const DLiteMetaModel *model,
                                         const char *name)
{
  size_t i;
  if (strcmp(name, "dimensions") == 0)
    return model->dims;
  if (strcmp(name, "properties") == 0)
    return model->props;
  if (strcmp(name, "relations") == 0)
    return model->rels;
  for (i=0; i < model->nvalues; i++)
    if (strcmp(name, model->values[i].name) == 0)
      return model->values[i].data;
  return err(dliteAttributeError, "Model '%s' has no such property: %s", model->uri, name), NULL;
}


/*
  Creates and return a new dlite metadata from `model`.

  If the metadata of `model` has properties "name", "version" and
  "namespace", they will automatically be set based on the uri of `model`.

  Returns NULL on error.
*/
DLiteMeta *dlite_meta_create_from_metamodel(DLiteMetaModel *model)
{
  DLiteMeta *retval=NULL, *meta=NULL;
  char *name=NULL, *version=NULL, *namespace=NULL;
  const char *missing;
  size_t i;

  if (dlite_meta_is_metameta(model->meta) &&
      dlite_meta_has_property(model->meta, "name") &&
      dlite_meta_has_property(model->meta, "version") &&
      dlite_meta_has_property(model->meta, "namespace")) {
    if (dlite_split_meta_uri(model->uri, &name, &version, &namespace))
      goto fail;
    dlite_metamodel_set_string(model, "name", name);
    dlite_metamodel_set_string(model, "version", version);
    dlite_metamodel_set_string(model, "namespace", namespace);
  }

  if ((missing = dlite_metamodel_missing_value(model)))
    FAILCODE2(dliteValueError, "Missing value for \"%s\" in metadata model: %s",
          missing, model->uri);

  if (!(meta = (DLiteMeta *)dlite_instance_create(model->meta,
                                                  model->dimvalues,
                                                  model->uri))) goto fail;

  for (i=0; i < model->meta->_nproperties; i++) {
    const void *src;
    void *dest;
    DLiteProperty *p = model->meta->_properties + i;
    size_t *shape = (p->ndims) ? DLITE_PROP_DIMS(meta, i) : NULL;

    src = dlite_metamodel_get_property(model, p->name);
    dest = dlite_instance_get_property_by_index((DLiteInstance *)meta, i);
    if ((src == NULL) && (dest == NULL)) {
      continue;
    }
    else if ((src != NULL) && (dest != NULL)) {
      if (dlite_type_ndcast(p->ndims,
                            dest, p->type, p->size, shape, NULL,
                            src, p->type, p->size, shape, NULL,
                            NULL)) goto fail;
    } else {
      goto fail;
    }
  }
  if (dlite_meta_init(meta)) goto fail;
  retval = meta;
 fail:
  if (name) free(name);
  if (version) free(version);
  if (namespace) free(namespace);
  if (!retval && meta) dlite_meta_decref(meta);
  return retval;
}
