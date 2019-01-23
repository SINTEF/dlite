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
  In DLite, instances are struct's starting with DLiteInstance_HEAD,
  which is a macro defining a few key fields that all instances has:
    - **uuid**: Instances are uniquely identified by their UUID.  If
      an instance has an URI, its UUID is generated from it (as a
      version 5 SHA1-based UUID using the DNS namespace).  Otherwise
      the UUID is random (generated as a version 4 UUID).
    - **uri**: An optional unique name identifying the instance (related
      to its UUID as described above).  For metadata the URI is mandantory
      and should be of the form "namespace/version/name".
    - **refcount**: Reference count.  Do not access this directly.
    - **meta**: Pointer to the metadata describing the instance.
  All instances may be cast to an DLiteInstance, which is a minimal
  struct representing all instances.


  Metadata
  --------
  In DLite, instances are described by specialised instances called
  metadata.  Metadata are struct's starting with the fields provided
  by the DLiteMeta_HEAD macro.  Since metadata themselves are
  instances, you can always cast a DLiteMeta (a struct representing
  any metadata) to a DLiteInstance.  This also means that metadata is
  described by their meta-metadata, and so forth.  To break this (in
  principle infinite) chain of abstractions, dlite provides a root
  abstraction, called BasicMetadataSchema, that all instances in DLite
  are instances of.  The BasicMetadataSchema, has itself as metadata
  and must therefore be able to describe itself.

  Most of the fields provided by the DLiteMeta_HEAD macro are not
  intended to be accessed by the user, except maybe for `ndimensions`,
  `nproperties`, `nrelations`, `dimensions`, `properties` and
  `relations`, which may be handy.


  Memory model
  ------------
  All instances, including metadata, uses the same memory model.  They
  are allocated in one chunk of memory holding both header, dimensions
  and property values.  The only exception are dimensional properties,
  whos values are allocated on the heap.  Care is taken to use the
  system padding rules when the memory is layed out, such that it can
  be mapped to a (possible generated) struct.  Allocating dimensional
  properties seperately has two benefits:
    - the size of the memory chunk for an instance is described by
      its metadata (it is independent on the length of the dimensions
      of the instance itself)
    - it allows to change the length of the dimensions of a data
      instance (but not for metadata, since they are immutable)

  The following table summarises the memory layout of an instance:

  | segment                       | nmemb             | size                  | offset               |
  | ----------------------------- | ----------------- | --------------------- | -------------------- |
  | header                        | 1                 | meta->headersize      | 0                    |
  | dimension values              | meta->ndimensions | sizeof(size_t)        | meta->dimoffset      |
  | property values               | meta->nproperties | [a]                   | meta->propoffsets    |
  | relation values               | meta->nrelations  | sizeof(DLiteRelation) | meta->reloffset      |
  | instance property offsets [b] | nproperties       | sizeof(size_t)        | meta->pooffset       |

    [a]: The size of properties depends on their `size` and whether
         they are dimensional.

    [b]: Only metadata instances has the last segment with instance
         property offsets.

  ### Header
  The header for all instances must start with `DLiteInstance_HEAD`.
  This is a bare minimal, but sufficient for most data instances.  It
  contains the UUID and (optionally) URI identifying the instance as
  well as a reference count and a pointer th the metadata describing
  the instance.  Metadata and special data instances like collections,
  extends this.

  The header of all metadata must start with `DLiteMeta_HEAD` (which
  itself starts with `DLiteInstance_HEAD`.

  ### Dimension values
  The length of each dimension of the current instance.

  ### Property values
  The value of each property of the current instance.  The metadata
  for this instance defines the type and size for all properties.  The
  size that the `i`th property occupies in an instance is given by
  `meta->properties[i].size`, except if the property has dimensions,
  in which case it is a pointer (of size `sizeof(void *)`) to a
  continuous allocated array with the property values.

  Some property types, like `dliteStringPtr` and `dliteProperty` may
  allocate additional memory.

  ### Relation values
  Array of relations for the current instance.

  ### Instance property offsets
  Memory offset of each property of its instances.  Don't access this
  array directly, use the `DLITE_PROP()` macro instead.  Note that
  only metadata instances has this segment.
*/

#include "boolean.h"
#include "dlite-misc.h"
#include "dlite-type.h"
#include "dlite-storage.h"
#include "dlite-arrays.h"


/** Function for additional initialisation of an instance.
    If defined, this function is called at end of dlite_instance_create().
    Returns non-zero on error. */
typedef int (*DLiteInit)(struct _DLiteInstance *inst);

/** Function for additional de-initialisation of a metadata instance.
    If defined, this function is called at beginning of
    dlite_instance_free().
    Returns non-zero on error. */
typedef int (*DLiteDeInit)(struct _DLiteInstance *inst);

///** Function for loading special properties.
//    Returns 1 if property `name` is loaded, 0 if `name` should be
//    loaded the normal way or -1 on error. */
//typedef int (*DLiteLoadProp(DLiteDataModel *d,
//                            const struct _DLiteInstance *inst,
//                            const char *name);
//
///** Function for saving special properties.
//    Returns 1 if property `name` is saved, 0 if `name` should be
//    saved the normal way or -1 on error. */
//typedef int (*DLiteSaveProp)(DLiteDataModel *d,
//                             const struct _DLiteInstance *inst,
//                             const char *name);


/** Expands to number of dimensions --> (size_t) */
#define DLITE_NDIM(inst) (((DLiteInstance *)(inst))->meta->ndimensions)

/** Expands to pointer to array of dimension values --> (size_t *) */
#define DLITE_DIMS(inst) \
  ((size_t *)((char *)(inst) + ((DLiteInstance *)(inst))->meta->dimoffset))

/** Expands to length of dimension `n` --> (size_t) */
#define DLITE_DIM(inst, n) (DLITE_DIMS(inst)[n])

/** Expands to array of dimension descriptions --> (DLiteDimension *) */
#define DLITE_DIMS_DESCR(inst) (((DLiteInstance *)(inst))->meta->dimensions)

/** Expands to description of dimensions `n` --> (DLiteDimension *) */
#define DLITE_DIM_DESCR(inst, n) \
  (((DLiteInstance *)(inst))->meta->dimensions + n)

/** Expands to number of properties (size_t). */
#define DLITE_NPROP(inst) (((DLiteInstance *)(inst))->meta->nproperties)

///** Expands to pointer to array of pointers to property values --> (void **) */
//#define DLITE_PROPS(inst)
//  ((void **)((char *)(inst) + ((DLiteInstance *)(inst))->meta->propptroffset))

/** Expands to pointer to the value of property `n` --> (void *)

    Note: for arrays this pointer must be dereferred (since all arrays
    are allocated on the heap). */
#define DLITE_PROP(inst, n) \
  ((void *)((char *)(inst) + ((DLiteInstance *)(inst))->meta->propoffsets[n]))

/** Expands to array of property descriptions --> (DLiteProperty *) */
#define DLITE_PROPS_DESCR(inst) (((DLiteInstance *)(inst))->meta->properties)

/** Expands to pointer to description of property `n` --> (DLiteProperty *) */
#define DLITE_PROP_DESCR(inst, n) \
  (((DLiteInstance *)(inst))->meta->properties + n)

/** Expands to number of relations --> (size_t) */
#define DLITE_NRELS(inst) (((DLiteInstance *)(inst))->meta->nrelations)

/** Expands to pointer to array of relations --> (DLiteRelation *) */
#define DLITE_RELS(inst)						\
  ((DLiteRelation *)((char *)(inst) +					\
		     ((DLiteInstance *)(inst))->meta->reloffset))

/** Expands to pointer to relation `n` --> (DLiteRelation *) */
#define DLITE_REL(inst, n) (DLITE_RELS(inst)[n])


/**
  Initial segment of all DLite instances.
*/
#define DLiteInstance_HEAD                                              \
  char uuid[DLITE_UUID_LENGTH+1]; /* UUID for this instance. */         \
  const char *uri;                /* Unique uri for this instance. */   \
                                  /*  May be NULL. */                   \
  int refcount;                   /* Number of references to this */    \
                                  /* instance. */                       \
  const struct _DLiteMeta *meta;  /* Pointer to the metadata descri- */ \
                                  /* bing this instance. */


/**
  Initial segment of all DLite metadata.  With instance we here refer
  to the dataset described by this metadata.

  For convinience we include `ndimensions`, `nproperties` and
  `nrelations` in the header of all MetaData, so we don't have to
  check the existence of these dimensions in the meta-metadata.  A
  consequence is that all meta-metadata must include these as the
  first 3 dimensions, regardless of whether they actually are used (as
  in the case for Entity, which don't have relations).
*/
#define DLiteMeta_HEAD                                                  \
  DLiteInstance_HEAD                                                    \
                                                                        \
  /* Convenient access to dimensions describing the instance */         \
  size_t ndimensions;  /* Number of dimensions in instance. */          \
  size_t nproperties;  /* Number of properties in instance. */          \
  size_t nrelations;   /* Number of relations in instance. */		\
                                                                        \
  /* Convenient access to description of instance */                    \
  DLiteDimension *dimensions;  /* Array of dimensions. */               \
  DLiteProperty *properties;   /* Array of properties. */               \
  DLiteRelation *relations;    /* Array of relations. */                \
                                                                        \
  /*  Function pointers used by instances */				\
  size_t headersize;     /* Size of instance header.  If zero, */	\
                         /* it defaults to sizeof(DLiteInstance) or */  \
                         /* sizeof(DLiteMeta). */                       \
  DLiteInit init;        /* Function initialising an instance. */       \
  DLiteDeInit deinit;    /* Function deinitialising an instance. */     \
                                                                        \
  /* Memory layout of instances */                                      \
  /* If `size` is zero, these values will automatically be */           \
  /* calculated. */                                                     \
  size_t dimoffset;      /* Offset of first dimension value. */         \
  size_t *propoffsets;   /* Pointer to array (in this metadata) of */	\
                         /* offsets to property values in instance. */  \
  size_t reloffset;      /* Offset of first relation value. */          \
  size_t pooffset;       /* Offset to array of property offsets */





/**
  Base definition of a DLite instance, that all instance (and
  metadata) objects can be cast to.  Is never actually
  instansiated.
*/
struct _DLiteInstance {
  DLiteInstance_HEAD  /*!< Common header for all instances. */
};


/**
  DLite dimension
*/
typedef struct _DLiteDimension {
  char *name;         /*!< Name of this dimension. */
  char *description;  /*!< Description of this dimension. */
} DLiteDimension;


/**
  DLite property

  E.g. if we have dimensions ["M", "N"] and dims is [1, 1, 0], it
  means that the data described by this property has dimensions
  ["N", "N", "M"].
*/
typedef struct _DLiteProperty {
  char *name;         /*!< Name of this property. */
  DLiteType type;     /*!< Type of the described data. */
  size_t size;        /*!< Size of one data element. */
  int ndims;          /*!< Number of dimension of the described */
                      /*   data.  Zero if scalar. */
  int *dims;          /*!< Array of dimension indices. May be NULL. */
  char *unit;         /*!< Unit of the described data. May be NULL. */
  char *description;  /*!< Human described of the described data. */
} DLiteProperty;



/**
  Base definition of a DLite metadata, that all metadata objects can
  be cast to.

  It may be instansiated, e.g. by the basic metadata schema.
*/
typedef struct _DLiteMeta {
  DLiteMeta_HEAD      /*!< Common header for all metadata. */
} DLiteMeta;



/* ================================================================= */
/**
 * @name Instances
 */
/* ================================================================= */
/** @{ */

/**
  Returns a new uninitialised dlite instance of Entiry `meta` with
  dimensions `dims`.  Memory for all properties is allocated and set
  to zero.  The lengths of `dims` is found in `meta->ndims`.

  Increases the reference count of `meta`.

  On error, NULL is returned.
 */
DLiteInstance *dlite_instance_create(const DLiteMeta *meta,
                                     const size_t *dims,
                                     const char *id);

/**
  Like dlite_instance_create() but takes the uri or uuid if the
  metadata as the first argument.  `dims`.  The lengths of `dims` is
  found in `meta->ndims`.

  Returns NULL on error.
*/
DLiteInstance *dlite_instance_create_from_id(const char *metaid,
                                             const size_t *dims,
                                             const char *id);

/**
  Increases reference count on `inst`.

  Returns the new reference count.
 */
int dlite_instance_incref(DLiteInstance *inst);


/**
  Decrease reference count to `inst`.  If the reference count reaches
  zero, the instance is free'ed.

  Returns the new reference count.
 */
int dlite_instance_decref(DLiteInstance *inst);


/**
  Loads instance identified by `id` from storage `s` and returns a
  new and fully initialised dlite instance.

  In case the storage only contains one instance, it is possible to
  set `id` to NULL.  However, it is an error to set `id` to NULL if the
  storage contains more than one instance.

  On error, NULL is returned.
 */
DLiteInstance *dlite_instance_load(const DLiteStorage *s, const char *id);

/**
  Like dlite_instance_load(), but allows casting the loaded instance
  into an instance of metadata identified by `metaid`.  If `metaid` is
  NULL, no casting is performed.

  For the cast to be successful requires that the correct translators
  have been registered.

  Returns NULL of error or if no translator can be found.

  @todo
    - implementation of metadata lookup
    - implementation of translators
    - implementation of a database of translator plugins
 */
DLiteInstance *dlite_instance_load_casted(const DLiteStorage *s,
                                          const char *id,
                                          const char *metaid);

/**
  Saves instance \a inst to storage \a s.  Returns non-zero on error.
 */
int dlite_instance_save(DLiteStorage *s, const DLiteInstance *inst);

/**
  A convinient function that loads an instance given an URL of the form

      driver://loc?options#id

  where `loc` corresponds to the `uri` argument of dlite_storage_open().

  Returns the instance or NULL on error.
 */
DLiteInstance *dlite_instance_load_url(const char *url);

/**
  A convinient function that saves instance `inst` to the storage specified
  by `url`, which should be of the form

      driver://loc?options

  Returns non-zero on error.
 */
int dlite_instance_save_url(const char *url, const DLiteInstance *inst);

/**
  Returns number of dimensions or -1 on error.
 */
int dlite_instance_get_ndimensions(const DLiteInstance *inst);

/**
  Returns number of properties or -1 on error.
 */
int dlite_instance_get_nproperties(const DLiteInstance *inst);

/**
  Returns size of dimension \a i or -1 on error.
 */
int dlite_instance_get_dimension_size_by_index(const DLiteInstance *inst,
                                               size_t i);

/**
  Returns a pointer to data corresponding to property with index \a i
  or NULL on error.
 */
void *dlite_instance_get_property_by_index(const DLiteInstance *inst, size_t i);

/**
  Sets property \a i to the value pointed to by \a ptr.
  Returns non-zero on error.
*/
int dlite_instance_set_property_by_index(DLiteInstance *inst, size_t i,
                                         const void *ptr);

/**
  Returns number of dimensions of property with index \a i or -1 on error.
 */
int dlite_instance_get_property_ndims_by_index(const DLiteInstance *inst,
                                               size_t i);

/**
  Returns size of dimension \a j in property \a i or -1 on error.
 */
int dlite_instance_get_property_dimsize_by_index(const DLiteInstance *inst,
                                                 size_t i, size_t j);

/**
  Returns size of dimension \a i or -1 on error.
 */
int dlite_instance_get_dimension_size(const DLiteInstance *inst,
                                      const char *name);

/**
  Returns a pointer to data corresponding to \a name or NULL on error.
 */
void *dlite_instance_get_property(const DLiteInstance *inst, const char *name);

/**
  Copies memory pointed to by \a ptr to property \a name.
  Returns non-zero on error.
*/
int dlite_instance_set_property(DLiteInstance *inst, const char *name,
                                const void *ptr);

/**
  Returns number of dimensions of property  \a name or -1 on error.
*/
int dlite_instance_get_property_ndims(const DLiteInstance *inst,
                                      const char *name);

/**
  Returns size of dimension \a j of property \a name or NULL on error.
*/
size_t dlite_instance_get_property_dimssize(const DLiteInstance *inst,
                                            const char *name, size_t j);

/**
  Returns non-zero if `inst` is a data instance.
 */
int dlite_instance_is_data(const DLiteInstance *inst);


/**
  Updates the size of all dimensions from.  The new dimension sizes are
  provided in `dims`, that must have length `inst->ndims`.  Dimensions
  corresponding to negative elements in `dims` will remain unchanged.

  All properties whos dimension are changed will be reallocated and
  new memory will be zeroed.  The values of properties with two or
  more dimensions, where any but the first dimension is updated,
  should be considered invalidated.

  Returns non-zero on error.
 */
int dlite_instance_set_dimension_sizes(DLiteInstance *inst, int *dims);

/**
  Like dlite_instance_set_dimension_sizes(), but only updates the size of
  dimension `i` to size `size`.  Returns non-zero on error.
 */
int dlite_instance_set_dimension_size_by_index(DLiteInstance *inst,
                                               size_t i, size_t size);

/**
  Like dlite_instance_set_dimension_sizes(), but only updates the size of
  dimension `name` to size `size`.  Returns non-zero on error.
 */
int dlite_instance_set_dimension_size(DLiteInstance *inst, const char *name,
                                      size_t size);

/**
  Copies instance `inst` to a newly created instance.

  If `newid` is NULL, the new instance will have no URI and a random UUID.
  If `newid` is a valid UUID, the new instance will have the given
  UUID and no URI.
  Otherwise, the URI of the new instance will be `newid` and the UUID
  assigned accordingly.

  Returns NULL on error.
 */
DLiteInstance *dlite_instance_copy(const DLiteInstance *inst,
                                   const char *newid);


/**
  Returns a new DLiteArray object for property number `i` in instance `inst`.

  The returned array object only describes, but does not own the
  underlying array data, which remains owned by the instance.

  Scalars are treated as a one-dimensional array or length one.

  Returns NULL on error.
 */
DLiteArray *
dlite_instance_get_property_array_by_index(const DLiteInstance *inst, size_t i);


/**
  Returns a new DLiteArray object for property `name` in instance `inst`.

  The returned array object only describes, but does not own the
  underlying array data, which remains owned by the instance.

  Scalars are treated as a one-dimensional array or length one.

  Returns NULL on error.
 */
DLiteArray *dlite_instance_get_property_array(const DLiteInstance *inst,
                                              const char *name);


/** @} */
/* ================================================================= */
/**
 * @name Metadata
 */
/* ================================================================= */
/** @{ */

/**
  Returns a new Entity created from the given arguments.
 */
DLiteMeta *
dlite_entity_create(const char *uri, const char *description,
                    size_t ndimensions, const DLiteDimension *dimensions,
                    size_t nproperties, const DLiteProperty *properties);

/**
  Increase reference count to meta-metadata.
 */
void dlite_meta_incref(DLiteMeta *meta);

/**
  Decrease reference count to meta-metadata.  If the reference count reaches
  zero, the meta-metadata is free'ed.
 */
void dlite_meta_decref(DLiteMeta *meta);

/**
  Loads metadata identified by `id` from storage `s` and returns a new
  fully initialised meta instance.
*/
DLiteMeta *dlite_meta_load(const DLiteStorage *s, const char *id);

/**
  Saves metadata `meta` to storage `s`.  Returns non-zero on error.
 */
int dlite_meta_save(DLiteStorage *s, const DLiteMeta *meta);

/**
  Returns index of dimension named `name` or -1 on error.
 */
int dlite_meta_get_dimension_index(const DLiteMeta *meta, const char *name);

/**
  Returns index of property named `name` or -1 on error.
 */
int dlite_meta_get_property_index(const DLiteMeta *meta, const char *name);

/**
  Returns a pointer to dimension with index `i` or NULL on error.
 */
const DLiteDimension *
dlite_meta_get_dimension_by_index(const DLiteMeta *meta, size_t i);

/**
  Returns a pointer to dimension named `name` or NULL on error.
 */
const DLiteDimension *dlite_meta_get_dimension(const DLiteMeta *meta,
                                               const char *name);

/**
  Returns a pointer to property with index `i` or NULL on error.
 */
const DLiteProperty *
dlite_meta_get_property_by_index(const DLiteMeta *meta, size_t i);

/**
  Returns a pointer to property named `name` or NULL on error.
 */
const DLiteProperty *dlite_meta_get_property(const DLiteMeta *meta,
                                             const char *name);


/**
  Returns non-zero if `meta` is meta-metadata (i.e. its instances are
  metadata).

  @note If `meta` contains either a "properties" property (of type
  DLiteProperty) or a "relations" property (of type DLiteRelation) in
  addition to a "dimensions" property (of type DLiteDimension), then
  it is able to describe metadata and is considered to be meta-metadata.
  Otherwise it is not.
*/
int dlite_meta_is_metameta(const DLiteMeta *meta);


/** @} */
/* ================================================================= */
/**
 * @name Metadata cache
 * Global metadata cache to avoid reloading entities for each time an
 * instance is created.
 */
/* ================================================================= */
/** @{ */

/**
  Frees up a global metadata store.  Will be called at program exit,
  but can be called at any time.
*/
void dlite_metastore_free(void);

/**
  Returns pointer to metadata for id `id` or NULL if `id` cannot be found.
*/
DLiteMeta *dlite_metastore_get(const char *id);

/**
  Adds metadata to global metadata store, giving away the ownership
  of `meta` to the store.  Returns non-zero on error.
*/
int dlite_metastore_add_new(const DLiteMeta *meta);

/**
  Adds metadata to global metadata store.  The caller keeps ownership
  of `meta`.  Returns non-zero on error.
*/
int dlite_metastore_add(const DLiteMeta *meta);

/** @} */

#endif /* _DLITE_ENTITY_H */
