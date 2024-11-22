#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "utils/err.h"
#include "utils/sha3.h"
#include "utils/strutils.h"
#include "dlite-macros.h"
#include "dlite-store.h"
#include "dlite-mapping.h"
#include "dlite-entity.h"
#include "dlite-schemas.h"
#include "dlite-storage-plugins.h"
#include "dlite-collection.h"
#include "dlite.h"



/**************************************************************
 * Collection
 **************************************************************/

/* Initialise additional data in a collection */
int dlite_collection_init(DLiteInstance *inst)
{
  DLiteCollection *coll = (DLiteCollection *)inst;
  if (coll->rstore) return errx(dliteSystemError,
                                "triplestore already initialised");
  if (!(coll->rstore = triplestore_create())) return 1;
  return 0;
}


/* De-initialise additional data in a collection */
int dlite_collection_deinit(DLiteInstance *inst)
{
  DLiteCollection *coll = (DLiteCollection *)inst;
  DLiteCollectionState state;
  DLiteInstance *inst2;
  const DLiteRelation *r;

  /* Release references to all instances. */
  dlite_collection_init_state(coll, &state);
  while ((r=dlite_collection_find(coll,&state, NULL, "_has-uuid", NULL,
                                  NULL))) {
    if ((inst2 = dlite_instance_get(r->o))) {
      dlite_instance_decref(inst2);  // remove ref from collection
      dlite_instance_decref(inst2);  // remove local ref to inst2
    } else {
      warnx("cannot remove missing instance: %s", r->o);
    }
  }
  dlite_collection_deinit_state(&state);

  triplestore_free(coll->rstore);
  return 0;
}

/* Help function for dlite_collection_gethash().
   Returns 1 if a > b, 0 if a == b and -1 if a < b. */
int _cmp_triple(const void *a, const void *b) {
  const Triple *t1=*(Triple **)a, *t2=*(Triple **)b;
  int v;
  if ((v = strcmp(t1->s, t2->s))) return v;
  if ((v = strcmp(t1->p, t2->p))) return v;
  if ((v = strcmp(t1->o, t2->o))) return v;
  if (t1->d && !t2->d) return 1;
  if (!t1->d && t2->d) return -1;
  if (t1->d && t2->d && (v = strcmp(t1->d, t2->d))) return v;
  return 0;
}

/* Help function for dlite_collection_gethash().
   Update the the hash with content of triple `t`. */
void _sha3_update_triple(sha3_context *c, const Triple *t)
{
  sha3_Update(c, t->s, strlen(t->s));
  sha3_Update(c, t->p, strlen(t->p));
  sha3_Update(c, t->o, strlen(t->o));
  if (t->d) sha3_Update(c, t->d, strlen(t->d));
}

