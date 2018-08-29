#ifndef _DLITE_COLLECTION_H
#define _DLITE_COLLECTION_H

#include "triplestore.h"
#include "dlite-entity.h"
#include "dlite-type.h"


/**
  A DLite Collection.

  Collections are a special type of instances that hold a set of
  instances and relations between them.

  In the current implementation, we allow the `meta` field to be NULL.

  A set of pre-defined relations are used to manage the collection itself.
  In order to not distinguish these relations from user-defined relations,
  their predicate are prefixed with a single underscore.  The pre-defined
  relations are:

  subject | predicate       | object
  ------- | --------------- | ----------
  label   | "_is-a"         | "Instance"
  label   | "_has-uuid"     | uuid
  label   | "_has-meta"     | uri
  label   | "_has-dimmap"   | relation-id
  instdim | "_maps-to"      | colldim

  The "_has-dimmap" relations links an instance label to a "_maps-to"
  relation that maps a dimension in the instance (`instdim`) to a
  common dimension in the collection (`colldim`).

*/
typedef struct _DLiteCollection {
  DLiteInstance_HEAD
  size_t ndims;               /*!< Number of (common) dimensions. */
  size_t ninstances;          /*!< Number of instances. */
  size_t ntriplets;           /*!< Number of relations. */

  Triplestore *store;         /*!< Triplestore managing the relations. */

  DLiteInstance **instances;  /*!< Array of instances. */
  DLiteRelation *triplets;     /*!< Array of relation. */
  char **dimnames;            /*!< Name of each (common) dimension. */
  int *dimsizes;              /*!< Size of each (common) dimension. */
} DLiteCollection;


/** State used by dlite_collection_find(). */
typedef struct _TripleState DLiteCollectionState;


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
  Adds (reference to) instance `inst` to collection.  Returns non-zero on
  error.
 */
int dlite_collection_add(DLiteCollection *coll, const char *label,
                         const DLiteInstance *inst);


/**
  Removes instance with given label from collection.  Returns non-zero on
  error.
 */
int dlite_collection_remove(DLiteCollection *coll, const char *label);


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


#endif /* _DLITE_COLLECTION_H */
