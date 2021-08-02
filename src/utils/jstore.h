/* jstore.h -- simple JSON storage
 *
 * Copyright (C) 2021 SINTEF
 *
 * Distributed under terms of the MIT license.
 */

/**
  @file
  @brief Simple JSON storage

  This library maintains a simple JSON storage, whos root is a JSON object.
  The keys identify storage items and the values are valid JSON string
  representing the item values.

  Items can be added, updated and removed from the storage.  Iteration
  over all items in the storage is also supported.
*/
#ifndef _JSTORE_H
#define _JSTORE_H

#include <stdlib.h>
#include "map.h"
#include "jsmnx.h"

//#define JSMN_HEADER
//#define JSMN_STRICT
//#define JSMN_PARENT_LINKS
//#include "jsmn.h"


/** JStore object */
typedef struct _JStore JStore;

/** JStore iterator object */
typedef struct _JStoreIter {
  JStore *js;
  map_iter_t miter;
} JStoreIter;


/** Create a new JSON store and return it.  Returns NULL on error. */
JStore *jstore_open(void);

/** Close JSON store.  Returns non-zero on error. */
int jstore_close(JStore *js);

/** Add JSON value to store with given key.
    If key already exists, it is replaced.
    Returns non-zero on error. */
int jstore_add(JStore *js, const char *key, const char *value);

/** Add JSON value to store with given key.
    The lengths of the key and value are provided by `klen` and `vlen`,
    respectively.  If the strings are NUL-terminated, the corresponding
    length may be set to zero.
    If key already exists, it is replaced.
    Returns non-zero on error. */
int jstore_addn(JStore *js, const char *key, size_t klen,
                const char *value, size_t vlen);

/** Add JSON value to store with given key.
    The store "steels" the ownership of the memory pointed to by `value`.
    If key already exists, it is replaced.
    Returns non-zero on error. */
int jstore_addstolen(JStore *js, const char *key, const char *value);

/** Returns JSON value for given key or NULL if the key isn't in the store.
    This function can also be used to check if a key exists in the store. */
const char *jstore_get(JStore *js, const char *key);

/** Removes item corresponding to given key from JSON store. */
int jstore_remove(JStore *js, const char *key);

/** Update JSON store with values from `other`. Return non-zero on error. */
int jstore_update(JStore *js, JStore *other);

/** Update JSON store with values from JSMN token `tok`. The JSMN token must
    be an object. Return non-zero on error. */
int jstore_update_from_jsmn(JStore *js, const char *src, jsmntok_t *tok);

/** Update JSON store with values from file `filename`.
   Return non-zero on error. */
int jstore_update_from_file(JStore *js, const char *filename);

/** Update `filename` from JSON store.
    The file is first read and then rewritten while the store is unchanged.
    Return non-zero on error. */
int jstore_update_file(JStore *js, const char *filename);

/** Returns a malloc()'ed JSON string with the content of the store or
    NULL on error. */
char *jstore_to_string(JStore *js);

/** Writes JSON store to file.  If `filename` exists, it is overwritten.
    Returns non-zero on error. */
int jstore_to_file(JStore *js, const char *filename);


/** Initialise iterator.  Return non-zero on error. */
int jstore_iter_init(JStore *js, JStoreIter *iter);

/** Returns the key to the next item in or NULL if there are no more items
    in the store.  Use jstore_get() to get the corresponding value. */
const char *jstore_iter_next(JStoreIter *iter);

/** Deinitialise the iterator.  Returns non-zero on error.
    In the current implementation, this function does nothing. */
int jstore_iter_deinit(JStoreIter *iter);


#endif  /* _JSTORE_H */
