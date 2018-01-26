#ifndef _DLITE_ENTITY_H
#define _DLITE_ENTITY_H

/**
  @file
  @brief API for instances and entities.

*/


/** Opaque type for a DLiteTriplet.

    Just a placeholder - not yet implemented... */
typedef struct _DLiteTriplet DLiteTriplet;



/** Initial segments of all DLite instances.

    For standard data instances, this header is simply followed by an
    array of dimension sizes (type: size_t) followed by property
    values.  Padding should be added between the properties, such that
    a pointer to `uuid` can be cast to a struct realising the
    instance.

    For example, lets say we have an instance named with two
    dimensions and two properties of type int and double,
    respectively.  We should then be able to access the members of
    this instance only from knowing the metadata (and the architecture
    and compiler-dependent padding):

        typedef struct {
          DLiteInstance_HEAD
          size_t N;   // first dimension
          size_t M;   // second dimension
          int n;      // first property
          double x;   // second property
        } MyInstance;

        MyInstance inst;
        DLiteInstance *instptr = &inst;
        pad0 = 4;     // typical padding on a x86_64 archecture
        double *xp;

        // Create a pointer to `inst.x` from `instptr`.  Note the
        // the added padding.
        xp = (double *)(instptr + sizeof(DLiteInstance) +
                        2*sizeof(size_t) + sizeof(int) + pad0);
        assert(*xp == inst.x);

    For this to work we need a portable and robust way to determine the
    padding.  TODO - check out that this is possible.
*/
#define DLiteInstance_HEAD                                                \
  const char *uuid; /*!< UUID for this data instance. If `url` is not */  \
                    /*!< NULL, it is generated from `url` (version 5, */  \
                    /*!< SHA-1 based UUID using the DNS namespace), */    \
                    /*!< otherwise it is a random (version 4) UUID. */    \
  const char *url;  /*!< Unique name or url to the data instance.  */     \
                    /*!< Can be NULL. */                                  \
  struct DLiteMeta *meta;  /*!< Pointer to the metadata describing this */\
                           /*!< instance. */



/** Initial segments of all DLite metadata.

    Since all metadata is an instance of its meta-metadata, it is also
    an instance.  It therefore starts with DLiteInstance_HEAD.

    It can be shown that DLiteMeta_HEAD can describe itself.  In
    principle we can therefore follow the `meta` to higher and higher
    levels of abstraction, until we reach the basic metadata schema,
    which has its `meta` field set to NULL.  However, at least
    initially, we will allow to set the `meta` field of any DLiteMeta
    to NULL, indicating that we are not interested in the next level
    of abstraction.
*/
#define DLiteMeta_HEAD                                                  \
  DLiteInstance_HEAD                                                    \
  /* Dimensions */                                                      \
  size_t ndims;             /*!< Number of dimensions. */               \
  size_t nprops;            /*!< Number of properties. */               \
  size_t ntriplets;         /*!< Number of relations.  Is always */     \
                            /*!< zero for Entities. */                  \
                                                                        \
  /* Properties */                                                      \
  const char *name;         /*!< Metadata name. */                      \
  const char *version;      /*!< Metadata version. */                   \
  const char *namespace;    /*!< Metadata namespace. */                 \
  const char *description;  /*!< Human description of this metadata. */ \
                                                                        \
  const char **dimnames;    /*!< Dimension names.*/                     \
  const char **dimdescr;    /*!< Dimension descriptions. */             \
                                                                        \
  const char **propnames;   /*!< Name of each property. */              \
  DLiteType *proptypes;     /*!< Type of each property. */              \
  int *proptypesizes;       /*!< Type size of each property. */         \
  int *propndims;           /*!< Number of dimensions for each property. */ \
  size_t **propdims;        /*!< Dimensions of each property. */        \
  const char *propdescr;    /*!< Description of each property. */       \
                                                                        \
  DLiteTriplet *triplets;   /*!< Array of relation triplets. */


/** Base definition of a DLite instance, that all instance (and
    metadata) objects can be cast to.  Is never actually
    instansiated. */
typedef struct _DLiteInstance {
  DLiteInstance_HEAD
} DLiteInstance;


/** Base definition of a DLite metadata, that all metadata objects can
    be cast to.

    It may be instansiated, e.g. by the basic metadata schema. */
typedef struct _DLiteMeta {
  DLiteMeta_HEAD
} DLiteMeta;


/** A DLite entity.

    This is the metadata for standard data objects.  Number of
    relations should always be zero. */
typedef struct _DLiteEntity {
  DLiteInstance_HEAD
  const char **propunits;   /*!< Unit of each property. */
} DLiteEntity;


/** A DLite Collection.

    Collections are a special type of instances that hold a set of
    instances and relations between them.

    In the current implementation, we allow the `meta` field to be NULL.

    A set of pre-defined relations are used to manage the collection itself.
    In order to not distinguish these relations from user-defined relations,
    their predicate are prefixed with a single underscore.  The pre-defined
    relations are:

        subject   predicate     object
        -------   -----------   ----------
        label     "_is-a"       "Instance"
        label     "_has-uuid"   uuid
        label     "_has-meta"   url
        label     "_dim-map"    instdim:colldim

    The "_dim-map" relation maps the name of a dimension in an
    instance (`instdim`) to a common dimension in the collection
    (`colldim`).  The object is the concatenation of `instdim`
    and `colldim` separated by a semicolon.
*/
typedef struct _DLiteCollection {
  DLiteInstance_HEAD
  size_t ninstances;          /*!< Number of instances. */
  size_t ntriplets;           /*!< Number of relations. */
  size_t ndimmaps;            /*!< Number of (common) dimensions. */

  DLiteInstance *instsances;  /*!< Array of instances. */
  DLiteTriplet *triplets;     /*!< Array of relation triplets. */
  const char **dimnames;      /*!< Name of each (common) dimension. */
  int *disizes;               /*!< Size of each (common) dimension. */

} DLiteCollection;



DLiteInstance *dlite_instance_create(DLiteEntity *meta, size_t, *dims);


DLiteInstance *dlite_instance_createv(DLiteEntity *meta, ...);

void dlite_instance_free(DLiteInstance *inst);


#endif /* _DLITE_ENTITY_H */
