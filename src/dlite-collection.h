#ifndef _DLITE_COLLECTION_H
#define _DLITE_COLLECTION_H

#include "triplestore.h"
#include "dlite-entity.h"
#include "dlite-type.h"


/**
  @file
  @brief A DLite Collection.

  Collections are a special type of instances that hold a set of
  instances and relations between them.

  A set of pre-defined relations are used to manage the collection itself.
  In order to not distinguish these relations from user-defined relations,
  their predicate are prefixed with a single underscore.  The pre-defined
  relations are:

  subject | predicate       | object
  ------- | --------------- | ----------
  label   | "_is-a"         | "Instance"
  label   | "_has-uuid"     | uuid
  label   | "_has-meta"     | metadata uri
  label   | "_has-dimmap"   | relation-id  -> (instdim, "_maps-to", coldim)
  instdim | "_maps-to"      | colldim
  coldim  | "_has-size"     | size

  The "_has-dimmap" relations links an instance label to a "_maps-to"
  relation that maps a dimension in the instance (`instdim`) to a
  common dimension in the collection (`colldim`).

  Note that we compared to SOFT, have added "n-rel-items" as a dimension
  such that the relations can be treated as an ordinary 2D array of
  strings.
*/

/** A specialised instance for collections */
typedef struct _DLiteCollection {
  /* -- extended header */
  DLiteInstance_HEAD
  TripleStore *rstore;       /*!< TripleStore managing the relations. */

  /* -- dimensions */
  size_t nrelations;         /*!< Number of relations. */

  /* -- properties */

  /** Pointer to array of relations.  This can safely be cast to
      ``char *relations[4]``, which is an 2D array of strings.
      Note that this pointer may change if `relations` is reallocated. */
  DLiteRelation *relations;

  /* -- relations */

  /* -- propdims */
  size_t __propdims[1];

} DLiteCollection;


/** State used by dlite_collection_find(). */
typedef struct _TripleState DLiteCollectionState;

/**
  Initiates a collection instance.

  Returns non-zero on error.
 */
int dlite_collection_init(DLiteInstance *inst);

/**
  Deinitiates a collection instance.

  Returns non-zero on error.
 */
int dlite_collection_deinit(DLiteInstance *inst);

/**
  Returns size of dimension number `i` or -1 on error.
*/
int dlite_collection_getdim(const DLiteInstance *inst, size_t i);

/**
  Loads instance relations to triplestore.  Returns -1 on error.
*/
int dlite_collection_loadprop(const DLiteInstance *inst, size_t i);

/**
  Saves triplestore to instance relations. Returns non-zero on error.
*/
int dlite_collection_saveprop(DLiteInstance *inst, size_t i);


/**
  Returns a new collection with given id.  If `id` is NULL, a new
  random uuid is generated.

  Returns NULL on error.

  @note
  This is just a simple wrapper around dlite_instance_create().
 */
DLiteCollection *dlite_collection_create(const char *id);


/**
  Increases reference count of collection `coll`.
 */
void dlite_collection_incref(DLiteCollection *coll);


/**
  Decreases reference count of collection `coll`.
 */
void dlite_collection_decref(DLiteCollection *coll);


/**
  Loads collection with given id from storage `s`.  If `lazy` is zero,
  all its instances are loaded immediately.  Otherwise, instances are
  first loaded on demand.  Returns non-zero on error.
 */
DLiteCollection *dlite_collection_load(DLiteStorage *s, const char *id,
                                       int lazy);

/**
  Convinient function that loads a collection from `url`, which should
  be of the form "driver://location?options#id".
  The `lazy` argument has the same meaning as for dlite_collection_load().
  Returns non-zero on error.
 */
DLiteCollection *dlite_collection_load_url(const char *url, int lazy);


/**
  Saves collection and all its instances to storage `s`.
  Returns non-zero on error.
 */
int dlite_collection_save(DLiteCollection *coll, DLiteStorage *s);

/**
  A convinient function that saves instance `inst` to the storage specified
  by `url`, which should be of the form "driver://path?options".
  Returns non-zero on error.
 */
int dlite_collection_save_url(DLiteCollection *coll, const char *url);


/**
  Adds subject-predicate-object relation to collection.  Returns non-zero
  on error.
 */
int dlite_collection_add_relation(DLiteCollection *coll, const char *s,
                                  const char *p, const char *o);


/**
  Remove matching relations.  Any of `s`, `p` or `o` may be NULL, allowing for
  multiple matches.  Returns the number of relations removed, or -1 on error.
 */
int dlite_collection_remove_relations(DLiteCollection *coll, const char *s,
                                      const char *p, const char *o);


/**
  Initiates a DLiteCollectionState for dlite_collection_find() and
  dlite_collection_next().  The state must be deinitialised with
  dlite_collection_deinit_state().
*/
void dlite_collection_init_state(const DLiteCollection *coll,
                                 DLiteCollectionState *state);

/**
  Deinitiates a TripleState initialised with dlite_collection_init_state().
*/
void dlite_collection_deinit_state(DLiteCollectionState *state);

/**
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
					   const char *o);

/**
  Like dlite_collection_find(), but returns only a pointer to the
  first matching relation, or NULL if there are no matching relations.
 */
const DLiteRelation *dlite_collection_find_first(const DLiteCollection *coll,
                                                 const char *s, const char *p,
                                                 const char *o);


/**
  Adds instance `inst` to collection, making `coll` the owner of the instance.

  Returns non-zero on error.
 */
int dlite_collection_add_new(DLiteCollection *coll, const char *label,
                             DLiteInstance *inst);

/**
  Adds (reference to) instance `inst` to collection.  Returns non-zero on
  error.
 */
int dlite_collection_add(DLiteCollection *coll, const char *label,
                         DLiteInstance *inst);


/**
  Removes instance with given label from collection.  Returns non-zero on
  error.
 */
int dlite_collection_remove(DLiteCollection *coll, const char *label);


/**
  Returns borrowed reference to instance with given label or NULL on error.
 */
const DLiteInstance *dlite_collection_get(const DLiteCollection *coll,
                                          const char *label);

/**
  Returns borrowed reference to instance with given id or NULL on error.
 */
const DLiteInstance *dlite_collection_get_id(const DLiteCollection *coll,
                                             const char *id);

/**
  Returns a new reference to instance with given label.  If `metaid` is
  given, the returned instance is casted to this metadata.

  Returns NULL on error.
 */
DLiteInstance *dlite_collection_get_new(const DLiteCollection *coll,
                                        const char *label,
                                        const char *metaid);

/**
  Returns non-zero if collection `coll` contains an instance with the
  given label.
 */
int dlite_collection_has(const DLiteCollection *coll, const char *label);

/**
  Returns non-zero if collection `coll` contains a reference to an
  instance with UUID or uri that matches `id`.
 */
int dlite_collection_has_id(const DLiteCollection *coll, const char *id);


/**
  Iterates over a collection.

  Returns a borrowed reference to the next instance or NULL if all
  instances have been visited.
*/
DLiteInstance *dlite_collection_next(DLiteCollection *coll,
				     DLiteCollectionState *state);

/**
  Iterates over a collection.

  Returns a new reference to the next instance or NULL if all
  instances have been visited.
*/
DLiteInstance *dlite_collection_next_new(DLiteCollection *coll,
                                         DLiteCollectionState *state);

/**
  Returns the number of instances that are stored in the collection or
  -1 on error.
*/
int dlite_collection_count(DLiteCollection *coll);


#endif /* _DLITE_COLLECTION_H */