/* Calculate hash of a collection. */
int dlite_collection_gethash(const DLiteInstance *inst, uint8_t *hash,
                             int hashsize)
{
  DLiteCollection *coll = (DLiteCollection *)inst;
  sha3_context c;
  const uint8_t *buf;
  unsigned bitsize = hashsize * 8;
  TripleState state;
  const Triple *t, **triples=NULL;
  size_t i=0, n=triplestore_length(coll->rstore);
  int retval=1;

  /* Initiate calculation of hash */
  sha3_Init(&c, bitsize);
  sha3_SetFlags(&c, SHA3_FLAGS_KECCAK);

  if (coll->_parent) {
    sha3_Update(&c, coll->_parent->uuid, DLITE_UUID_LENGTH);
    sha3_Update(&c, coll->_parent->hash, DLITE_HASH_SIZE);
  }
  sha3_Update(&c, coll->meta->uri, strlen(coll->meta->uri));

  /* Calculate the hash of a sorted copy of the relations */
  triplestore_init_state(coll->rstore, &state);
  if (!(triples = malloc(n * sizeof(Triple *))))
    FAILCODE(dliteMemoryError, "allocation failure");
  while ((t = triplestore_next(&state))) {
    triples[i++] = t;
  }
  assert(i == n);
  qsort((void *)triples, n, sizeof(Triple *), _cmp_triple);
  for (i=0; i<n; i++) {
    if (strcmp(triples[i]->p, "_has-hash") == 0) continue;
    if (strcmp(triples[i]->p, "_has-uuid") == 0) {
      if ((t = triplestore_find_first(coll->rstore,
                                      triples[i]->s, "_has-hash", NULL,
                                      NULL))) {
        _sha3_update_triple(&c, t);
      } else {
        uint8_t hash[DLITE_HASH_SIZE];
        DLiteInstance * inst;
        char hex[2*DLITE_HASH_SIZE+1];
        if (!(inst = dlite_instance_get(triples[i]->o))) goto fail;
        if (dlite_instance_get_hash(inst, hash, DLITE_HASH_SIZE))
          FAILCODE1(dliteValueError,
                    "error calculating hash of instance '%s'", triples[i]->o);
        if (strhex_encode(hex, sizeof(hex), hash, DLITE_HASH_SIZE) < 0)
          FAILCODE1(dliteValueError,
                    "failed hex-encoding hash of '%s'", triples[i]->o);
        sha3_Update(&c, triples[i]->s, strlen(triples[i]->s));
        sha3_Update(&c, "_has-hash", 9);
        sha3_Update(&c, hex, 2*DLITE_HASH_SIZE);
      }
    }
    _sha3_update_triple(&c, triples[i]);
  }

  buf = sha3_Finalize(&c);
  memcpy(hash, buf, hashsize);
  retval = 0;
 fail:
  triplestore_deinit_state(&state);
  free((void *)triples);
  return retval;
}

/* Returns size of dimension number `i` or -1 on error. */
int dlite_collection_getdim(const DLiteInstance *inst, size_t i)
{
  DLiteCollection *coll = (DLiteCollection *)inst;
  if (i != 0) return errx(dliteIndexError,
                         "index out of range: %lu", (unsigned long)i);
  return triplestore_length(coll->rstore);
}

/* Loads instance relations to triplestore.  Returns -1 on error. */
int dlite_collection_loadprop(const DLiteInstance *inst, size_t i)
{
  int retval = 0;
  DLiteCollection *coll = (DLiteCollection *)inst;
  TripleState state;
  const Triple *t;
  if (i != 0) return errx(dliteIndexError,
                         "index out of range: %lu", (unsigned long)i);
  triplestore_clear(coll->rstore);
  if (triplestore_add_triples(coll->rstore, coll->relations, coll->nrelations))
    return -1;

  /* Iterate over all newly added triples and load instances corresponding
     to "_has-uuid" relations. */
  triplestore_init_state(coll->rstore, &state);
  while ((t = triplestore_find(&state, NULL, "_has-uuid", NULL, NULL))) {
    DLiteInstance *inst2 = dlite_instance_get(t->o);
    if (!inst2) retval = errx(dliteStorageLoadError,
                              "cannot get instance \"%s\" labeled \"%s\" "
                              "from collection \"%s\".  "
                              "Is DLITE_STORAGES properly set?",
                              t->o, t->s, coll->uuid);
  }
  triplestore_deinit_state(&state);

  return retval;
}

/* Saves triplestore to instance relations. Returns non-zero on error. */
int dlite_collection_saveprop(DLiteInstance *inst, size_t i)
{
  DLiteCollection *coll = (DLiteCollection *)inst;
  TripleState state;
  const Triple *t;
  int n, j=0;

  if ((n = dlite_instance_get_dimension_size_by_index(inst, i)) < 0) return -1;
  if (i != 0) return errx(dliteIndexError,
                          "index out of range: %lu", (unsigned long)i);

  triplestore_init_state(coll->rstore, &state);
  while ((t = triplestore_next(&state))) {
    assert(j < (int)coll->nrelations);
    triple_clean(coll->relations + j);
    triple_copy(coll->relations + j, t);
    j++;
  }
  triplestore_deinit_state(&state);

  assert(j == n);
  return 0;
}

