#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "utils/integers.h"
#include "utils/boolean.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-datamodel.h"

#include "config.h"


/* Forward declarations */
DLiteInstance *json_load(const DLiteStorage *s, const char *id);
int json_save(DLiteStorage *s, const DLiteInstance *inst);
void *json_iter_create(const DLiteStorage *s, const char *metaid);
int json_iter_next(void *iter, char *buf);
void json_iter_free(void *iter);


DLiteInstance *inst, *data3;


MU_TEST(test_get_instance_from_in_memory_store)
{
    char* filename = STRINGIFY(DLITE_ROOT) "/src/tests/test-data.json";
    DLiteStorage* s = NULL;
    DLiteInstance* inst1, * inst0;
    int r, stat;
    printf("\n--- test_get_instance_from_in_memory_store ---\n");

    s = dlite_storage_open("json", filename, "mode=r");
    mu_check(s);

    inst0 = json_load(s, "204b05b2-4c89-43f4-93db-fd1cb70f54ef");
    mu_check(inst0);
    r = dlite_storage_close(s);
    mu_assert_int_eq(0, r);

    inst1 = dlite_instance_get("204b05b2-4c89-43f4-93db-fd1cb70f54ef");
    mu_check(inst1);
    dlite_instance_debug(inst1);
}

MU_TEST(test_remove_last_instance)
{
    char* filename = STRINGIFY(DLITE_ROOT) "/src/tests/test-data.json";
    DLiteStorage* s = NULL;
    DLiteInstance* inst1, * inst0;
    int r, stat;
    printf("\n--- test_remove_last_instance ---\n");

    s = dlite_storage_open("json", filename, "mode=r");
    mu_check(s);

    inst0 = json_load(s, "e076a856-e36e-5335-967e-2f2fd153c17d");
    mu_check(inst0);
    inst1 = json_load(s, "e076a856-e36e-5335-967e-2f2fd153c17d");
    mu_check(inst1);
    dlite_instance_debug(inst1);

    r = dlite_storage_close(s);
    mu_assert_int_eq(0, r);

    stat = dlite_instance_decref(inst1);
    mu_assert_int_eq(1, stat);
    dlite_instance_debug(inst1);

    stat = dlite_instance_decref(inst1);
    mu_assert_int_eq(0, stat);
}

MU_TEST(test_load)
{
  char *filename = STRINGIFY(DLITE_ROOT) "/src/tests/test-read-data.json";
  DLiteStorage *s = NULL;
  int r;
  printf("\n--- test_load: dbd9d597-16b4-58f5-b10f-7e49cf85084b ---\n");

  s = dlite_storage_open("json", filename, "mode=r");
  mu_check(s);

  inst = json_load(s, "dbd9d597-16b4-58f5-b10f-7e49cf85084b");
  mu_check(inst);
  dlite_json_print(inst);

  r = dlite_storage_close(s);
  mu_assert_int_eq(0, r);
}

MU_TEST(test_load2)
{
  int stat;
  char *url = "json://" STRINGIFY(DLITE_ROOT) "/src/tests/test-read-data.json"
    "#dlite/1/test-c";
  printf("\n--- test_load2: %s ---\n", url);

  DLiteInstance *inst2 = dlite_instance_load_url(url);
  mu_check(inst2);
  stat = dlite_instance_decref(inst2);
  mu_assert_int_eq(1, stat);  // store
}

MU_TEST(test_load3)
{
  int stat;
  char *url = "json://" STRINGIFY(DLITE_ROOT) "/src/tests/test-read-data.json"
    "#4cd9ed73-cb8d-5b98-8f3c-db5c916c53a5";
  printf("\n--- test_load3: %s ---\n", url);

  DLiteInstance *inst2 = dlite_instance_load_url(url);
  mu_check(inst2);
  stat = dlite_instance_decref(inst2);
  mu_assert_int_eq(1, stat);  // store
}

MU_TEST(test_load4)
{
  int stat;
  char *url = "json://" STRINGIFY(DLITE_ROOT) "/src/tests/test-read-data.json#dlite/1/A";
  printf("\n--- test_load4: %s ---\n", url);

  DLiteInstance *inst2 = dlite_instance_load_url(url);
  mu_check(inst2);
  stat = dlite_instance_decref(inst2);
  mu_assert_int_eq(2, stat);  // store + inst->meta
}

MU_TEST(test_load_data3)
{
  char *url = "json://" STRINGIFY(DLITE_ROOT) "/src/tests/test-read-data.json#data3";
  printf("\n--- test_load_data3: %s ---\n", url);

  data3 = dlite_instance_load_url(url);
  mu_check(data3);
  dlite_json_print(data3);
}


MU_TEST(test_write)
{
  int stat;
  DLiteStorage *s;
  assert(inst);
  printf("\n--- test_write ---\n");

  s = dlite_storage_open("json", "test-json-write.json", "mode=w");
  mu_check(s);

  stat = json_save(s, inst);
  mu_assert_int_eq(0, stat);

  stat = dlite_storage_close(s);
  mu_assert_int_eq(0, stat);

  stat = dlite_instance_decref(inst);
  mu_assert_int_eq(0, stat);
}


MU_TEST(test_append)
{
  DLiteStorage *s;
  int stat;
  printf("\n--- test_append ---\n");

  s = dlite_storage_open("json", "test-json-write.json", "mode=a");
  mu_check(s);

  stat = json_save(s, data3);
  mu_assert_int_eq(0, stat);

  stat = dlite_storage_close(s);
  mu_assert_int_eq(0, stat);

  stat = dlite_instance_decref(data3);
  mu_assert_int_eq(0, stat);
}


MU_TEST(test_iter)
{
  char *filename = STRINGIFY(DLITE_ROOT) "/src/tests/test-read-data.json";
  DLiteStorage *s=NULL;
  void *iter;
  char uuid[DLITE_UUID_LENGTH+1];
  int r, n=0;
  printf("\n--- test_iter ---\n");

  s = dlite_storage_open("json", filename, "mode=r");
  mu_check(s);

  iter = json_iter_create(s, NULL);
  while ((r = json_iter_next(iter, uuid)) == 0) {
    dlite_errclr();
    printf("\nuuid: %s\n", uuid);
    DLiteInstance *inst2 = dlite_instance_load(s, uuid);
    mu_check(inst2);
    dlite_json_print(inst2);
    //dlite_instance_decref(inst2);
    n++;
  }
  mu_assert_int_eq(1, r);
  mu_assert_int_eq(6, n);

  json_iter_free(iter);

  r = dlite_storage_close(s);
  mu_assert_int_eq(0, r);
}





/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_get_instance_from_in_memory_store);
  MU_RUN_TEST(test_remove_last_instance);
  MU_RUN_TEST(test_load);
  MU_RUN_TEST(test_load2);
  MU_RUN_TEST(test_load3);
  MU_RUN_TEST(test_load4);
  MU_RUN_TEST(test_load_data3);
  MU_RUN_TEST(test_write);
  MU_RUN_TEST(test_append);
  MU_RUN_TEST(test_iter);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
