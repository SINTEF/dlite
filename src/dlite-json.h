/**
  @file
  @brief Provides built-in support for JSON in dlite

  A set of utility function for serialising and deserialising dlite
  instances to/from JSON.
*/
#ifndef _DLITE_JSON_H
#define _DLITE_JSON_H

#include "utils/jstore.h"
#include "utils/jsmnx.h"


/** Flags for controlling serialisation */
typedef enum {
  dliteJsonSingle=1,    /*!< single-entity format */
  //dliteJsonMulti=2,     /*!< multi-entity format (cannot be combined)
  //                           with dliteJsonSingle) */
  dliteJsonUriKey=4,    /*!< Use uri (if it exists) as json key in multi-
                             entity format. */
  dliteJsonWithUuid=8,  /*!< include uuid in output */
  dliteJsonWithMeta=16, /*!< always include "meta" (even for metadata) */
  dliteJsonArrays=32,   /*!< write metadata dimension and properties as
                             json arrays (old format) */
} DLiteJsonFlag;

/** JSON formats */
typedef enum {
  dliteJsonDataFormat,    /*!< Data format - multiple items */
  dliteJsonMetaFormat     /*!< Metadata format - single item */
} DLiteJsonFormat;


/**
 * @name Serilisation
 */
/** @{ */


/**
  Serialise instance `inst` to `dest`, formatted as JSON.

  No more than `size` bytes are written to `dest` (incl. the
  terminating NUL).

  Returns number of bytes written to `dest`.  If the output is
  truncated because it exceeds `size`, the number of bytes that would
  have been written if `size` was large enough is returned.  On error, a
  negative value is returned.
*/
int dlite_json_sprint(char *dest, size_t size, const DLiteInstance *inst,
                      int indent, DLiteJsonFlag flags);

/**
  Like dlite_sprint(), but prints to allocated buffer.

  Prints to position `pos` in `*dest`, which should point to a buffer
  of size `*size`.  `*dest` is reallocated if needed.

  Returns number or bytes written or a negative number on error.
 */
int dlite_json_asprint(char **dest, size_t *size, size_t pos,
                       const DLiteInstance *inst, int indent,
                       DLiteJsonFlag flags);

/**
  Like dlite_sprint(), but returns allocated buffer with serialised instance.
 */
char *dlite_json_aprint(const DLiteInstance *inst, int indent,
                        DLiteJsonFlag flags);

/**
  Like dlite_sprint(), but prints to stream `fp`.

  Returns number or bytes printed or a negative number on error.
 */
int dlite_json_fprint(FILE *fp, const DLiteInstance *inst, int indent,
                      DLiteJsonFlag flags);

/**
  Prints json representation of `inst` to standard output.

  Returns number or bytes printed or a negative number on error.
*/
int dlite_json_print(const DLiteInstance *inst);

/**
  Like dlite_json_sprint(), but prints the output to file `filename`.

  Returns number or bytes printed or a negative number on error.
 */
int dlite_json_printfile(const char *filename, const DLiteInstance *inst,
                         DLiteJsonFlag flags);

/**
  Appends json representation of `inst` to json string pointed to by `*s`.

  On input, `*s` should be a malloc'ed string representation of a json object.
  It will be reallocated as needed.

  `*size` if the allocated size of `*s`.  It will be updated when `*s`
  is realocated.

  Returns number or bytes inserted or a negative number on error.
 */
int dlite_json_append(char **s, size_t *size, const DLiteInstance *inst,
                      DLiteJsonFlag flags);


/** @} */
/**
 * @name Deserialisation
 */
/** @{ */

/**
  Returns a new instance scanned from `src`.

  `id` is the uri or uuid of the instance to load.  If `src` only
  contain one instance (of the required metadata), `id` may be NULL.

  If `metaid` is not NULL, it should be the URI or UUID of the
  metadata of the returned instance.  It is an error if no such
  instance exists in the source.

  Returns the instance or NULL on error.
 */
DLiteInstance *dlite_json_sscan(const char *src, const char *id,
                                const char *metaid);

/**
  Like dlite_sscan(), but scans instance `id` from stream `fp` instead
  of a string.

  Returns the instance or NULL on error.
 */
DLiteInstance *dlite_json_fscan(FILE *fp, const char *id, const char *metaid);

/**
  Like dlite_json_sscan(), but scans instance `id` from file
  `filename` instead of a string.

  Returns the instance or NULL on error.
*/
DLiteInstance *dlite_json_scanfile(const char *filename, const char *id,
                                   const char *metaid);




/** @} */
/**
 * @name Iterator
 */
/** @{ */

/** Opaque iterator struct */
typedef struct _DLiteJsonIter DLiteJsonIter;

/**
  Creates and returns a new iterator used by dlite_json_next().

  Arguments
  - src: input JSON string to search.
  - length: length of `src`.  If zero or negative, all of `src` will be used.
  - metaid: limit the search to instances of metadata with this id.

  The source should be a JSON object with keys being instance UUIDs
  and values being the JSON representation of the individual instances.

  Returns a new iterator or NULL on error.
 */
DLiteJsonIter *dlite_json_iter_create(const char *src, int length,
                                      const char *metaid);

/**
  Free's iterator created with dlite_json_iter_create().
 */
void dlite_json_iter_free(DLiteJsonIter *iter);

/**
  Search for instances in the JSON document provided to dlite_json_iter_create()
  and returns a pointer to instance UUIDs.

  `iter` should be an iterator created with dlite_json_iter_create().

  If `length` is given, it is set to the length of the returned identifier.

  Returns a pointer to the next matching UUID or NULL if there are no more
  matches left.
 */
const char *dlite_json_next(DLiteJsonIter *iter, int *length);



/** @} */
/**
 * @name JSON store
 */
/** @{ */


/**
  Load content of json string `src` to json store `js`.
  `len` is the length of `src`.

  Returns json format or -1 on error.
 */
DLiteJsonFormat dlite_jstore_loads(JStore *js, const char *src, int len);

/**
  Read content of `filename` to json store `js`.

  Returns json format or -1 on error.
 */
DLiteJsonFormat dlite_jstore_loadf(JStore *js, const char *filename);

/** Opaque iterator struct */
typedef struct _DLiteJStoreIter DLiteJStoreIter;

/**
  Add json representation of `inst` to json store `js`.

  Returns non-zero on error.
 */
int dlite_jstore_add(JStore *js, const DLiteInstance *inst,
                     DLiteJsonFlag flags);

/**
  Removes instance with given id from json store `js`.

  Returns non-zero on error.
 */
int dlite_jstore_remove(JStore *js, const char *id);

/**
  Initiate iterator `init` from json store `js`.
  If `metaid` is provided, the iterator will only iterate over instances
  of this metadata.

  Returns a new iterator or NULL on error.
 */
DLiteJStoreIter *dlite_jstore_iter_create(JStore *js, const char *metaid);

/**
  Deinitialises iterater.

  Return non-zero on error.
*/
int dlite_jstore_iter_free(DLiteJStoreIter *iter);

/**
  Return the id of the next instance in the json store or NULL if the
  iterator is exausted.
 */
const char *dlite_jstore_iter_next(DLiteJStoreIter *iter);





/** @} */


#endif /*_ DLITE_JSON_H */