/*
  Returns a new collection with given id.  If `id` is NULL, a new
  random uuid is generated.

  Returns NULL on error.

  Note:
  This is just a simple wrapper around dlite_instance_create().
 */
DLiteCollection *dlite_collection_create(const char *id)
{
  DLiteMeta *meta = dlite_meta_get(DLITE_COLLECTION_ENTITY);
  size_t  dims[] = {0};
  return (DLiteCollection *)dlite_instance_create(meta,  dims, id);
}


/*
  Increases reference count of collection `coll`.
 */
void dlite_collection_incref(DLiteCollection *coll)
{
  dlite_instance_incref((DLiteInstance *)coll);
}


/*
  Decreases reference count of collection `coll`.
 */
void dlite_collection_decref(DLiteCollection *coll)
{
  dlite_instance_decref((DLiteInstance *)coll);
}


/*
  Safe type casting from instance to collection.
 */
DLiteCollection *dlite_collection_from_instance(DLiteInstance *inst)
{
  if (strcmp(inst->meta->uuid, DLITE_COLLECTION_ENTITY) != 0)
    return errx(dliteTypeError,
                "cannot cast instance %s to a collection", inst->uuid), NULL;
  return (DLiteCollection *)inst;
}

/*
  Cast collection to instance - always possible.
 */
DLiteInstance *dlite_collection_to_instance(DLiteCollection *coll)
{
  return (DLiteInstance *)coll;
}


/*
  Loads collection with given id from storage `s`.  If `lazy` is zero,
  all its instances are also loaded.  Otherwise, instances are loaded
  on demand.

  Returns a new reference to the collection or NULL on error.
 */
DLiteCollection *dlite_collection_load(DLiteStorage *s, const char *id,
                                       int lazy)
{
  DLiteCollection *coll;
  DLiteCollectionState state;
  const Triple *t, *t2;

  if (!(coll = (DLiteCollection *)dlite_instance_load(s, id)))
    return NULL;

  if (lazy) {
    //dlite_storage_paths_append(s->location);
    return coll;
  }

  dlite_collection_init_state(coll, &state);
  while ((t = dlite_collection_find(coll, &state, NULL, "_has-uuid", NULL,
                                    NULL))) {
    if (!(t2 = dlite_collection_find_first(coll, t->s, "_has-meta", NULL,
                                           NULL)))
      FAILCODE1(dliteInconsistentDataError,
                "collection inconsistency - no \"_has-meta\" relation for "
                "instance: %s", t->s);
    if (strcmp(t2->o, DLITE_COLLECTION_ENTITY) == 0) {
      if (!dlite_collection_load(s, t->o, 0)) goto fail;
    } else {
      if (!dlite_instance_load(s, t->o)) goto fail;
    }
  }
  dlite_collection_deinit_state(&state);
  return coll;
 fail:
  dlite_collection_deinit_state(&state);
  if (coll) dlite_collection_decref(coll);
  return NULL;
}

/*
  Convinient function that loads a collection from `url`, which should
  be of the form "driver://location?options#id".
  The `lazy` argument has the same meaning as for dlite_collection_load().

  Returns a new reference to the collection or NULL on error.
 */
DLiteCollection *dlite_collection_load_url(const char *url, int lazy)
{
  char *str=NULL, *driver=NULL, *location=NULL, *options=NULL, *id=NULL;
  DLiteStorage *s=NULL;
  DLiteCollection *coll=NULL;
  if (!(str = strdup(url))) FAILCODE(dliteMemoryError, "allocation failure");
  if (dlite_split_url(str, &driver, &location, &options, &id)) goto fail;
  if (!id || !(coll = (DLiteCollection *)dlite_instance_get(id))) {
    err_clear();
    if (!(s = dlite_storage_open(driver, location, options))) goto fail;
    if (!(coll = dlite_collection_load(s, id, lazy))) goto fail;
  }
 fail:
  if (s) dlite_storage_close(s);
  if (str) free(str);
  return coll;
}


/*
  Saves collection and all its instances to storage `s`.
  Returns non-zero on error.
 */
