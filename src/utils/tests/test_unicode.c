#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "unicode.h"

#include "minunit/minunit.h"



MU_TEST(test_utf8decode)
{
  int n;
  long unicode;

  n = utf8decode("A...", &unicode);  // A
  mu_assert_int_eq(1, n);
  mu_assert_int_eq(0x0041, unicode);

  n = utf8decode("\xc3\xa9...", &unicode);  // e acute
  mu_assert_int_eq(2, n);
  mu_assert_int_eq(0x00e9, unicode);

  n = utf8decode("\xe2\x82\xac...", &unicode);  // eurosign
  mu_assert_int_eq(3, n);
  mu_assert_int_eq(0x20ac, unicode);

  n = utf8decode("\xf0\x90\x8d\x88...", &unicode);  // gothic hwair
  mu_assert_int_eq(4, n);
  mu_assert_int_eq(0x10348, unicode);
}


MU_TEST(test_utf8encode)
{
  int n;
  char buf[5];

  n = utf8encode(0x0041, buf);
  mu_assert_int_eq(1, n);
  mu_assert_string_eq("A", buf);

  n = utf8encode(0x00e9, buf);
  mu_assert_int_eq(2, n);
  mu_assert_string_eq("\xc3\xa9", buf);

  n = utf8encode(0x20ac, buf);
  mu_assert_int_eq(3, n);
  mu_assert_string_eq("\xe2\x82\xac", buf);

  n = utf8encode(0x10348, buf);
  mu_assert_int_eq(4, n);
  mu_assert_string_eq("\xf0\x90\x8d\x88", buf);
}




/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_utf8decode);
  MU_RUN_TEST(test_utf8encode);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
