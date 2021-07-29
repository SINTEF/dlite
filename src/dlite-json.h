/**
  @file
  @brief Provides built-in sopport for JSON in dlite
 */

#ifndef _DLITE_PRINT_H
#define _DLITE_PRINT_H

#include "utils/jstore.h"
#define JSMN_HEADER
#include "utils/jsmn.h"


/** Flags for serialisation */
typedef enum {
  dliteJsonWithUuid=1,    /*!< Whether to include uuid in output */
  dliteJsonMetaAsData=2,  /*!< Whether to write metadata as data */
} DLiteJsonFlag;


/** Iterater struct */
typedef struct _DLiteJsonIter {
  JStoreIter jiter;                    /*!< jstore iterater */
  char metauuid[DLITE_UUID_LENGTH+1];  /*!< UUID of metadata */
  jsmntok_t *tokens;                   /*!< pointer to allocated tokens */
  unsigned int ntokens;                /*!< number of allocated tokens */
} DLiteJsonIter;


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
  Appends json representation of `inst` to json string pointed to by `*s`.

  The input/output string `*s` will be reallocated as needed. `*size` if
  the size of `*s`.

  Returns number or bytes inserted or a negative number on error.
 */
//int dlite_json_append(char **s, size_t *size, const DLiteInstance *inst,
//                      DLiteJsonFlag flags);


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
 * @name JSON store
 */
/** @{ */

/**
  Appends json representation of `inst` to json store `js`.

  Returns non-zero on error.
 */
int dlite_json_add(JStore *js, const DLiteInstance *inst, DLiteJsonFlag flags);

/**
  Initiate iterator `init` from json store `js`.
  If `metaid` is provided, the iterator will only iterate over instances
  of this metadata.

  Returns non-zero on error.
 */
int dlite_json_iter_init(DLiteJsonIter *iter, JStore *js, const char *metaid);

/**
  Return the id of the next instance in the json store or NULL if the
  iterator is exausted.
 */
const char *dlite_json_iter_next(DLiteJsonIter *iter);

/**
  deinitialises iterater.  Return non-zero on error.
*/
int dlite_json_iter_deinit(DLiteJsonIter *iter);





///** Opaque iterator struct */
//typedef struct _DLiteJsonIter DLiteJsonIter;
//
///**
//  Creates and returns a new iterator used by dlite_json_next().
//
//  Arguments
//  - src: input JSON string to search.
//  - length: length of `src`.  If zero or negative, all of `src` will be used.
//  - metaid: limit the search to instances of metadata with this id.
//
//  The source should be a JSON object with keys being instance UUIDs
//  and values being the JSON representation of the individual instances.
//
//  Returns new iterator or NULL on error.
// */
//DLiteJsonIter *dlite_json_iter_init(const char *src, int length,
//                                    const char *metaid);
//
///**
//  Free's iterator created with dlite_json_iter_init().
// */
//void dlite_json_iter_deinit(DLiteJsonIter *iter);
//
///**
//  Search for instances in the JSON document provided to dlite_json_iter_init()
//  and returns a pointer to instance UUIDs.
//
//  `iter` should be an iterator created with dlite_json_iter_init().
//
//  If `length` is given, it is set to the length of the returned identifier.
//
//  Returns a pointer to the next matching UUID or NULL if there are no more
//  matches left.
// */
//const char *dlite_json_next(DLiteJsonIter *iter, int *length);

/** @} */


#endif /*_ DLITE_PRINT_H */
