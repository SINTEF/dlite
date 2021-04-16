#include <stdlib.h>
#include <string.h>

#include "strutils.h"

#include "minunit/minunit.h"



MU_TEST(test_strquote)
{
  char buf[10];
  int n;

  n = strquote(buf, sizeof(buf), "abc");
  mu_assert_int_eq(5, n);
  mu_assert_string_eq("\"abc\"", buf);

  n = strquote(buf, sizeof(buf), "s=\"a\"");
  mu_assert_int_eq(9, n);
  mu_assert_string_eq("\"s=\\\"a\\\"\"", buf);

  n = strquote(buf, sizeof(buf), "0123456789abcdef");
  mu_assert_int_eq(18, n);
  mu_assert_string_eq("\"01234567", buf);
}


MU_TEST(test_strnquote)
{
  char buf[10];
  int n;

  n = strnquote(buf, sizeof(buf), "0123456789abcdef", -1, 0);
  mu_assert_int_eq(18, n);
  mu_assert_string_eq("\"01234567", buf);

  n = strnquote(buf, sizeof(buf), "0123456789abcdef", 5, 0);
  mu_assert_int_eq(7, n);
  mu_assert_string_eq("\"01234\"", buf);

  n = strnquote(buf, sizeof(buf), "  s=\"a\"", -1, strquoteInitialBlanks);
  mu_assert_int_eq(11, n);
  mu_assert_string_eq("\"  s=\\\"a\\", buf);

  n = strnquote(buf, sizeof(buf), "\"  s=a\"", -1, strquoteNoQuote);
  mu_assert_int_eq(9, n);
  mu_assert_string_eq("\\\"  s=a\\\"", buf);

  n = strnquote(buf, sizeof(buf), "  s=\"a\"", -1, strquoteNoEscape);
  mu_assert_int_eq(9, n);
  mu_assert_string_eq("\"  s=\"a\"\"", buf);
}


MU_TEST(test_strunquote)
{
  char buf[10];
  int n, consumed;

  n = strunquote(buf, sizeof(buf), "\"123\"", &consumed, 0);
  mu_assert_int_eq(3, n);
  mu_assert_int_eq(5, consumed);
  mu_assert_string_eq("123", buf);

  n = strunquote(buf, sizeof(buf), "  \"123\" + 4 ", &consumed, 0);
  mu_assert_int_eq(3, n);
  mu_assert_int_eq(7, consumed);
  mu_assert_string_eq("123", buf);

  n = strunquote(buf, sizeof(buf), "  \"123\" + 4 ", &consumed,
                 strquoteInitialBlanks);
  mu_assert_int_eq(-1, n);

  n = strunquote(buf, sizeof(buf), "\"0123456789abcdef\"", &consumed, 0);
  mu_assert_int_eq(16, n);
  mu_assert_int_eq(18, consumed);
  mu_assert_string_eq("012345678", buf);

  n = strunquote(buf, sizeof(buf), "0123456789abcdef\"", &consumed, 0);
  mu_assert_int_eq(-1, n);

  n = strunquote(buf, sizeof(buf), "0123456789abcdef\"  ", &consumed,
                 strquoteNoQuote);
  mu_assert_int_eq(19, n);
  mu_assert_int_eq(19, consumed);
  mu_assert_string_eq("012345678", buf);

  n = strunquote(buf, sizeof(buf), " \"0123456789abcdef ", NULL, 0);
  mu_assert_int_eq(-2, n);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_strquote);
  MU_RUN_TEST(test_strnquote);
  MU_RUN_TEST(test_strunquote);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
