#include "clp2.h"

#include "minunit/minunit.h"


MU_TEST(test_clp2)
{
  mu_assert_int_eq((size_t)0, clp2(0));
  mu_assert_int_eq((size_t)1, clp2(1));
  mu_assert_int_eq((size_t)2, clp2(2));
  mu_assert_int_eq((size_t)4, clp2(3));
  mu_assert_int_eq((size_t)4, clp2(4));
  mu_assert_int_eq((size_t)8, clp2(5));
}


MU_TEST(test_flp2)
{
  mu_assert_int_eq((size_t)0, flp2(0));
  mu_assert_int_eq((size_t)1, flp2(1));
  mu_assert_int_eq((size_t)2, flp2(2));
  mu_assert_int_eq((size_t)2, flp2(3));
  mu_assert_int_eq((size_t)4, flp2(4));
  mu_assert_int_eq((size_t)4, flp2(5));
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
