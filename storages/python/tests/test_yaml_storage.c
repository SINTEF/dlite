#include <stdlib.h>
#include <stdio.h>

#include "minunit/minunit.h"

#include "dlite.h"
#include "dlite-macros.h"


MU_TEST(test_save)
{
  DLiteStorage *s;
  DLiteInstance *inst;
  mu_check((s = dlite_storage_open("yaml", "test2.yaml", "mode=w")));
  mu_assert_int_eq(1, dlite_storage_is_writable(s));

  inst = dlite_instance_get(DLITE_ENTITY_SCHEMA);
  mu_check(inst);
  mu_assert_int_eq(0, dlite_instance_save(s, inst));
  dlite_instance_decref(inst);

  mu_assert_int_eq(0, dlite_storage_close(s));
}

MU_TEST(test_load)
{
  DLiteStorage *s;
  DLiteInstance *inst;
  mu_check((s = dlite_storage_open("yaml", "test2.yaml", "mode=r")));
  mu_assert_int_eq(0, dlite_storage_is_writable(s));

  inst = dlite_instance_load(s, DLITE_ENTITY_SCHEMA);
  mu_check(inst);
  mu_assert_string_eq(DLITE_ENTITY_SCHEMA, inst->uri);
  dlite_instance_decref(inst);

  mu_assert_int_eq(0, dlite_storage_close(s));
}





/***********************************************************************/


MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_save);
  MU_RUN_TEST(test_load);
}

int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
