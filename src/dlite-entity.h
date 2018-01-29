#ifndef _DLITE_ENTITY_H
#define _DLITE_ENTITY_H

/**
  @file
  @brief API for instances and entities.

  There is only data and transformations.  All data are instances of
  their metadata and metadata are instances of their meta-metadata,
  etc...

  Instances
  ---------
  Instances are uniquely defined by their UUID.  In addition they may
  have a unique name, which we here refers to as their URL.  For
  instance, for an Entiry, the concatenated "namespace/name/version"
  string is its URL.  If an instance has an URL, the UUID is generated
  from it (as a version 5 SHA1-based UUID using the DNS namespace),
  otherwise a random UUID is generated (as a version 4 UUID).

  The memory allocated for each instance, may be casted to a struct
  starting with the members defined by the DLiteInstance_HEAD macro
  (containing the UUID, URL and pointer to the metadata).  This header
  is followed by:
    - the size of each dimension (size_t)
    - the value of each property
    - pointer to array of DLiteTriplet for each set of relations
      (entities has no set of relations)

  Note, for the casting to work, the members must be correctly
  alligned.  This is taken care about by dlite_instance_create().

  Metadata
  --------
  Since all metadata is an instance of its meta-metadata, it is also
  an instance.  The DLiteMeta_HEAD, which is common to all metadata,
  therefore starts with DLiteInstance_HEAD.

  It can be shown that DLiteMeta_HEAD can describe itself.  In
  principle we can therefore follow the `meta` to higher and higher
  levels of abstraction, until we reach the basic metadata schema,
  which has its `meta` field set to NULL.  However, at least
  initially, we will allow to set the `meta` field of any DLiteMeta
  to NULL, indicating that we are not interested in the next level
  of abstraction.

  The metadata for normal data instances are called Entities and are
  represented by DLiteEntity.
*/

#include "dlite-type.h"
#include "dlite-storage.h"




/**
  Initial segments of all DLite instances.
*/
#define DLiteInstance_HEAD                                                \
  char uuid[DLITE_UUID_LENGTH+1]; /*!< UUID for this data instance. */    \
  const char *url;                /*!< Unique name or url to the data */  \
                                  /*!< instance.  Can be NULL. */         \
  struct _DLiteMeta *meta;        /*!< Pointer to the metadata descri- */ \
                                  /*!< bing this instance. */


/**
  Initial segments of all DLite metadata.
*/
#define DLiteMeta_HEAD                                                      \
  DLiteInstance_HEAD                                                        \
                                                                            \
  /* Dimensions */                                                          \
  size_t ndims;             /*!< Number of dimensions. */                   \
  size_t nprops;            /*!< Number of properties. */                   \
  size_t nrelsets;          /*!< Number of relation sets. */                \
                                                                            \
  /* Properties */                                                          \
  const char *name;         /*!< Metadata name. */                          \
  const char *version;      /*!< Metadata version. */                       \
  const char *namespace;    /*!< Metadata namespace. */                     \
  const char *description;  /*!< Human description of this metadata. */     \
                                                                            \
  const char **dimnames;    /*!< Dimension names.*/                         \
  const char **dimdescr;    /*!< Dimension descriptions. */                 \
                                                                            \
  const char **propnames;   /*!< Name of each property. */                  \
  DLiteType *proptypes;     /*!< Type of each property. */                  \
  size_t *propsizes;        /*!< Type size of each property. */             \
  size_t *propndims;        /*!< Number of dimensions for each property. */ \
  size_t **propdims;           /*!< Array of dimension indices for each */  \
                            /*!< property. Shape: [nprops, npropdims_max] */\
  const char **propdescr;   /*!< Description of each property. */           \
                                                                            \
  /* Internal properties calculated from the properties above. */           \
  int refcount;             /*!< Reference count for instances. */          \
  size_t size;              /*!< Size of instance memory. */                \
  size_t *dimoffsets;       /*!< Memory offset of dimension lengths. */     \
  size_t *propoffsets;      /*!< Memory offset of property values. */       \
  size_t *reloffsets;       /*!< Memory offset of relation sets. */         \
                                                                            \
  /* Relations */                                                           \
  size_t *relcounts;        /*!< Number of relations in each set. */        \
  const char **reldescr;    /*!< Description of each relation set. */


