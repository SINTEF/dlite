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

  | segment         | nmemb             | size                  | offset                  |
  | --------------- | ----------------- | --------------------- | ----------------------- |
  | header          | 1                 | meta->headersize      | 0                       |
  | dimensions      | meta->ndimensions | sizeof(size_t)        | meta->dimoffset         |
  | properties      | meta->nproperties | [a]                   | meta->propoffsets       |
  | relations       | meta->nrelations  | sizeof(DLiteRelation) | meta->reloffset         |
  | propdims        | meta->npropdims   | sizeof(size_t)        | meta->propdimsoffset    |
  | propdiminds [b] | nproperties       | sizeof(size_t)        | meta->propdimindsoffset |
  | propoffsets [b] | nproperties       | sizeof(size_t)        | PROPOFFSETSOFFSET(meta) |

    [a]: The size of properties depends on their `size` and whether
         they are dimensional or not.

    [b]: Only metadata instances have the last two segments.

  ### header
  The header for all instances must start with `DLiteInstance_HEAD`.
  This is a bare minimal, but sufficient for most data instances.  It
  contains the UUID and (optionally) URI identifying the instance as
  well as a reference count and a pointer th the metadata describing
  the instance.  Metadata and special data instances like collections,
  extends this.

  The header of all metadata must start with `DLiteMeta_HEAD` (which
  itself starts with `DLiteInstance_HEAD`.

  ### dimensions
  The length of each dimension of the current instance.

  ### properties
  The value of each property of the current instance.  The metadata
  for this instance defines the type and size for all properties.  The
  size that the `i`th property occupies in an instance is given by
  `meta->properties[i].size`, except if the property has dimensions,
  in which case it is a pointer (of size `sizeof(void *)`) to a
  continuous allocated array with the property values.

  Some property types, like `dliteStringPtr` and `dliteProperty` may
  allocate additional memory.

  ### relations
  Array of relations for the current instance.

  ### propdims
  Array of evaluated property dimension sizes (calculated from
  `dimensions`).  Use the DLITE_PROP_DIM() macro to access these
  values.

  ### propdiminds
  Array with indices into `propdims` for the first dimension of each
  property.  Note that only metadata has this segment.

  ### propoffsets
  Memory offset of each property of its instances.  Don't access this
  array directly, use the DLITE_PROP() macro instead.  Note that only
  metadata instances has this segment.

  Note that the allocated size of data instances are fully defined by
  their metadata.  However, this is not true for metadata since it
  depends on `nproperties` - the number of properties it defines.  Use
  the DLITE_INSTANCE_SIZE() macro to get the allocated size of an
  instance.
*/

#include <stddef.h>
#include "utils/boolean.h"
#include "utils/jsmnx.h"
#include "dlite-misc.h"
#include "dlite-type.h"
#include "dlite-storage.h"
#include "dlite-arrays.h"


/**
 * @name Typedefs and structs
 */
/** @{ */

/** Function for additional initialisation of an instance.
    If defined, this function is called at end of dlite_instance_create().
    Returns non-zero on error. */
typedef int (*DLiteInit)(struct _DLiteInstance *inst);

/** Function for additional de-initialisation of a metadata instance.
    If defined, this function is called at beginning of
    dlite_instance_free().
    Returns non-zero on error. */
typedef int (*DLiteDeInit)(struct _DLiteInstance *inst);

/** Function for accessing internal dimension sizes of extended
    metadata.  If provided it will be called by dlite_instance_save(),
    dlite_instance_get_dimension_size() and related functions.
    Returns size of dimension `i` or -1 on error. */
typedef int (*DLiteGetDimension)(const DLiteInstance *inst, size_t i);

/** Function for setting internal dimension sizes of extended
    metadata.  If provided, it will be called by
    dlite_instance_set_dimension_size() and related functions.
    Returns zero on success.  If the extended metadata does not support
    setting size of dimension `i`, 1 is returned. On other errors, a
    negative value is returned. */
typedef int (*DLiteSetDimension)(DLiteInstance *inst, size_t i, size_t value);

/** Function used by extended metadata to load internal state from
    property number `i`.  If provided, this function will be called by
    dlite_instance_set_property() and related functions.
    Returns non-zero on error. */
typedef int (*DLiteLoadProperty)(const DLiteInstance *inst, size_t i);

/** Function used by extended metadata to save internal state to
    property number `i`.  If provided, this function will be called by
    dlite_instance_save(), dlite_instance_get_property() and related
    functions.
    Returns non-zero on error. */
typedef int (*DLiteSaveProperty)(DLiteInstance *inst, size_t i);



/**
  Initial segment of all DLite instances.
*/
#define DLiteInstance_HEAD                                              \
  char uuid[DLITE_UUID_LENGTH+1]; /* UUID for this instance. */         \
  const char *uri;                /* Unique uri for this instance. */   \
                                  /* May be NULL. */                    \
  int _refcount;                  /* Number of references to this */    \
                                  /* instance. */                       \
  const struct _DLiteMeta *meta;  /* Pointer to the metadata descri- */ \
                                  /* bing this instance. */             \
  const char *iri;                /* Unique IRI to corresponding */     \
                                  /* entity in an ontology. May be */   \
                                  /* NULL. */

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
  size_t _ndimensions;  /* Number of dimensions in instance. */         \
  size_t _nproperties;  /* Number of properties in instance. */         \
  size_t _nrelations;   /* Number of relations in instance. */          \
                                                                        \
  /* Convenient access to description of instance */                    \
  DLiteDimension *_dimensions;  /* Array of dimensions. */              \
  DLiteProperty *_properties;   /* Array of properties. */              \
  DLiteRelation *_relations;    /* Array of relations. */               \
                                                                        \
  /* Function pointers used by instances */                             \
  size_t _headersize;     /* Size of instance header.  If zero, */      \
                          /* it defaults to sizeof(DLiteInstance) or */ \
                          /* sizeof(DLiteMeta). */                      \
  DLiteInit _init;        /* Function initialising an instance. */      \
  DLiteDeInit _deinit;    /* Function deinitialising an instance. */    \
  DLiteGetDimension _getdim;   /* Gets dim. size from internal state.*/ \
  DLiteSetDimension _setdim;   /* Sets dim. size of internal state. */  \
  DLiteLoadProperty _loadprop; /* Loads internal state from prop. */    \
  DLiteSaveProperty _saveprop; /* Saves internal state to prop. */      \
                                                                        \
  /* Property dimension sizes of instances */                           \
  /* Automatically assigned by dlite_meta_init() */                     \
  size_t _npropdims;      /* Total number of property dimensions. */    \
  size_t *_propdiminds;   /* Pointer to array (within this metadata) */ \
                          /* of `propdims` indices to first property */ \
                          /* dimension. Length: nproperties */          \
                                                                        \
  /* Memory layout of instances */                                      \
  /* If `size` is zero, these values will automatically be assigned */  \
  size_t _dimoffset;      /* Offset of first dimension value. */        \
  size_t *_propoffsets;   /* Pointer to array (in this metadata) of */  \
                          /* offsets to property values in instance. */ \
  size_t _reloffset;      /* Offset of first relation value. */         \
  size_t _propdimsoffset; /* Offset to `propdims` array. */             \
  size_t _propdimindsoffset; /* Offset of `propdiminds` array. */


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
struct _DLiteDimension {
  char *name;         /*!< Name of this dimension. */
  char *description;  /*!< Description of this dimension. */
};


/**
  DLite property

  E.g. if we have dimensions ["M", "N"] and dims is [1, 1, 0], it
  means that the data described by this property has dimensions
  ["N", "N", "M"].
*/
struct _DLiteProperty {
  char *name;         /*!< Name of this property. */
  DLiteType type;     /*!< Type of the described data. */
  size_t size;        /*!< Size of one data element. */
  int ndims;          /*!< Number of dimension of the described
                           data.  Zero if scalar. */
  char **dims;        /*!< Array of dimension strings.  May be NULL. */
  char *unit;         /*!< Unit of the described data. May be NULL. */
  char *iri;          /*!< Unique IRI to corresponding entity in an
                           ontology. */
  char *description;  /*!< Human described of the described data. */
};



/**
  Base definition of a DLite metadata, that all metadata objects can
  be cast to.

  It may be instansiated, e.g. by the basic metadata schema.
*/
typedef struct _DLiteMeta {
  DLiteMeta_HEAD      /*!< Common header for all metadata. */
} DLiteMeta;


/**
  Opaque datatype used for metadata models.
 */
typedef struct _DLiteMetaModel DLiteMetaModel;


/** @} */
/**
 * @name Macros
 */
/** @{ */

/** Expands to number of dimensions --> (size_t) */
#define DLITE_NDIM(inst) (((DLiteInstance *)(inst))->meta->_ndimensions)

/** Expands to pointer to array of dimension values --> (size_t *) */
#define DLITE_DIMS(inst) \
  ((size_t *)((char *)(inst) + ((DLiteInstance *)(inst))->meta->_dimoffset))

/** Expands to length of dimension `n` --> (size_t) */
#define DLITE_DIM(inst, n) (DLITE_DIMS(inst)[n])

/** Expands to array of dimension descriptions --> (DLiteDimension *) */
#define DLITE_DIMS_DESCR(inst) (((DLiteInstance *)(inst))->meta->_dimensions)

/** Expands to description of dimensions `n` --> (DLiteDimension *) */
#define DLITE_DIM_DESCR(inst, n) \
  (((DLiteInstance *)(inst))->meta->_dimensions + n)

/** Expands to number of properties --> (size_t). */
#define DLITE_NPROP(inst) (((DLiteInstance *)(inst))->meta->_nproperties)

/** Expands to pointer to the value of property `n` --> (void *)

    Note: for arrays this pointer must be dereferred (since all arrays
    are allocated on the heap). */
#define DLITE_PROP(inst, n) \
  ((void *)((char *)(inst) + ((DLiteInstance *)(inst))->meta->_propoffsets[n]))

/** Expands to number of dimensions of property `n` --> (int) */
#define DLITE_PROP_NDIM(inst, n) \
  (((DLiteInstance *)(inst))->meta->_properties[n].ndims)

/** Expands to array of dimension sizes for property `n` --> (size_t *) */
#define DLITE_PROP_DIMS(inst, n) \
  ((size_t *)((char *)(inst) + \
              ((DLiteInstance *)(inst))->meta->_propdimsoffset) + \
   ((DLiteInstance *)(inst))->meta->_propdiminds[n])

/** Expands to size of the `m`th dimension of property `n` --> (size_t) */
#define DLITE_PROP_DIM(inst, n, m) (DLITE_PROP_DIMS(inst, n)[m])

/** Expands to array of property descriptions --> (DLiteProperty *) */
#define DLITE_PROPS_DESCR(inst) (((DLiteInstance *)(inst))->meta->_properties)

/** Expands to pointer to description of property `n` --> (DLiteProperty *) */
#define DLITE_PROP_DESCR(inst, n) \
  (((DLiteInstance *)(inst))->meta->_properties + n)

/** Expands to number of relations --> (size_t) */
#define DLITE_NRELS(inst) (((DLiteInstance *)(inst))->meta->_nrelations)

/** Expands to pointer to array of relations --> (DLiteRelation *) */
#define DLITE_RELS(inst)						\
  ((DLiteRelation *)((char *)(inst) +					\
		     ((DLiteInstance *)(inst))->meta->_reloffset))

/** Expands to pointer to relation `n` --> (DLiteRelation *) */
#define DLITE_REL(inst, n) (DLITE_RELS(inst) + n)

/** Expands to the offset of propoffsets memory section --> (size_t) */
#define DLITE_PROPOFFSETSOFFSET(inst)                                   \
  ((size_t)(((DLiteMeta *)inst)->meta->_propdimindsoffset +             \
            ((DLiteMeta *)inst)->_nproperties * sizeof(size_t)))

/** Expands to the allocated size of a data or metadata instance
    --> (size_t) */
#define DLITE_INSTANCE_SIZE(inst)                                       \
  ((dlite_instance_is_data((DLiteInstance *)inst)) ?                    \
   (inst->meta->_propdimindsoffset) :                                   \
   (inst->meta->_propdimindsoffset +                                    \
    2 * (((DLiteMeta *)inst)->_ndimensions) * sizeof(size_t)))

/** Updates metadata for extended entities.
    `meta`: DLiteMeta
    `type`: the struct to extend
    `firstdim`: name of the first dimension
*/
#define DLITE_UPDATE_EXTENEDE_META(meta, type, firstdim)  \
  do {                                                    \
    meta->_headersize = offsetof(type, firstdim);         \
    dlite_meta_init((DLiteMeta *)meta);                   \
  } while (0)



/** @} */
/* ================================================================= */
/**
 * @name Framework internals and debugging
 */
/* ================================================================= */
/** @{ */

/**
  Prints instance to stdout. Intended for debugging.
 */
void dlite_instance_debug(const DLiteInstance *inst);

/**
  Returns the allocated size of an instance with metadata `meta` and
  dimensions `dims`.  The length of `dims` is given by
  ``meta->_ndimensions``.  Mostly intended for internal use.

  Returns -1 on error.
 */
size_t dlite_instance_size(const DLiteMeta *meta, const size_t *dims);

/**
  Returns a newly allocated NULL-terminated array of string pointers
  with uuids available in the internal storage (istore).

  Mostly intended for internal/debugging use.

*/
char** dlite_istore_get_uuids(int* nuuids);

/** @} */
/* ================================================================= */
/**
 * @name Instance API
 */
/* ================================================================= */
/** @{ */

/**
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
                                     const char *id);

/**
  Like dlite_instance_create() but takes the uri or uuid of the
  metadata as the first argument.

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
  Checks whether an instance with the given `id` exists.

  If `check_storages` is true, the storage plugin path is searched if
  the instance is not found in the in-memory store.  This is
  equivalent to dlite_instance_get(), except that a borrowed
  reference is returned and no error is reported if the instance
  cannot be found.

  Returns a pointer to the instance (borrowed reference) if it exists
  or NULL otherwise.
 */
DLiteInstance *dlite_instance_has(const char *id, bool check_storages);

/**
  Returns a new reference to instance with given `id` or NULL if no such
  instance can be found.

  If the instance exists in the in-memory store it is returned (with
  its refcount increased by one).  Otherwise it is searched for in the
  storage plugin path (initiated from the DLITE_STORAGES environment
  variable).

  It is an error message if the instance cannot be found.
*/
DLiteInstance *dlite_instance_get(const char *id);

/**
  Like dlite_instance_get(), but maps the instance with the given id
  to an instance of `metaid`.  If `metaid` is NULL, it falls back to
  dlite_instance_get().  Returns NULL on error.
 */
DLiteInstance *dlite_instance_get_casted(const char *id, const char *metaid);


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
  A convinient function that loads an instance given an URL of the form

      driver://loc?options#id

  where `loc` corresponds to the `uri` argument of dlite_storage_open().
  If `loc` is not given, the instance is loaded from the metastore using
  `id`.

  Returns the instance or NULL on error.
 */
DLiteInstance *dlite_instance_load_url(const char *url);

/**
  Like dlite_instance_load(), but allows casting the loaded instance
  into an instance of metadata identified by `metaid`.  If `metaid` is
  NULL, no casting is performed.

  Some storages accept that `id` is NULL if the storage only contain
  one instance.  In that case that instance is returned.

  For the cast to be successful requires that the correct mappings
  have been registered.

  Returns NULL on error or if no mapping can be found.
 */
DLiteInstance *dlite_instance_load_casted(const DLiteStorage *s,
                                          const char *id,
                                          const char *metaid);


/**
  Saves instance `inst` to storage `s`.  Returns non-zero on error.
 */
int dlite_instance_save(DLiteStorage *s, const DLiteInstance *inst);


/**
  A convinient function that saves instance `inst` to the storage specified
  by `url`, which should be of the form

      driver://loc?options

  Returns non-zero on error.
 */
int dlite_instance_save_url(const char *url, const DLiteInstance *inst);


/**
  Returns a pointer to instance UUID.
 */
const char *dlite_instance_get_uuid(const DLiteInstance *inst);

/**
  Returns a pointer to instance URI.
 */
const char *dlite_instance_get_uri(const DLiteInstance *inst);

/**
  Returns a pointer to instance IRI.
 */
const char *dlite_instance_get_iri(const DLiteInstance *inst);

/**
  Returns a pointer to the UUID of the instance metadata.
 */
const char *dlite_instance_get_meta_uuid(const DLiteInstance *inst);

/**
  Returns a pointer to the URI of the instance metadata.
 */
const char *dlite_instance_get_meta_uri(const DLiteInstance *inst);


/**
  Returns true if instance has a dimension with the given name.
 */
bool dlite_instance_has_dimension(DLiteInstance *inst, const char *name);

/**
  Returns number of dimensions or -1 on error.
 */
size_t dlite_instance_get_ndimensions(const DLiteInstance *inst);

/**
  Returns number of properties or -1 on error.
 */
size_t dlite_instance_get_nproperties(const DLiteInstance *inst);

/**
  Returns size of dimension `i` or -1 on error.
 */
size_t dlite_instance_get_dimension_size_by_index(const DLiteInstance *inst,
                                                  size_t i);

/**
  Returns a pointer to data corresponding to property with index `i`
  or NULL on error.

  The returned pointer points to the actual data and should not be
  dereferred for arrays.
 */
void *dlite_instance_get_property_by_index(const DLiteInstance *inst, size_t i);

/**
  Sets property `i` to the value pointed to by `ptr`.
  Returns non-zero on error.
*/
int dlite_instance_set_property_by_index(DLiteInstance *inst, size_t i,
                                         const void *ptr);

/**
  Returns number of dimensions of property with index `i` or -1 on error.
 */
int dlite_instance_get_property_ndims_by_index(const DLiteInstance *inst,
                                               size_t i);

/**
  Returns size of dimension `j` in property `i` or -1 on error.
 */
int dlite_instance_get_property_dimsize_by_index(const DLiteInstance *inst,
                                                 size_t i, size_t j);

/**
  Returns a malloc'ed array of dimensions of property `i` or NULL on error.
 */
size_t *dlite_instance_get_property_dims_by_index(const DLiteInstance *inst,
                                                  size_t i);

/**
  Returns size of dimension `i` or -1 on error.
 */
int dlite_instance_get_dimension_size(const DLiteInstance *inst,
                                      const char *name);

/**
  Returns a pointer to data corresponding to `name` or NULL on error.
 */
void *dlite_instance_get_property(const DLiteInstance *inst, const char *name);

/**
  Copies memory pointed to by `ptr` to property `name`.
  Returns non-zero on error.
*/
int dlite_instance_set_property(DLiteInstance *inst, const char *name,
                                const void *ptr);

/**
  Returns true if instance has a property with the given name.
 */
bool dlite_instance_has_property(const DLiteInstance *inst, const char *name);

/**
  Returns number of dimensions of property `name` or -1 on error.
*/
int dlite_instance_get_property_ndims(const DLiteInstance *inst,
                                      const char *name);

/**
  Returns size of dimension `j` of property `name` or NULL on error.
*/
size_t dlite_instance_get_property_dimssize(const DLiteInstance *inst,
                                            const char *name, size_t j);

/**
  Returns non-zero if `inst` is a data instance.
 */
int dlite_instance_is_data(const DLiteInstance *inst);

/**
  Returns non-zero if `inst` is metadata.

  This is simply the inverse of dlite_instance_is_data().
 */
int dlite_instance_is_meta(const DLiteInstance *inst);

/**
  Returns non-zero if `inst` is meta-metadata.

  Meta-metadata contains either a "properties" property (of type
  DLiteProperty) or a "relations" property (of type DLiteRelation) in
  addition to a "dimensions" property (of type DLiteDimension).
 */
int dlite_instance_is_metameta(const DLiteInstance *inst);


/**
  Updates dimension sizes from internal state by calling the getdim()
  method of extended metadata.  Does nothing, if the metadata has no
  getdim() method.

  Returns non-zero on error.
 */
int dlite_instance_sync_to_dimension_sizes(DLiteInstance *inst);

/**
  Updates internal state of extended metadata from instance dimensions
  using setdim().  Does nothing, if the metadata has no setdim().

  Returns non-zero on error.
 */
int dlite_instance_sync_from_dimension_sizes(DLiteInstance *inst);

/**
  Help function that update properties from the saveprop() method of
  extended metadata.  Does nothing, if the metadata has no saveprop() method.

  Returns non-zero on error.
 */
int dlite_instance_sync_to_properties(DLiteInstance *inst);

/**
  Updates internal state of extended metadata from instance properties
  using loadprop().  Does nothing, if the metadata has no loadprop().

  Returns non-zero on error.
 */
int dlite_instance_sync_from_properties(DLiteInstance *inst);


/**
  Updates the size of all dimensions in `inst`.  The new dimension
  sizes are provided in `dims`, which must be of length
  `inst->ndimensions`.  Dimensions corresponding to negative elements
  in `dims` will remain unchanged.

  All properties whos dimension are changed will be reallocated and
  new memory will be zeroed.  The values of properties with two or
  more dimensions, where any but the first dimension is updated,
  should be considered invalidated.

  Returns non-zero on error.
 */
int dlite_instance_set_dimension_sizes(DLiteInstance *inst, const int *dims);

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

  `order` can be 'C' for row-major (C-style) order and 'F' for column-manor
  (Fortran-style) order.

  The returned array object only describes, but does not own the
  underlying array data, which remains owned by the instance.

  Scalars are treated as a one-dimensional array or length one.

  Returns NULL on error.
 */
DLiteArray *
dlite_instance_get_property_array_by_index(const DLiteInstance *inst,
                                           size_t i, int order);

/**
  Returns a new DLiteArray object for property `name` in instance `inst`.

  `order` can be 'C' for row-major (C-style) order and 'F' for column-manor
  (Fortran-style) order.

  The returned array object only describes, but does not own the
  underlying array data, which remains owned by the instance.

  Scalars are treated as a one-dimensional array or length one.

  Returns NULL on error.
 */
DLiteArray *dlite_instance_get_property_array(const DLiteInstance *inst,
                                              const char *name, int order);

/**
  Copy value of property `name` to memory pointed to by `dest`.  It must be
  large enough to hole all the data.  The meaning or `order` is:
    'C':  row-major (C-style) order, no reordering.
    'F':  coloumn-major (Fortran-style) order, transposed order.

  Return non-zero on error.
 */
int dlite_instance_copy_property(const DLiteInstance *inst, const char *name,
                                 int order, void *dest);


/**
  Like dlite_instance_copy_property(), but the property is specified by
  its index `i` instead of name.
 */
int dlite_instance_copy_property_by_index(const DLiteInstance *inst, int i,
                                          int order, void *dest);

/**
  Copies and possible type-cast value of property number `i` to memory
  pointed to by `dest` using `castfun`.  The destination memory is
  described by arguments `type`, `size` `dims` and `strides`.  It must
  be large enough to hole all the data.

  If `castfun` is NULL, it defaults to dlite_type_copy_cast().

  Return non-zero on error.
 */
int dlite_instance_cast_property_by_index(const DLiteInstance *inst,
                                          int i,
                                          DLiteType type,
                                          size_t size,
                                          const size_t *dims,
                                          const int *strides,
                                          void *dest,
                                          DLiteTypeCast castfun);


/**
  Assigns property `name` to memory pointed to by `src`. The meaning
  of `order` is:
    'C':  row-major (C-style) order, no reordering.
    'F':  coloumn-major (Fortran-style) order, transposed order.

  Return non-zero on error.
 */
int dlite_instance_assign_property(const DLiteInstance *inst, const char *name,
                                   int order, const void *src);


/**
  Assigns property `i` by copying and possible type-cast memory
  pointed to by `src` using `castfun`.  The memory pointed to by `src`
  is described by arguments `type`, `size` `dims` and `strides`.

  If `castfun` is NULL, it defaults to dlite_type_copy_cast().

  Return non-zero on error.
 */
int dlite_instance_assign_casted_property_by_index(const DLiteInstance *inst,
                                                   int i,
                                                   DLiteType type,
                                                   size_t size,
                                                   const size_t *dims,
                                                   const int *strides,
                                                   const void *src,
                                                   DLiteTypeCast castfun);


/** @} */
/* ================================================================= */
/**
 * @name Metadata API
 */
/* ================================================================= */
/** @{ */

/**
  Specialised function that returns a new metadata created from the
  given arguments.  It is an instance of DLITE_ENTITY_SCHEMA.
 */
DLiteMeta *
dlite_meta_create(const char *uri, const char *iri,
                  const char *description,
                  size_t ndimensions, const DLiteDimension *dimensions,
                  size_t nproperties, const DLiteProperty *properties);

/**
  Initialises internal data of metadata `meta`.

  Note, even though this function is called internally in
  dlite_instance_create(), it has to be called again after properties
  has been assigned to the metadata.  This because `_npropdims` and
  `__propdiminds` depends on the property dimensions.

  Returns non-zero on error.
 */
int dlite_meta_init(DLiteMeta *meta);

/**
  Increase reference count to meta-metadata.

  Returns the new reference count.
 */
int dlite_meta_incref(DLiteMeta *meta);

/**
  Decrease reference count to meta-metadata.  If the reference count reaches
  zero, the meta-metadata is free'ed.

  Returns the new reference count.
 */
int dlite_meta_decref(DLiteMeta *meta);

/**
  Returns a new reference to metadata with given `id` or NULL if no such
  instance can be found.
*/
DLiteMeta *dlite_meta_get(const char *id);

/**
  Loads metadata identified by `id` from storage `s` and returns a new
  fully initialised meta instance.
*/
DLiteMeta *dlite_meta_load(const DLiteStorage *s, const char *id);

/**
  Like dlite_instance_load_url(), but loads metadata instead.
  Returns the metadata or NULL on error.
 */
DLiteMeta *dlite_meta_load_url(const char *url);

/**
  Saves metadata `meta` to storage `s`.  Returns non-zero on error.
 */
int dlite_meta_save(DLiteStorage *s, const DLiteMeta *meta);

/**
  Saves metadata `meta` to `url`.  Returns non-zero on error.
 */
int dlite_meta_save_url(const char *url, const DLiteMeta *meta);

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


/**
  Returns true if `meta` has a dimension with the given name.
 */
bool dlite_meta_has_dimension(const DLiteMeta *meta, const char *name);


/**
  Returns true if `meta` has a property with the given name.
 */
bool dlite_meta_has_property(const DLiteMeta *meta, const char *name);


/** @} */
/* ================================================================= */
/**
 * @name Dimensions
 */
/* ================================================================= */
/** @{ */

/**
  Returns a newly malloc'ed DLiteDimension or NULL on error.

  The `description` arguments may be NULL.
*/
DLiteDimension *dlite_dimension_create(const char *name,
                                       const char *description);

/**
  Frees a DLiteDimension.
*/
void dlite_dimension_free(DLiteDimension *dim);



/** @} */
/* ================================================================= */
/**
 * @name Properties
 */
/* ================================================================= */
/** @{ */

/**
  Returns a newly malloc'ed DLiteProperty or NULL on error.

  It is created with no dimensions.  Use dlite_property_add_dim() to
  add dimensions to the property.

  The arguments `unit`, `iri` and `description` may be NULL.
*/
DLiteProperty *dlite_property_create(const char *name,
                                     DLiteType type,
                                     size_t size,
                                     const char *unit,
                                     const char *iri,
                                     const char *description);

/**
  Frees a DLiteProperty.
*/
void dlite_property_free(DLiteProperty *prop);


/**
  Add dimension expression `expr` to property.  Returns non-zero on error.
 */
int dlite_property_add_dim(DLiteProperty *prop, const char *expr);


/**
  Writes a string representation of data for property `p` to `dest`.

  The pointer `ptr` should point to the memory where the data is stored.
  The meaning and layout of the data is described by property `p`.
  The actual sizes of the property dimension is provided by `dims`.  Use
  dlite_instance_get_property_dims_by_index() or the DLITE_PROP_DIMS macro
  for accessing `dims`.

  No more than `n` bytes are written to `dest` (incl. the terminating
  NUL).  Arrays will be written with a JSON-like syntax.

  The `width` and `prec` arguments corresponds to the printf() minimum
  field width and precision/length modifier.  If you set them to -1, a
  suitable value will selected according to `type`.  To ignore their
  effect, set `width` to zero or `prec` to -2.

  Returns number of bytes written to `dest`.  If the output is
  truncated because it exceeds `n`, the number of bytes that would
  have been written if `n` was large enough is returned.  On error, a
  negative value is returned.
 */
int dlite_property_print(char *dest, size_t n, const void *ptr,
                         const DLiteProperty *p, const size_t *dims,
                         int width, int prec, DLiteTypeFlag flags);

/**
  Like dlite_type_print(), but prints to allocated buffer.

  Prints to position `pos` in `*dest`, which should point to a buffer
  of size `*n`.  `*dest` is reallocated if needed.

  Returns number or bytes written or a negative number on error.
 */
int dlite_property_aprint(char **dest, size_t *n, size_t pos, const void *ptr,
                          const DLiteProperty *p, const size_t *dims,
                          int width, int prec, DLiteTypeFlag flags);


/**
  Scans property from `src` and write it to memory pointed to by `ptr`.

  The property is described by `p`.

  For arrays, `ptr` should points to the first element and will not be
  not dereferenced.  Evaluated dimension sizes are given by `dims`.

  The `flags` provides some format options.  If zero (default) bools
  and strings are expected to be quoted.

  Returns number of characters consumed from `src` or a negative
  number on error.
 */
int dlite_property_scan(const char *src, void *ptr, const DLiteProperty *p,
                        const size_t *dims, DLiteTypeFlag flags);


/**
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
    - dims: Evaluated shape of property to scan.
    - flags: Format options.  If zero (default) strings are expected to be
        quoted.

  Returns:
    - Number of characters consumed from `src` or a negative number on error.
 */
int dlite_property_jscan(const char *src, const jsmntok_t *item,
                         const char *key, void *ptr, const DLiteProperty *p,
                         const size_t *dims, DLiteTypeFlag flags);


/** @} */
/* ================================================================= */
/**
 * @name MetaModel - a data model for metadata
 *
 *  An interface for easy creation of metadata programically.
 *  This is especially useful in bindings to other languages like Fortran
 *  where code generation is more difficult.
 */
/* ================================================================= */
/** @{ */

/**
  Create and return a new empty metadata model.  `iri` is optional and
  may be NULL.

  Returns NULL on error.
 */
DLiteMetaModel *dlite_metamodel_create(const char *uri,
                                       const char *metaid,
                                       const char *iri);

/**
  Frees metadata model.
 */
void dlite_metamodel_free(DLiteMetaModel *model);

/**
  Sets actual value of dimension `name`, where `name` must correspond to
  a named dimension in the metadata of this model.
 */
int dlite_metamodel_set_dimension_value(DLiteMetaModel *model,
                                        const char *name,
                                        size_t value);

/**
  Adds a data value to `model` corresponding to property `name` of
  the metadata for this model.

  Note that `model` only stores a pointer to `value`.  This means
  that `value` must not be reallocated or free'ed while `model` is in
  use.

  This can e.g. be used to add description.

  Returns non-zero on error.
*/
int dlite_metamodel_add_value(DLiteMetaModel *model, const char *name,
                              const void *value);

/**
  Like dlite_metamodel_add_value(), but if a value exists, it is replaced
  instead of added.

  Returns non-zero on error.
*/
int dlite_metamodel_set_value(DLiteMetaModel *model, const char *name,
                              const void *value);

/**
  Adds a string to `model` corresponding to property `name` of the
  metadata for this model.

  Returns non-zero on error.
*/
int dlite_metamodel_add_string(DLiteMetaModel *model, const char *name,
                               const char *s);

/**
  Like dlite_metamodel_add_string(), but if the string already exists, it
  is replaced instead of added.

  Returns non-zero on error.
*/
int dlite_metamodel_set_string(DLiteMetaModel *model, const char *name,
                               const char *s);

/**
  Adds a dimension to the property named "dimensions" of the metadata
  for `model`.

  The name and description of the new dimension is given by `name` and
  `description`, respectively.  `description` may be NULL.

  Returns non-zero on error.
*/
int dlite_metamodel_add_dimension(DLiteMetaModel *model,
                                      const char *name,
                                  const char *description);

/**
  Adds a new property to the property named "properties" of the metadata
  for `model`.

  Arguments:
    - model: datamodel that the property is added to
    - name: name of new property
    - typename: type of new property, ex. "string80", "int64", "string",...
    - unit: unit of new type. May be NULL
    - iri: iri reference to an ontology. May be NULL
    - description: description of property. May be NULL

  Use dlite_metamodel_add_property_dim() to add dimensions to the
  property.

  Returns non-zero on error.
*/
int dlite_metamodel_add_property(DLiteMetaModel *model,
                                 const char *name,
                                 const char *typename,
                                 const char *unit,
                                 const char *iri,
                                 const char *description);

/**
  Add dimension expression `expr` to property `name` (which must have
  been added with dlite_metamodel_add_property()).

  Returns non-zero on error.
*/
int dlite_metamodel_add_property_dim(DLiteMetaModel *model,
                                  const char *name,
                                     const char *expr);

/**
  If `model` is missing a value described by a property in its
  metadata, return a pointer to the name of the first missing value.

  If all values are assigned, NULL is returned.
 */
const char *dlite_metamodel_missing_value(const DLiteMetaModel *model);

/**
  Returns a pointer to the value, dimension or property added with the
  given name.

  Returns NULL on error.
 */
const void *dlite_metamodel_get_property(const DLiteMetaModel *model,
                                         const char *name);

/**
  Creates and return a new dlite metadata from `model`.
*/
DLiteMeta *dlite_meta_create_from_metamodel(DLiteMetaModel *model);


#endif /* _DLITE_ENTITY_H */
