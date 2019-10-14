#include <stdlib.h>
#include <stdio.h>

#include "minunit/minunit.h"

#include "dlite.h"
#include "dlite-macros.h"


#define HOST "localhost"
#define USER "jesperf_user"
#define DATABASE "jesperf_test"

DLiteStorage *db=NULL;


MU_TEST(test_open_db)
{
  const char *options = "database=" DATABASE ";user=" USER;
  mu_check((db = dlite_storage_open("postgresql", HOST, options)));
  mu_assert_int_eq(1, dlite_storage_is_writable(db));
}


MU_TEST(test_save)
{
  DLiteInstance *meta, *inst;
  size_t dims[] = {2};
  const char *name = "Ada";
  float age = 42.;
  const char *skills[] = {"jumping", "hopping"};
  int n, i;

  mu_check((meta = dlite_instance_load_url("json://Person.json?mode=r")));

  mu_check((inst = dlite_instance_create((DLiteMeta *)meta, dims, "ada")));
  mu_assert_int_eq(0, dlite_instance_set_property(inst, "name", &name));
  mu_assert_int_eq(0, dlite_instance_set_property(inst, "age", &age));
  mu_assert_int_eq(0, dlite_instance_set_property(inst, "skills", skills));

  mu_assert_int_eq(0, dlite_instance_save_url("json://persons.json?mode=w",
                                              inst));
  if (db)
    mu_check((dlite_instance_save(db, inst)));

  printf("\n*** refcount=%d\n", inst->refcount);
  n = inst->refcount;
  mu_assert_int_eq(1, n);
  for (i=0; i<n; i++) dlite_instance_decref(inst);
}


MU_TEST(test_load)
{
  DLiteInstance *inst;
  if (db) {
    mu_check((inst = dlite_instance_load(db, "ada")));
    mu_check((dlite_instance_save_url("json:persons2.json?mode=w", inst)));
  }
}


MU_TEST(test_close_db)
{
  if (db)
    mu_assert_int_eq(0, dlite_storage_close(db));
}




/***********************************************************************/


MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_open_db);
  MU_RUN_TEST(test_save);
  MU_RUN_TEST(test_load);
  MU_RUN_TEST(test_close_db);
}

int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
