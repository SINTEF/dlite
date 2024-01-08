#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "strtob.h"
#include "test_macros.h"

#include "minunit/minunit.h"



MU_TEST(test_strtob)
{
  char *endptr;
  mu_assert_int_eq(1, strtob("1", &endptr));
  mu_assert_string_eq("", endptr);

  mu_assert_int_eq(1, strtob("true", &endptr));
  mu_assert_string_eq("", endptr);

  mu_assert_int_eq(1, strtob(".true.", &endptr));
  mu_assert_string_eq("", endptr);

  mu_assert_int_eq(1, strtob("yes", &endptr));
  mu_assert_string_eq("", endptr);

  mu_assert_int_eq(1, strtob("on", &endptr));
  mu_assert_string_eq("", endptr);

  mu_assert_int_eq(1, strtob(" 1 ", &endptr));
  mu_assert_string_eq(" ", endptr);

  mu_assert_int_eq(1, strtob(" True", &endptr));
  mu_assert_string_eq("", endptr);

  mu_assert_int_eq(1, strtob(".TRUE.  ", &endptr));
  mu_assert_string_eq("  ", endptr);

  mu_assert_int_eq(1, strtob(" ono", &endptr));
  mu_assert_string_eq("o", endptr);

  mu_assert_int_eq(1, strtob(" 111 ", &endptr));
  mu_assert_string_eq("11 ", endptr);

  mu_assert_int_eq(-1, strtob(". ono", &endptr));
  mu_assert_string_eq(" ono", endptr);

  mu_assert_int_eq(-1, strtob("21", &endptr));
  mu_assert_string_eq("1", endptr);

  mu_assert_int_eq(0, strtob("0", &endptr));
  mu_assert_string_eq("", endptr);

  mu_assert_int_eq(0, strtob(" False ", &endptr));
  mu_assert_string_eq(" ", endptr);

  mu_assert_int_eq(0, strtob(".fALSE. ", &endptr));
  mu_assert_string_eq(" ", endptr);

  mu_assert_int_eq(0, strtob("offoff", &endptr));
  mu_assert_string_eq("off", endptr);

  mu_assert_int_eq(0, strtob("no", &endptr));
  mu_assert_string_eq("", endptr);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_strtob);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
