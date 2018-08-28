#ifndef _DLITE_STORE_H
#define _DLITE_STORE_H

/**
  An in-memory store for instances.
 */

#include "dlite-storage.h"
#include "dlite-entity.h"


typedef struct _DLiteStore DLiteStore;


/**
  Returns a new store.
*/
DLiteStore *dlite_store_create();

/**
  Frees a store.
*/
void dlite_store_free(DLiteStore *store);

/**
  Returns new store loaded from storage `s`.
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
  Removes instance with given id from store.  Returns non-zero on error.
 */
int dlite_store_remove(DLiteStore *store, const char *id);

/**
  Returns a borrowed pointer to instance, or NULL if `id` is not
  in the store.
*/
DLiteInstance *dlite_store_get(const DLiteStore *store, const char *id);


#endif /* _DLITE_STORE_H */
