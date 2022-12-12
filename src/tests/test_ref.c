/***************************************************************
 * Test dliteRef
 ***************************************************************/
#include <stdlib.h>
#include <stdio.h>

#include "minunit/minunit.h"

#include "dlite.h"
#include "dlite-macros.h"


MU_TEST(test_load)
{
  char *path = STRINGIFY(dlite_SOURCE_DIR) "/src/tests/test_ref.json";
  DLiteInstance *inst = dlite_instance_load_loc("json", path, NULL, "engine1");
  mu_check(inst);

  /* Note, m1 and m2 are owned by inst - we just borrow their references... */
  DLiteInstance **motors = dlite_instance_get_property(inst, "motors");
  DLiteInstance *m1=motors[0], *m2=motors[1];

  printf("\n");
  dlite_json_print((DLiteInstance *)inst->meta);
  printf("---\n");
  dlite_json_print((DLiteInstance *)m1->meta);

  printf("===\n");
  dlite_json_print(inst);

  printf("---\n");
  dlite_json_print(m1);

  printf("---\n");
  dlite_json_print(m2);


  dlite_instance_decref(inst);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_load);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
