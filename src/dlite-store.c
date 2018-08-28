#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "err.h"
#include "map.h"
#include "dlite-macros.h"
#include "dlite-store.h"


/* Definition of DLiteStore */
struct _DLiteStore {
  map_void_t map;  /* maps uuid to pointer to instance */
};

/* Item stored in the map */
/* FIXME - remove this struct and add reference counting to instances
   instead */
typedef struct {
  DLiteInstance *inst;  /* pointer to stored instance */
  int own;              /* flag marking whether we own the instance and
                           should free it when the store is free'ed */
} item_t;


/*
  Returns a new store.
*/
DLiteStore *dlite_store_create()
{
  DLiteStore *store;
  if (!(store = calloc(1, sizeof(DLiteStore))))
    return err(1, "allocation failure"), NULL;
  map_init(&store->map);
  return store;
}

/*
  Frees a store.
*/
void dlite_store_free(DLiteStore *store)
{
  const char *uuid;
  item_t *item;
  map_iter_t iter = map_iter(&store->map);
  while ((uuid = map_next(&store->map, &iter))) {
    // XXX FIXME - call dlite_store_remove()
    item = (item_t *)map_get(&store->map, uuid);
    if (item->own) dlite_instance_decref(item->inst);
  }
  map_deinit(&store->map);
}

/*
  Returns new store loaded from storage `s`.
*/
DLiteStore *dlite_store_load(DLiteStorage *s)
{
  char **p, **uuids=NULL;
  DLiteStore *store=NULL, *retval=NULL;
  DLiteInstance *inst;
  if (!(uuids = dlite_storage_uuids(s))) goto fail;
  if (!(store = dlite_store_create())) goto fail;
  for (p=uuids; *p; p++) {
    if (!(inst = dlite_instance_load(s, *p, NULL))) goto fail;
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
  int retval=1;
  const char *uuid;
  map_iter_t iter = map_iter(&store->map);
  while ((uuid = map_next(&store->map, &iter))) {
    item_t *item = (item_t *)map_get(&store->map, uuid);
    assert(item);
    if (dlite_instance_save(s, item->inst)) goto fail;
  }
  retval = 0;
 fail:
  map_deinit(&store->map);
  return retval;
}

/*
  Adds instance to store.  If `steel` is non-zero, the ownership of
  `inst` is taken over by the store.  Returns non-zero on error.
 */
static int add(DLiteStore *store, DLiteInstance *inst, int steel)
{
  int retval=1;
  item_t *item;
  if (!(item = calloc(1, sizeof(item_t))))
    FAIL("allocation error");
  item->inst = inst;
  item->own = steel;
  if (map_set(&store->map, inst->uuid, item))
    FAIL1("failing inserting instance %s into store", inst->uuid);

  retval = 0;
 fail:
  if (retval && item) free(item);
  return retval;
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
  Removes instance with given id from store.  Returns non-zero on error.
 */
int dlite_store_remove(DLiteStore *store, const char *id)
{
  item_t *item;
  int uuidver, retval=1;
  char uuid[DLITE_UUID_LENGTH+1];
  if ((uuidver = dlite_get_uuid(uuid, id)) != 0 && uuidver != 5)
    FAIL1("id '%s' is neither a valid UUID or a convertable string", id);
  if (!(item = (item_t *)map_get(&store->map, uuid)))
    FAIL1("id '%s' not in store", id);
  if (item->own)
    dlite_instance_decref(item->inst);
  map_remove(&store->map, uuid);

  retval = 0;
 fail:
  return retval;
}

/*
  Returns a borrowed reference to instance, or NULL if `id` is not
  in the store.
*/
/*
DLiteInstance *dlite_store_get(const DLiteStore *store, const char *id)
{
  DLiteInstance *inst;

  return inst;
}
*/