/**
  Base definition of a DLite instance, that all instance (and
  metadata) objects can be cast to.  Is never actually
  instansiated.
*/
typedef struct _DLiteInstance {
  DLiteInstance_HEAD
} DLiteInstance;


/**
  Base definition of a DLite metadata, that all metadata objects can
  be cast to.

  It may be instansiated, e.g. by the basic metadata schema.
*/
typedef struct _DLiteMeta {
  DLiteMeta_HEAD
} DLiteMeta;


/**
  A DLite entity.

  This is the metadata for standard data objects.  Number of
  relations should always be zero.
*/
typedef struct _DLiteEntity {
  DLiteMeta_HEAD
  const char **propunits;   /*!< Unit of each property. */
} DLiteEntity;


/**
  A DLite Collection.

  Collections are a special type of instances that hold a set of
  instances and relations between them.

  In the current implementation, we allow the `meta` field to be NULL.

  A set of pre-defined relations are used to manage the collection itself.
  In order to not distinguish these relations from user-defined relations,
  their predicate are prefixed with a single underscore.  The pre-defined
  relations are:

  subject | predicate   | object
  ------- | ----------- | ----------
  label   | "_is-a"     | "Instance"
  label   | "_has-uuid" | uuid
  label   | "_has-meta" | url
  label   | "_dim-map"  | instdim:colldim

  The "_dim-map" relation maps the name of a dimension in an
  instance (`instdim`) to a common dimension in the collection
  (`colldim`).  The object is the concatenation of `instdim`
  and `colldim` separated by a colon.
*/
typedef struct _DLiteCollection {
  DLiteInstance_HEAD
  size_t ninstances;          /*!< Number of instances. */
  size_t ntriplets;           /*!< Number of relations. */

  char **labels;              /*!< Array of instances. */
  DLiteTriplet *triplets;     /*!< Array of relation triplets. */
  char **dimnames;            /*!< Name of each (common) dimension. */
  int *dimsizes;              /*!< Size of each (common) dimension. */
} DLiteCollection;


/**
  Returns a new uninitialised dlite instance of Entiry \a meta with
  dimensions \a dims.  Memory for all properties is allocated and set
  to zero.  The lengths of \a dims is found in `meta->ndims`.

  Increases the reference count of \a meta.

  On error, NULL is returned.
 */
DLiteInstance *dlite_instance_create(DLiteEntity *meta, size_t *dims,
                                     const char *id);

/**
  Loads instance identified by \a id from storage \a s and returns a
  new and fully initialised dlite instance.

  On error, NULL is returned.
 */
DLiteInstance *dlite_instance_load(DLiteStorage *s, const char *id);

/**
  Saves instance \a inst to storage \a s.  Returns non-zero on error.
 */
int dlite_instance_save(DLiteStorage *s, const DLiteInstance *inst);

/**
  Free's an instance and all arrays associated with dimensional properties.
  Decreases the reference count of the associated metadata.
 */
void dlite_instance_free(DLiteInstance *inst);


/**
  Loads Entity identified by \a id from storage \a s and returns a
  new and fully initialised DLiteEntity object.

  Its `meta` member will be NULL, so it will not refer to any
  meta-metadata.

  On error, NULL is returned.
 */
DLiteEntity *dlite_entity_load(DLiteStorage *s, const char *id);

/**
  Saves Entity to storage \a s.  Returns non-zero on error.
 */
int dlite_entity_save(DLiteStorage *s, const DLiteEntity *entity);

/**
  Increase reference count to Entity.
 */
void dlite_entity_incref(DLiteEntity *entity);

/**
  Decrease reference count to Entity.  If the reference count reaches
  zero, the Entity is free'ed.
 */
void dlite_entity_decref(DLiteEntity *entity);

/**
  Free's all memory used by \a entity and clear all data.
 */
void dlite_entity_clear(DLiteEntity *entity);


/********************************************************************
 *  Meta data
 *
 *  These functions are mainly used internally or by code generators.
 *  Do not waist time on them...
 ********************************************************************/

/**
  Initialises internal properties of \a meta.  This function should
  not be called before the non-internal properties has been initialised.

  Returns non-zero on error.
 */
int dlite_meta_postinit(DLiteMeta *meta);

/**
  Free's all memory used by \a meta and clear all data.
 */
void dlite_meta_clear(DLiteMeta *meta);


#endif /* _DLITE_ENTITY_H */