int dlite_collection_save(DLiteCollection *coll, DLiteStorage *s)
{
  DLiteCollectionState state;
  DLiteInstance *inst;
  const DLiteMeta *e = dlite_get_collection_entity();
  int stat=0;
  if ((stat = dlite_instance_save(s, (DLiteInstance *)coll))) return stat;
  dlite_collection_init_state(coll, &state);
  while ((inst = dlite_collection_next(coll, &state))) {
    if (inst->meta == e)
      stat |= dlite_collection_save((DLiteCollection *)inst, s);
    else
      stat |= dlite_instance_save(s, inst);
  }
  dlite_collection_deinit_state(&state);
  return stat;
}

/*
  A convinient function that saves instance `inst` to the storage specified
  by `url`, which should be of the form "driver://path?options".
  Returns non-zero on error.
 */
int dlite_collection_save_url(DLiteCollection *coll, const char *url)
{
  int retval=1;
  char *str=NULL, *driver=NULL, *path=NULL, *options=NULL;
  DLiteStorage *s=NULL;
  if (!(str = strdup(url))) FAILCODE(dliteMemoryError, "allocation failure");
  if (dlite_split_url(str, &driver, &path, &options, NULL)) goto fail;
  if (!(s = dlite_storage_open(driver, path, options))) goto fail;
  retval = dlite_collection_save(coll, s);
 fail:
  if (s) dlite_storage_close(s);
  if (str) free(str);
  return retval;
}

/*
  Adds subject-predicate-object relation to collection.  `d` is the
  datatype of literal object. Returns non-zero on error.
 */
int dlite_collection_add_relation(DLiteCollection *coll, const char *s,
                                  const char *p, const char *o, const char *d)
{
  int stat = triplestore_add(coll->rstore, s, p, o, d);
  if (!stat)
    DLITE_PROP_DIM(coll, 0, 0) = coll->nrelations;
  return stat;
}


/*
  Remove matching relations.  Any of `s`, `p` or `o` may be NULL, allowing for
  multiple matches.  Returns the number of relations removed, or -1 on error.
 */
int dlite_collection_remove_relations(DLiteCollection *coll, const char *s,
                                      const char *p, const char *o,
                                      const char *d)
{
  int retval = triplestore_remove(coll->rstore, s, p, o, d);
  if (retval > -1)
    DLITE_PROP_DIM(coll, 0, 0) = coll->nrelations;
  return retval;
}


/*
  Initiates a DLiteCollectionState for dlite_collection_find() and
  dlite_collection_next().  The state must be deinitialised with
  dlite_collection_deinit_state().
*/
void dlite_collection_init_state(const DLiteCollection *coll,
                                 DLiteCollectionState *state)
{
  triplestore_init_state(coll->rstore, (TripleState *)state);
}


/*
  Deinitiates a TripleState initialised with dlite_collection_init_state().
*/
void dlite_collection_deinit_state(DLiteCollectionState *state)
{
  triplestore_deinit_state((TripleState *)state);
}


/*
  Resets `state` already initialised with dlite_collection_init_state().
*/
void dlite_collection_reset_state(DLiteCollectionState *state)
{
  triplestore_reset_state((TripleState *)state);
}


/*
  Finds matching relations.

  If `state` is NULL, only the first match will be returned.

  Otherwise, this function should be called iteratively.  Before the
  first call it should be provided a `state` initialised with
  dlite_collection_init_state().

  For each call it will return a pointer to triple matching `s`, `p`
  and `o`.  Any of these may be NULL, allowing for multiple matches.
  When no more matches can be found, NULL is returned.

  No other calls to dlite_collection_add(), dlite_collection_find() or
  dlite_collection_add_relation() should be done while searching.
 */
const DLiteRelation *dlite_collection_find(const DLiteCollection *coll,
                                           DLiteCollectionState *state,
                                           const char *s, const char *p,
                                           const char *o, const char *d)
{
  if (state)
    return (DLiteRelation *)triplestore_find((TripleState *)state, s, p, o, d);
  else
    return (DLiteRelation *)triplestore_find_first(coll->rstore, s, p, o, d);
}

