#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-store.h"

#include "config.h"


DLiteMeta *entity = NULL;
DLiteInstance *inst = NULL;
DLiteStore *store = NULL;
char *entity_uri = "http://sintef.no/calm/0.1/Chemistry";
char *inst_id = "8411a72c-c7a3-5a6a-b126-1e90b8a55ae2";


/* Counts number of uuid's in store */
int count_uuids(DLiteStore *store) {
  int n=0;
  const char *uuid;
  DLiteStoreIter iter = dlite_store_iter(store);
  while ((uuid = dlite_store_next(store, &iter))) n++;
  return n;
}

/* Counts number of adds to store */
/*
int count_adds(DLiteStore *store) {
  int n=0;
  const char *uuid;
  DLiteStoreIter iter = dlite_store_iter(store);
  while ((uuid = dlite_store_next(store, &iter))) {

  }n++;
  return n;
}
*/

/* All tests depends on JSON since it is used to read data */

MU_TEST(test_entity_load)
{
  DLiteStorage *s;
  char *path = STRINGIFY(dlite_SOURCE_DIR) "/tools/tests/Chemistry-0.1.json";
  mu_check((s = dlite_storage_open("json", path, "mode=r")));
  mu_check((entity = dlite_meta_load(s, entity_uri)));
  mu_assert_int_eq(0, dlite_storage_close(s));
  mu_assert_int_eq(2, entity->_refcount);  /* global + inst_store */
}

MU_TEST(test_instance_load)
{
  DLiteStorage *s;
  char *path = STRINGIFY(dlite_SOURCE_DIR) "/src/tests/alloys.json";
  mu_check((s = dlite_storage_open("json", path, "mode=r")));
  mu_check((inst = dlite_instance_load(s, inst_id)));
  mu_assert_int_eq(0, dlite_storage_close(s));

  mu_assert_int_eq(1, inst->_refcount);    /* global */
  mu_assert_int_eq(3, entity->_refcount);  /* global + inst_store + inst */
}

MU_TEST(test_store_create)
{
  mu_check((store = dlite_store_create()));
}


/* tests add, get, remove */
MU_TEST(test_store)
{
  mu_assert_int_eq(0, dlite_store_add(store, (DLiteInstance *)entity));
  mu_assert_int_eq(4, entity->_refcount);  /* global+inst_store+inst+store */
  mu_assert_int_eq(0, dlite_store_add(store, inst));
  mu_assert_int_eq(2, count_uuids(store));
  mu_assert_int_eq(2, inst->_refcount);    /* global+store */
  mu_assert_int_eq(4, entity->_refcount);  /* global+inst_store+inst+store */

  /* removing non-existing uuid */
  mu_check(dlite_store_remove(store, "invalid_uuid"));
  mu_assert_int_eq(2, count_uuids(store));

  /* adding same instance twice */
  mu_assert_int_eq(2, inst->_refcount);
  mu_assert_int_eq(0, dlite_store_add(store, inst));
  mu_assert_int_eq(2, count_uuids(store));
  mu_assert_int_eq(3, inst->_refcount);    /* global+2*store */
  mu_assert_int_eq(4, entity->_refcount);  /* global+inst_store+inst+store */

  /* Removing once a double-added instance should not decrease the count... */
  mu_assert_int_eq(0, dlite_store_remove(store, inst->uuid));
  mu_assert_int_eq(2, count_uuids(store));
  mu_assert_int_eq(2, inst->_refcount);    /* global+store */

  /* ... but the second remove should */
  mu_assert_int_eq(0, dlite_store_remove(store, inst->uuid));
  mu_assert_int_eq(1, count_uuids(store));
  mu_assert_int_eq(1, inst->_refcount);    /* global */

  /* add it again */
  mu_assert_int_eq(0, dlite_store_add(store, inst));
  mu_assert_int_eq(2, count_uuids(store));
  mu_assert_int_eq(2, inst->_refcount);    /* global+store */

  /* remove the entity */
  mu_assert_int_eq(0, dlite_store_remove(store, entity->uuid));
  mu_assert_int_eq(1, count_uuids(store));
  mu_assert_int_eq(3, entity->_refcount);  /* global+inst_store+inst */
}


MU_TEST(test_save_and_load)
{
  DLiteStorage *s;
  char *path = "test_store.json";
  mu_check((s = dlite_storage_open("json", path, "mode=w")));
  mu_assert_int_eq(0, dlite_store_save(s, store));
  mu_assert_int_eq(0, dlite_storage_close(s));

  mu_assert_int_eq(2, inst->_refcount);    /* global+store */
  mu_assert_int_eq(3, entity->_refcount);  /* global+inst_store+inst */
}


MU_TEST(test_store_free)
{
  mu_assert_int_eq(2, inst->_refcount);    /* global + store */
  dlite_store_free(store);
  mu_assert_int_eq(1, inst->_refcount);    /* global */
}

MU_TEST(test_instance_free)
{
  mu_assert_int_eq(1, inst->_refcount);    /* global */
  mu_assert_int_eq(3, entity->_refcount);  /* global + inst_store + inst */

  dlite_instance_decref(inst);
  mu_assert_int_eq(2, entity->_refcount);  /* global + inst_store */
}

MU_TEST(test_entity_free)
{
  dlite_meta_decref(entity);
  mu_assert_int_eq(1, entity->_refcount);  /* global */

  dlite_meta_decref(entity);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_entity_load);     /* setup */
  MU_RUN_TEST(test_instance_load);
  MU_RUN_TEST(test_store_create);

  MU_RUN_TEST(test_store);
  MU_RUN_TEST(test_save_and_load);

  MU_RUN_TEST(test_store_free);
  MU_RUN_TEST(test_instance_free);   /* tear down */
  MU_RUN_TEST(test_entity_free);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
