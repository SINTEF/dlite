#include <stdlib.h>

#include "minunit/minunit.h"
#include "dlite.h"


MU_TEST(test_errcode)
{
  mu_assert_int_eq(dliteIndexError, dlite_errcode("DLiteIndexError"));
  mu_assert_int_eq(dliteIndexError, dlite_errcode("DLiteIndex..."));
  mu_assert_int_eq(dliteUnknownError, dlite_errcode("another error..."));
}



/***********************************************************************/


MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_errcode);
}

int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
