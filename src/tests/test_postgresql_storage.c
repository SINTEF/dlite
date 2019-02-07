#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "dlite.h"
#include "dlite-macros.h"


MU_TEST(test_open)
{
  char *dbname = "dummy";
  DLiteStorage *s = NULL;

  mu_check((s = dlite_storage_open("postgresql", dbname, NULL)));
  mu_assert_int_eq(0, dlite_storage_close(s));
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_open);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
