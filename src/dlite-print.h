/* A set of utility function for serialising and deserialising dlite
   instances to/from JSON.

   This file provides built-in support for JSON to DLite.  This will not
   replace
 */

#ifndef _DLITE_PRINT_H
#define _DLITE_PRINT_H


typedef enum {
  dliteJsonUuid=1,        /*!< Whether to include uuid */
  dliteJsonMetaAsData=2,  /*!< Whether to print entities simplified */
} DLiteJsonFlag;

/**
  Serialise instance `inst` to `dest`, formatted as JSON.

  No more than `size` bytes are written to `dest` (incl. the
  terminating NUL).

  Returns number of bytes written to `dest`.  If the output is
  truncated because it exceeds `size`, the number of bytes that would
  have been written if `size` was large enough is returned.  On error, a
  negative value is returned.
*/
int dlite_json_sprint(char *dest, size_t size, DLiteInstance *inst,
                      int indent, DLiteJsonFlag flags);

/**
  Like dlite_sprint(), but prints to allocated buffer.

  Prints to position `pos` in `*dest`, which should point to a buffer
  of size `*size`.  `*dest` is reallocated if needed.

  Returns number or bytes written or a negative number on error.
 */
int dlite_json_asprint(char **dest, size_t *size, size_t pos,
                       DLiteInstance *inst, int indent, DLiteJsonFlag flags);


/**
  Like dlite_sprint(), but returns allocated buffer with serialised instance.
 */
char *dlite_json_aprint(DLiteInstance *inst, int indent, DLiteJsonFlag flags);


/**
  Like dlite_sprint(), but prints to stream `fp`.

  Returns number or bytes printed or a negative number on error.
 */
int dlite_json_fprint(FILE *fp, DLiteInstance *inst, int indent,
                      DLiteJsonFlag flags);



/**
  Returns a new instance scanned from `src`.

  `id` is the uri or uuid of the instance to load.  If the string only
  contain one instance, `id` may be NULL.

  Returns the instance or NULL on error.
 */
DLiteInstance *dlite_json_sscan(const char *src, const char *id);

/**
  Like dlite_sscan(), but scans instance `id` from stream `fp` instead
  of a string.

  Returns the instance or NULL on error.
 */
DLiteInstance *dlite_json_fscan(FILE *fp, const char *id);



/** Opaque iterator struct */
typedef struct _DLiteJsonIter DLiteJsonIter;

/**
  Creates and returns a new iterator used by dlite_json_next().

  Arguments
  - src: input JSON string to search.
  - length: length of `src`.  If zero or negative, all of `src` will be used.
  - metaid: limit the search to instances of metadata with this id.

  Returns new iterator or NULL on error.
 */
DLiteJsonIter *dlite_json_iter_init(const char *src, int length,
                                    const char *metaid);

/**
  Free's iterator created with dlite_json_iter_init().
 */
void dlite_json_iter_deinit(DLiteJsonIter *iter);

/**
  Search for instances in the JSON document provided to dlite_json_iter_init()
  and returns a pointer to instance UUIDs.

  Returns a pointer to the next matching UUID or NULL if there are no more
  matches left.
 */
const char *dlite_json_next(DLiteJsonIter *iter, int *length);



#endif /*_ DLITE_PRINT_H */
