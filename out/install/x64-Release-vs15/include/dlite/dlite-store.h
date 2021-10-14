#ifndef _DLITE_STORE_H
#define _DLITE_STORE_H

/**
  @file
  @brief An in-memory store for instances.
 */

#include "utils/map.h"
#include "dlite-storage.h"
#include "dlite-entity.h"


/** Store type. */
typedef struct _DLiteStore DLiteStore;

/** Iteraror type returned by dlite_store_iter(). */
typedef struct {
  map_iter_t iter;
} DLiteStoreIter;



/**
  Returns a new store.
*/
DLiteStore *dlite_store_create();

/**
  Frees a store.
*/
void dlite_store_free(DLiteStore *store);

/**
  Returns a new store populated with all instances in storage `s`.
*/
DLiteStore *dlite_store_load(DLiteStorage *s);

/**
  Saves store to storage.  Returns non-zero on error.
*/
int dlite_store_save(DLiteStorage *s, DLiteStore *store);

/**
  Adds instance to store.  The ownership is retained with the caller.
  Returns non-zero on error.
 */
int dlite_store_add(DLiteStore *store, DLiteInstance *inst);

/**
  Adds instance to store, giving away the ownership to the store.
  Hence, the instance will be free'ed when the store is free'ed.
  Returns non-zero on error.
 */
int dlite_store_add_new(DLiteStore *store, DLiteInstance *inst);

/**
  Removes instance with given id from store and return it.  Returns
  NULL on error.
 */
DLiteInstance *dlite_store_pop(DLiteStore *store, const char *id);

/**
  Like dlite_store_pop(), but removes all occurences of the instance in
  `store`. Returns NULL on error.
 */
DLiteInstance *dlite_store_pop_all(DLiteStore *store, const char *id);

/**
  Removes instance with given id from store.  Returns non-zero on error.
 */
int dlite_store_remove(DLiteStore *store, const char *id);

/**
  Returns a borrowed pointer to instance, or NULL if `id` is not
  in the store.
*/
DLiteInstance *dlite_store_get(const DLiteStore *store, const char *id);

/**
  Returns an initiated iterator for use with dlite_store_next().
 */
DLiteStoreIter dlite_store_iter(const DLiteStore *store);

/**
  Returns the next uuid in `store` using iterator `iter` returned by
  dlite_store_iter().  Returns NULL when there are no more uuid's.

  Example:

        const char *uuid;
        DLiteStore *store = dlite_store_load(storage);
        DLiteStoreIter iter = dlite_store_iter(store);

        while ((uuid = dlite_store_next(store, &iter))) {
           printf("uuid=%s\n", uuid);
        }
 */
const char *dlite_store_next(const DLiteStore *store, DLiteStoreIter *iter);


#endif /* _DLITE_STORE_H */
