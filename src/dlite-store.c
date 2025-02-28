#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "utils/err.h"
#include "dlite-macros.h"
#include "dlite-store.h"

/* TODO
   - Add read and write locks for tread safety they should be local to
     the store.
 */


/* Item to add in the store */
typedef struct {
  DLiteInstance *inst;  /* pointer to instance */
  int refcount;         /* number of times this instance has been added */
} item_t;

/* New map type */
typedef map_t(item_t) item_map_t;

/* Definition of DLiteStore */
struct _DLiteStore {
  item_map_t map;  /* maps uuid to pointer to instance */
};



/*
  Returns a new store.
*/
DLiteStore *dlite_store_create()
{
  DLiteStore *store;
  if (!(store = calloc(1, sizeof(DLiteStore))))
    return err(dliteMemoryError, "allocation failure"), NULL;
  map_init(&store->map);
  return store;
}

/*
  Frees a store.
*/
void dlite_store_free(DLiteStore *store)
{
  const char *uuid;
  map_iter_t iter = map_iter(&store->map);
  while ((uuid = map_next(&store->map, &iter))) {
    item_t *item = (item_t *)map_get(&store->map, uuid);
    assert(item);
    dlite_instance_decref(item->inst);
  }
  map_deinit(&store->map);
  free(store);
}

/*
  Returns a new store populated with all instances in storage `s`.
*/
DLiteStore *dlite_store_load(DLiteStorage *s)
{
  char **p, **uuids=NULL;
  DLiteStore *store=NULL, *retval=NULL;
  DLiteInstance *inst;
  if (!(uuids = dlite_storage_uuids(s, NULL))) goto fail;
  if (!(store = dlite_store_create())) goto fail;
  for (p=uuids; *p; p++) {
    if (!(inst = dlite_instance_load(s, *p))) goto fail;
    if (!dlite_store_add_new(store, inst)) goto fail;
  }
  retval = store;
 fail:
  if (uuids) dlite_storage_uuids_free(uuids);
  if (!retval && store) dlite_store_free(store);
  return retval;
}

/*
  Saves store to storage.  Returns non-zero on error.
*/
int dlite_store_save(DLiteStorage *s, DLiteStore *store)
{
  int retval=0;
  const char *uuid;
  map_iter_t iter = map_iter(&store->map);
  while ((uuid = map_next(&store->map, &iter))) {
    item_t *item = (item_t *)map_get(&store->map, uuid);
    assert(item);
    retval += (dlite_instance_save(s, item->inst)) ? 1 : 0;
  }
  return retval;
}

/*
  Adds instance to store.  If `steel` is non-zero, the ownership of
  `inst` is taken over by the store.  Returns non-zero on error.
 */
static int add(DLiteStore *store, DLiteInstance *inst, int steel)
{
  item_t *p;
  if ((p = map_get(&store->map, inst->uuid))) {
    p->refcount++;
  } else {
    item_t item;
    item.inst = inst;
    item.refcount = 1;
    if (map_set(&store->map, inst->uuid, item))
      return err(1, "failing adding instance %s to store", inst->uuid);
  }
  if (!steel)
    dlite_instance_incref(inst);
  return 0;
}

/*
  Adds instance to store.  Returns non-zero on error.
 */
int dlite_store_add(DLiteStore *store, DLiteInstance *inst)
{
  return add(store, inst, 0);
}

/*
  Like dlite_store_add(), but steals the reference to `inst`.
  This is useful when `inst` is newly created and not used after the call.
 */
int dlite_store_add_new(DLiteStore *store, DLiteInstance *inst)
{
  return add(store, inst, 1);
}

/*
  Removes instance with given id from store and return it.  Returns
  NULL on error.
 */
DLiteInstance *dlite_store_pop(DLiteStore *store, const char *id)
{
  item_t *item;
  DLiteInstance *inst;
  int uuidver;
  char uuid[DLITE_UUID_LENGTH+1];
  if ((uuidver = dlite_get_uuid(uuid, id)) < 0 || uuidver == UUID_RANDOM)
    FAIL1("id '%s' is neither a valid UUID or a convertable string", id);
  if (!(item = (item_t *)map_get(&store->map, uuid)))
    FAIL1("id '%s' is not in store", id);
  inst = item->inst;
  if (--item->refcount <= 0)
    map_remove(&store->map, uuid);
  return inst;
 fail:
  return NULL;
}

/*
  Like dlite_store_pop(), but removes all occurences of the instance in
  `store`. Returns NULL on error.
 */
DLiteInstance *dlite_store_pop_all(DLiteStore *store, const char *id)
{
  item_t *item;
  DLiteInstance *inst;
  int uuidver;
  char uuid[DLITE_UUID_LENGTH+1];
  if ((uuidver = dlite_get_uuid(uuid, id)) < 0 || uuidver == UUID_RANDOM)
    FAIL1("id '%s' is neither a valid UUID or a convertable string", id);
  if (!(item = (item_t *)map_get(&store->map, uuid)))
    FAIL1("id '%s' is not in store", id);
  inst = item->inst;
  map_remove(&store->map, uuid);
  return inst;
 fail:
  return NULL;
}

/*
  Removes instance with given id from store.  Returns non-zero on error.
 */
int dlite_store_remove(DLiteStore *store, const char *id)
{
  DLiteInstance *inst = dlite_store_pop(store, id);
  if (!inst) return 1;
  dlite_instance_decref(inst);
  return 0;
}
/*
int dlite_store_remove(DLiteStore *store, const char *id)
{
  item_t *item;
  int uuidver;
  char uuid[DLITE_UUID_LENGTH+1];
  if ((uuidver = dlite_get_uuid(uuid, id)) < 0 || uuidver == UUID_RANDOM)
    FAIL1("id '%s' is neither a valid UUID or a convertable string", id);
  if (!(item = (item_t *)map_get(&store->map, uuid))) return 1;

  dlite_instance_decref(item->inst);
  if (--item->refcount <= 0)
    map_remove(&store->map, uuid);
  return 0;
 fail:
  return 1;
}
*/

/*
  Returns a borrowed reference to instance, or NULL if `id` is not
  in the store.
*/
DLiteInstance *dlite_store_get(const DLiteStore *store, const char *id)
{
  item_t *item;
  int uuidver;
  char uuid[DLITE_UUID_LENGTH+1];
  if ((uuidver = dlite_get_uuid(uuid, id)) < 0 || uuidver == UUID_RANDOM)
    FAIL1("id '%s' is neither a valid UUID or a convertable string", id);
  if (!(item = (item_t *)map_get(&((DLiteStore *)store)->map, uuid)))
    FAIL1("id '%s' not in store", id);
  return item->inst;
 fail:
  return NULL;
}


/*
  Initialises iterator `iter`.
 */
DLiteStoreIter dlite_store_iter(const DLiteStore *store)
{
  DLiteStoreIter iter;
  (void)store;  /* to get rid of unused warning (since map_iter() is a
                   macro that discards `store` */
  iter.iter = map_iter(&store->map);
  return iter;
}


/*
  Iterates over all instances in store, using an iterator initialised with
  dlite_store_iter().
 */
const char *dlite_store_next(const DLiteStore *store, DLiteStoreIter *iter)
{
  return map_next((map_void_t *)&store->map, &iter->iter);
}
