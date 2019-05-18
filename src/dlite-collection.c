#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "utils/err.h"
#include "dlite-macros.h"
#include "dlite-store.h"
#include "dlite-mapping.h"
#include "dlite-entity.h"
#include "dlite-schemas.h"
#include "dlite-collection.h"



/**************************************************************
 * Instance store
 **************************************************************/

/* Global store for all instances added to any collection

   Instances are uniquely identified by their uuid.  Hence, if
   instances added to collections have the same uuid, they are exactly
   the same instances regardless of whether they are added to
   different collections.  That implies:
     - it is an error to add instances with the same uuid pointing to
       different memory locations
     - all collections will return a pointer to the same memory region
       when queried for the same uuid
   We implement this by a global store shared by all collections.

   It is initialised at the first call to dlite_collection_create()
   and is automatically released at stop time.
 */
static DLiteStore *_istore = NULL;

/* Frees up a global instance store */
static void _istore_free(void)
{
  dlite_store_free(_istore);
  _istore = NULL;
}

/* Returns a new initializes instance store */
static DLiteStore *_istore_init()
{
  DLiteStore *store = dlite_store_create();
  atexit(_istore_free);
  return store;
}



/**************************************************************
 * Collection
 **************************************************************/

/* Initialise additional data in a collection */
int dlite_collection_init(DLiteInstance *inst)
{
  DLiteCollection *coll = (DLiteCollection *)inst;

  /* Initialise global store */
  if (!_istore) _istore = _istore_init();
  assert(_istore);

  /* Initialise tripletstore */
  coll->rstore =
    triplestore_create_external(&coll->relations, &coll->nrelations,
                                NULL, NULL);

  return 0;
}


/* De-initialise additional data in a collection */
int dlite_collection_deinit(DLiteInstance *inst)
{
  DLiteCollection *coll = (DLiteCollection *)inst;
  triplestore_free(coll->rstore);
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
  DLiteMeta *meta = dlite_meta_get(DLITE_COLLECTION_SCHEMA);
  size_t dims[] = {0, 4};
  return (DLiteCollection *)dlite_instance_create(meta, dims, id);
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
  Adds subject-predicate-object relation to collection.  Returns non-zero
  on error.
 */
int dlite_collection_add_relation(DLiteCollection *coll, const char *s,
                                  const char *p, const char *o)
{
  triplestore_add(coll->rstore, s, p, o);
  return 0;
}


/*
  Remove matching relations.  Any of `s`, `p` or `o` may be NULL, allowing for
  multiple matches.  Returns the number of relations removed, or -1 on error.
 */
int dlite_collection_remove_relations(DLiteCollection *coll, const char *s,
                                      const char *p, const char *o)
{
  int retval = triplestore_remove(coll->rstore, s, p, o);
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
  Finds matching relations.

  If `state` is NULL, only the first match will be returned.

  Otherwise, this function should be called iteratively.  Before the
  first call it should be provided a `state` initialised with
  dlite_collection_init_state().

  For each call it will return a pointer to triplet matching `s`, `p`
  and `o`.  Any of these may be NULL, allowing for multiple matches.
  When no more matches can be found, NULL is returned.

  No other calls to dlite_collection_add(), dlite_collection_find() or
  dlite_collection_add_relation() should be done while searching.
 */
const DLiteRelation *dlite_collection_find(const DLiteCollection *coll,
                                           DLiteCollectionState *state,
                                           const char *s, const char *p,
                                           const char *o)
{
  if (state)
    return (DLiteRelation *)triplestore_find((TripleState *)state, s, p, o);
  else
    return (DLiteRelation *)triplestore_find_first(coll->rstore, s, p, o);
}


/*
  Adds instance `inst` to collection, making `coll` the owner of the instance.

  Returns non-zero on error.
 */
int dlite_collection_add_new(DLiteCollection *coll, const char *label,
                             DLiteInstance *inst)
{
  if (!inst->meta)
    return err(1, "instance must have associated metadata to be added "
               "to a collection");
  if (dlite_collection_find(coll, NULL, label, "_is-a", "Instance"))
    return err(1, "instance with label '%s' is already in the collection",
               label);
  dlite_collection_add_relation(coll, label, "_is-a", "Instance");
  dlite_collection_add_relation(coll, label, "_has-uuid", inst->uuid);
  dlite_collection_add_relation(coll, label, "_has-meta", inst->meta->uri);
  dlite_store_add_new(_istore, inst);
  return 0;
}


/*
  Adds (reference to) instance `inst` to collection.  Returns non-zero on
  error.
 */
int dlite_collection_add(DLiteCollection *coll, const char *label,
                         DLiteInstance *inst)
{
  dlite_instance_incref(inst);
  if (dlite_collection_add_new(coll, label, inst)) return 1;
  return 0;
}

/*
  Removes instance with given label from collection.  Returns non-zero on
  error.
 */
int dlite_collection_remove(DLiteCollection *coll, const char *label)
{
  DLiteCollectionState state;
  const DLiteRelation *r;
  if (dlite_collection_remove_relations(coll, label, "_is-a", "Instance") > 0) {
    r = dlite_collection_find(coll, NULL, label, "_has-uuid", NULL);
    assert(r);
    dlite_store_remove(_istore, r->o);

    dlite_collection_init_state(coll, &state);
    while ((r=dlite_collection_find(coll,&state, label, "_has-dimmap", NULL)))
      triplestore_remove_by_id(coll->rstore, r->o);
    dlite_collection_deinit_state(&state);

    dlite_collection_remove_relations(coll, label, "_has-uuid", NULL);
    dlite_collection_remove_relations(coll, label, "_has-meta", NULL);
    dlite_collection_remove_relations(coll, label, "_has-dimmap", NULL);
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
  if ((r = dlite_collection_find(coll, NULL, label, "_has-uuid", NULL)))
    return dlite_store_get(_istore, r->o);
  return NULL;
}

/*
  Returns a new reference to instance with given label.  If `metaid` is
  given, the returned instance is casted to this metadata.

  Returns NULL on error.
 */
const DLiteInstance *dlite_collection_get_new(const DLiteCollection *coll,
                                              const char *label,
                                              const char *metaid)
{
  const DLiteInstance *inst;
  if (!(inst = dlite_collection_get(coll, label))) return NULL;
  if (metaid)
    inst = dlite_mapping(metaid, &inst, 1);
  else
    dlite_instance_incref((DLiteInstance *)inst);
  return inst;
}


/*
  Iterates over a collection.

  Returns the next instance or NULL if there are no more instances.
*/
DLiteInstance *dlite_collection_next(DLiteCollection *coll,
				     DLiteCollectionState *state)
{
  UNUSED(coll);
  const Triplet *t;
  while ((t = triplestore_find(state, NULL, "_has-uuid", NULL)))
    return dlite_store_get(_istore, t->o);
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
  while (triplestore_find(&state, NULL, "_is-a", "Instance")) count++;
  triplestore_deinit_state(&state);
  return count;
}
