#ifndef _DLITE_COLLECTION_H
#define _DLITE_COLLECTION_H

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
  size_t ntriplets;           /*!< Number of relations. */

  DLiteTriplet *triplets;     /*!< Array of relation triplets. */
  char **dimnames;            /*!< Name of each (common) dimension. */
  int *dimsizes;              /*!< Size of each (common) dimension. */
} DLiteCollection;


#endif /* _DLITE_COLLECTION_H */
