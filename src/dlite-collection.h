#ifndef _DLITE_COLLECTION_H
#define _DLITE_COLLECTION_H

#include "triplestore.h"
#include "dlite-entity.h"
#include "dlite-type.h"



/**
  A DLite Collection.

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
  label   | "_has-meta"     | uri
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
typedef struct _DLiteCollection {
  DLiteInstance_HEAD
  /* dimensions */
  //size_t ndimensions;         /*!< Number of dimensions. */
  //size_t ninstances;          /*!< Number of instances. */
  //size_t ndimmaps;            /*!< Number of dimension maps. */
  size_t nrelations;          /*!< Number of relations. */
  size_t nrelitems;           /*!< Number of items in a relation, always 4. */

  /* properties */

  /** Pointer to array of relations.  This can safely be cast to
      ``char *relations[4]``, which is an 2D array of strings.
      Note that this pointer may change if `relations` is reallocated. */
  DLiteRelation *relations;

  /* internal data */
  TripleStore *rstore;       /*!< TripleStore managing the relations. */
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
  Returns a new collection with given id.  If `id` is NULL, a new
  random uuid is generated.

  Returns NULL on error.
 */
DLiteCollection *dlite_collection_create(const char *id);


/**
  Free's a collection and decreases the reference count of the
  associated metadata.
 */
void dlite_collection_free(DLiteCollection *coll);


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
  Initiates a DLiteCollectionState for dlite_collection_find().
*/
void dlite_collection_init_state(const DLiteCollection *coll,
                                 DLiteCollectionState *state);


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
DLiteInstance *dlite_collection_get(const DLiteCollection *coll,
                                    const char *label);



#endif /* _DLITE_COLLECTION_H */