/*
  Like dlite_collection_find(), but returns only a pointer to the
  first matching relation, or NULL if there are no matching relations.
 */
const DLiteRelation *dlite_collection_find_first(const DLiteCollection *coll,
                                                 const char *s, const char *p,
                                                 const char *o, const char *d)
{
  return dlite_collection_find(coll, NULL, s, p, o, d);
}

/*
  Adds instance `inst` to collection, making `coll` the owner of the instance.
  Hence `coll` "steals" the reference to `inst`.

  Returns non-zero on error.
 */
int dlite_collection_add_new(DLiteCollection *coll, const char *label,
                             DLiteInstance *inst)
{
  if (dlite_collection_find(coll, NULL, label, "_is-a", "Instance", NULL))
    return errx(dliteValueError,
                "instance with label '%s' is already in the collection",
                label);

  dlite_collection_add_relation(coll, label, "_is-a", "Instance", NULL);
  dlite_collection_add_relation(coll, label, "_has-uuid", inst->uuid,
                                "xsd:anyURI");
  dlite_collection_add_relation(coll, label, "_has-meta", inst->meta->uri, NULL);
  return 0;
}


/*
  Adds (reference to) instance `inst` to collection.  Returns non-zero on
  error.
 */
int dlite_collection_add(DLiteCollection *coll, const char *label,
                         DLiteInstance *inst)
{
  if (dlite_collection_add_new(coll, label, inst)) return 1;
  dlite_instance_incref(inst);
  return 0;
}

/*
  Removes instance with given label from collection.  Returns non-zero on
  error.
 */
int dlite_collection_remove(DLiteCollection *coll, const char *label)
{
  DLiteCollectionState state;
  DLiteInstance *inst;
  const DLiteRelation *r;
  if (dlite_collection_remove_relations(coll, label, "_is-a", "Instance", NULL) > 0) {
    r = dlite_collection_find(coll, NULL, label, "_has-uuid", NULL, NULL);
    assert(r);

    /* Removes reference hold by collection to the instance. We have
     to call dlite_instance_decref() twice, since dlite_instance_get()
     returns a new reference. */
    if ((inst = dlite_instance_get(r->o))) {
      dlite_instance_decref(inst);
      dlite_instance_decref(inst);
    } else {
      warn("cannot remove missing instance: %s", r->o);
    }

    /* FIXME - there is something wrong here... */
    //dlite_collection_init_state(coll, &state);
    //while ((r=dlite_collection_find(coll, &state, label, "_has-dimmap", NULL)))
    //  triplestore_remove_by_id(coll->rstore, r->o);
    //*dlite_collection_deinit_state(&state);
    UNUSED(state);

    dlite_collection_remove_relations(coll, label, "_has-uuid", NULL, NULL);
    dlite_collection_remove_relations(coll, label, "_has-meta", NULL, NULL);
    dlite_collection_remove_relations(coll, label, "_has-dimmap", NULL, NULL);
    return 0;
  }
  return 1;
}


/*
  Returns borrowed reference to instance with given label or NULL on error.
 */
const DLiteInstance *dlite_collection_get(const DLiteCollection *coll,
                                          const char *label)
{
  const DLiteRelation *r;
  if ((r = dlite_collection_find(coll, NULL, label, "_has-uuid", NULL,
                                 NULL))) {
    DLiteInstance *inst = dlite_instance_get(r->o);
    if (!inst) FAILCODE2(dliteKeyError,
                         "no such instance '%s' in collection '%s'",
                         r->o, coll->uuid);

    //assert(inst->_refcount >= 2);
    //dlite_instance_decref(inst);

    /* FIXME - this looks like a hack, should be solved properly... */
    if (inst->_refcount >= 2) dlite_instance_decref(inst);
    return inst;
  }
  errx(dliteValueError, "cannot load instance '%s' from collection", label);
 fail:
  return NULL;
}

/*
  Returns borrowed reference to instance with given id or NULL on error.
 */
