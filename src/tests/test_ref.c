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


  printf("\n==========================================\n");
  dlite_json_print(inst);

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
