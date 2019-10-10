#include <stdlib.h>
#include <stdio.h>

#include "minunit/minunit.h"

#include "dlite.h"
#include "dlite-macros.h"

DLiteStorage *s=NULL;


MU_TEST(test_open)
{
  mu_check((s = dlite_storage_open("yaml", "test.yaml", "mode=w")));
  mu_assert_int_eq(1, dlite_storage_is_writable(s));
}

MU_TEST(test_save)
{
  DLiteInstance *inst = dlite_instance_get(DLITE_ENTITY_SCHEMA);
  dlite_instance_save(s, inst);
  dlite_instance_decref(inst);
}


MU_TEST(test_close)
{
  mu_assert_int_eq(0, dlite_storage_close(s));
}




/***********************************************************************/


MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_open);   /* setup */

  MU_RUN_TEST(test_save);

  MU_RUN_TEST(test_close);  /* teardown */
}

int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