const DLiteInstance *dlite_collection_get_id(const DLiteCollection *coll,
                                             const char *id)
{
  const DLiteRelation *r;
  char uuid[DLITE_UUID_LENGTH+1];
  if (dlite_get_uuid(uuid, id) < 0) return NULL;
  if ((r = dlite_collection_find(coll, NULL, NULL, "_has-uuid", uuid, NULL)))
    return dlite_instance_get(id);
  return NULL;
}

/*
  Returns a new reference to instance with given label.  If `metaid` is
  given, the returned instance is casted to this metadata.

  Returns NULL on error.
 */
DLiteInstance *dlite_collection_get_new(const DLiteCollection *coll,
                                        const char *label,
                                        const char *metaid)
{
  DLiteInstance *inst = (DLiteInstance *)dlite_collection_get(coll, label);
  if (!inst) return NULL;
  if (metaid) {
    if (!(inst = dlite_mapping(metaid, (const DLiteInstance **)&inst, 1)))
      return errx(dliteMappingError,
                  "cannot map instance labeled '%s' to '%s'",
                  label, metaid), NULL;
  } else {
    dlite_instance_incref(inst);
  }
  return inst;
}

/*
  Returns non-zero if collection `coll` contains an instance with the
  given label.
 */
int dlite_collection_has(const DLiteCollection *coll, const char *label)
{
  return (dlite_collection_find_first(coll, label, "_has-uuid", NULL,
                                      NULL)) ? 1 : 0;
}

/*
  Returns non-zero if collection `coll` contains a reference to an
  instance with UUID or uri that matches `id`.
 */
int dlite_collection_has_id(const DLiteCollection *coll, const char *id)
{
  char uuid[DLITE_UUID_LENGTH+1];
  if (dlite_get_uuid(uuid, id) < 0) return 0;
  return (dlite_collection_find_first(coll, NULL, "_has-uuid", uuid,
                                      NULL)) ? 1 : 0;
}


/*
  Iterates over a collection.

  Returns a borrowed reference to the next instance or NULL if all
  instances have been visited.
*/
DLiteInstance *dlite_collection_next(DLiteCollection *coll,
				     DLiteCollectionState *state)
{
  DLiteInstance *inst = dlite_collection_next_new(coll, state);
  if (inst) {
    assert(inst->_refcount >= 2);
    dlite_instance_decref(inst);
  }
  return inst;
}

/*
  Iterates over a collection.

  Returns a new reference to the next instance or NULL if all
  instances have been visited.
*/
DLiteInstance *dlite_collection_next_new(DLiteCollection *coll,
                                         DLiteCollectionState *state)
{
  UNUSED(coll);
  const Triple *t;
  while ((t = triplestore_find(state, NULL, "_has-uuid", NULL, NULL)))
    return dlite_instance_get(t->o);
  return NULL;
}


/*
  Returns the number of instances that are stored in the collection or
  -1 on error.
*/
int dlite_collection_count(DLiteCollection *coll)
{
  int count=0;
  TripleState state;
  triplestore_init_state(coll->rstore, &state);
  while (triplestore_find(&state, NULL, "_is-a", "Instance", NULL)) count++;
  triplestore_deinit_state(&state);
  return count;
}


/*
  Return pointer to the value for a pair of two criteria.

  Useful if one knows that there may only be one value.  The returned
  value is held by the collection and should be copied by the user
  since it may be overwritten by later calls to the collection.

  Parameters:
      s, p, o: Criteria to match. Two of these must be non-NULL.
      d: If not NULL, the required datatype of literal objects.
      fallback: Value to return if no matches are found.
      any: If non-zero, return first matching value.

  Returns a pointer to the value of the `s`, `p` or `o` that is NULL.
  On error NULL is returned.
 */
const char *dlite_collection_value(DLiteCollection *coll, const char *s,
                                   const char *p, const char *o, const char *d,
                                   const char *fallback, int any)
{
  return triplestore_value(coll->rstore, s, p, o, d, fallback, any);
}
