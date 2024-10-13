#include "clp2.h"

#include "minunit/minunit.h"


MU_TEST(test_clp2)
{
  mu_assert_int_eq(0, (int)clp2(0));
  mu_assert_int_eq(1, (int)clp2(1));
  mu_assert_int_eq(2, (int)clp2(2));
  mu_assert_int_eq(4, (int)clp2(3));
  mu_assert_int_eq(4, (int)clp2(4));
  mu_assert_int_eq(8, (int)clp2(5));
}


MU_TEST(test_flp2)
{
  mu_assert_int_eq(0, (int)flp2(0));
  mu_assert_int_eq(1, (int)flp2(1));
  mu_assert_int_eq(2, (int)flp2(2));
  mu_assert_int_eq(2, (int)flp2(3));
  mu_assert_int_eq(4, (int)flp2(4));
  mu_assert_int_eq(4, (int)flp2(5));
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_clp2);
  MU_RUN_TEST(test_flp2);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
