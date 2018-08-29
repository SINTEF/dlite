#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "dlite.h"
#include "dlite-store.h"

#include "config.h"


#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s

DLiteEntity *entity = NULL;
DLiteInstance *inst = NULL;
DLiteStore *store = NULL;
char *entity_uri = "http://www.sintef.no/calm/0.1/Chemistry";
char *inst_id = "8411a72c-c7a3-5a6a-b126-1e90b8a55ae2";


/* Counts number of uuid's in store */
int count_uuids(DLiteStore *store) {
  int n=0;
  const char *uuid;
  DLiteStoreIter iter = dlite_store_iter(store);
  while ((uuid = dlite_store_next(store, &iter))) n++;
  return n;
}


MU_TEST(test_entity_load)
{
  DLiteStorage *s;
  char *path = STRINGIFY(DLITE_ROOT) "/tools/tests/Chemistry-0.1.json";

  mu_check((s = dlite_storage_open("json", path, "r")));
  mu_check((entity = dlite_entity_load(s, entity_uri)));
  mu_assert_int_eq(0, dlite_storage_close(s));
}

MU_TEST(test_instance_load)
{
  DLiteStorage *s;
  char *path = STRINGIFY(DLITE_ROOT) "/src/tests/alloys.json";

  mu_check((s = dlite_storage_open("json", path, "r")));
  mu_check((inst = dlite_instance_load(s, inst_id, entity)));
  mu_assert_int_eq(0, dlite_storage_close(s));
}

MU_TEST(test_store_create)
{
  mu_check((store = dlite_store_create()));
}


/* tests add, get, remove */
MU_TEST(test_store)
{
  //mu_assert_int_eq(0, dlite_store_add(store, (DLiteInstance *)entity));
  mu_assert_int_eq(0, dlite_store_add(store, inst));
  //mu_assert_int_eq(2, count_uuids(store));
  //
  //mu_check(dlite_store_remove(store, "invalid_uuid"));
  //mu_assert_int_eq(2, count_uuids(store));
  //
  //mu_assert_int_eq(0, dlite_store_remove(store, entity->uuid));
  mu_assert_int_eq(1, count_uuids(store));
}


MU_TEST(test_save_and_load)
{
  DLiteStorage *s;
  //DLiteStore *store2;
  //DLiteInstance *inst2;

  char *path = "test_store.json";

  mu_check((s = dlite_storage_open("json", path, "w")));
  mu_assert_int_eq(0, dlite_store_save(s, store));
  mu_assert_int_eq(0, dlite_storage_close(s));

  /*
  mu_check((s = dlite_storage_open("json", path, "r")));
  mu_check((store2 = dlite_store_load(s)));
  mu_check(!dlite_storage_close(s));
  */


}





MU_TEST(test_store_free)
{
  dlite_store_free(store);
}

MU_TEST(test_instance_free)
{
  dlite_instance_decref(inst);
}

MU_TEST(test_entity_free)
{
  dlite_entity_decref(entity);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_entity_load);     /* setup */
  MU_RUN_TEST(test_instance_load);
  MU_RUN_TEST(test_store_create);

  MU_RUN_TEST(test_store);
  MU_RUN_TEST(test_save_and_load);

  MU_RUN_TEST(test_store_free);      /* tear down */
  MU_RUN_TEST(test_instance_free);
  MU_RUN_TEST(test_entity_free);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
