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


DLiteInstance *inst;



MU_TEST(test_read)
{
  char *filename = STRINGIFY(DLITE_ROOT) "/src/tests/test-read-data.json";
  DLiteStorage *s = NULL;
  int r;

  s = dlite_storage_open("json", filename, "mode=r");
  mu_check(s);

  inst = json_load(s, "dbd9d597-16b4-58f5-b10f-7e49cf85084b");
  mu_check(inst);
  printf("\n--- dbd9d597-16b4-58f5-b10f-7e49cf85084b ---\n");
  dlite_json_print(inst);

  r = dlite_storage_close(s);
  mu_assert_int_eq(0, r);
}


MU_TEST(test_write)
{
  DLiteStorage *s=NULL;
  int r;
  assert(inst);

  s = dlite_storage_open("json", "test-json-write.json", "mode=w");
  mu_check(s);

  r = json_save(s, inst);
  mu_assert_int_eq(0, r);

  r = dlite_storage_close(s);
  mu_assert_int_eq(0, r);

  dlite_instance_decref(inst);
}


MU_TEST(test_iter)
{
  DLiteStorage *s=NULL;
  void *iter;
  char uuid[DLITE_UUID_LENGTH+1];
  int r, n=0;

  s = dlite_storage_open("json", "test-json-write.json", "mode=w");
  mu_check(s);

  iter = json_iter_create(s, NULL);
  while ((r = json_iter_next(iter, uuid)) == 0) {
    printf("\n** uuid: %s\n", uuid);
    DLiteInstance *inst2 = dlite_instance_get(uuid);
    dlite_json_print(inst2);
    dlite_instance_decref(inst2);
    n++;
  }
  mu_assert_int_eq(1, r);
  mu_assert_int_eq(5, n);

  json_iter_free(iter);
}





/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_read);
  MU_RUN_TEST(test_write);
  MU_RUN_TEST(test_iter);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
