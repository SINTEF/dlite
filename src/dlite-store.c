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
  DLiteInstance **instp;
  map_iter_t iter = map_iter(&store->map);
  while ((uuid = map_next(&store->map, &iter))) {
    instp = (DLiteInstance **)map_get(&store->map, uuid);
    dlite_instance_decref(*instp);
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
  if (!(uuids = dlite_storage_uuids(s))) goto fail;
  if (!(store = dlite_store_create())) goto fail;
  for (p=uuids; *p; p++) {

    printf("\n");
    printf("uuid=%s\n", *p);
    printf("\n");

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
  int retval=0;
  const char *uuid;
  map_iter_t iter = map_iter(&store->map);
  while ((uuid = map_next(&store->map, &iter))) {
    DLiteInstance *inst;
    DLiteInstance **instp = (DLiteInstance **)map_get(&store->map, uuid);
    assert(instp);
    inst = *instp;
    assert(inst);
    retval += (dlite_instance_save(s, inst)) ? 1 : 0;
  }
  return retval;
}

/*
  Adds instance to store.  If `steel` is non-zero, the ownership of
  `inst` is taken over by the store.  Returns non-zero on error.
 */
static int add(DLiteStore *store, DLiteInstance *inst, int steel)
{
  int retval=1;
  if (map_set(&store->map, inst->uuid, inst))
    FAIL1("failing inserting instance %s into store", inst->uuid);
  if (!steel)
    dlite_instance_incref(inst);
  retval = 0;
 fail:
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
  DLiteInstance *inst, **instp;
  if (!(instp = (DLiteInstance **)map_get(&store->map, id))) return 1;
  inst = *instp;
  assert(inst);
  map_remove(&store->map, inst->uuid);
  dlite_instance_decref(inst);
  return 0;
}


/*
  Returns a borrowed reference to instance, or NULL if `id` is not
  in the store.
*/
DLiteInstance *dlite_store_get(const DLiteStore *store, const char *id)
{
  DLiteInstance **instp;
  int uuidver;
  char uuid[DLITE_UUID_LENGTH+1];
  if ((uuidver = dlite_get_uuid(uuid, id)) != 0 && uuidver != 5)
    FAIL1("id '%s' is neither a valid UUID or a convertable string", id);
  if (!(instp = (DLiteInstance **)map_get((map_void_t *)&store->map, uuid)))
    FAIL1("id '%s' not in store", id);
  return *instp;
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
