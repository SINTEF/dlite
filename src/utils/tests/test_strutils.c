#include <stdlib.h>
#include <string.h>

#include "strutils.h"

#include "minunit/minunit.h"



MU_TEST(test_strnput)
{
  char *buf=NULL;
  size_t size=0;
  int n=0, m;

  m = strnput(&buf, &size, 0, "abc", -1);
  mu_assert_int_eq(3, m);
  mu_assert_string_eq("abc", buf);
  n += m;

  m = strnput(&buf, &size, n, " def", -1);
  mu_assert_int_eq(4, m);
  mu_assert_string_eq("abc def", buf);
  n += m;

  m = strnput(&buf, &size, n, " ghij", 3);
  mu_assert_int_eq(3, m);
  mu_assert_string_eq("abc def gh", buf);
  n += m;

  m = strnput(&buf, &size, 0, " ghij", 3);
  mu_assert_int_eq(3, m);
  mu_assert_string_eq(" gh", buf);
  n = m;

  free(buf);
}


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


MU_TEST(test_strhex_encode)
{
  unsigned char data[4] = {0x61, 0x62, 0x63, 0x64};
  char hex[13];
  int n;

  n = strhex_encode(hex, sizeof(hex), data, sizeof(data));
  mu_assert_int_eq(8, n);
  mu_assert_string_eq("61626364", hex);

  n = strhex_encode(hex, sizeof(hex), data, 2);
  mu_assert_int_eq(4, n);
  mu_assert_string_eq("6162", hex);

  n = strhex_encode(hex, 7, data, sizeof(data));
  mu_assert_int_eq(8, n);
  mu_assert_string_eq("616263", hex);

  n = strhex_encode(hex, 5, data, sizeof(data));
  mu_assert_int_eq(8, n);
  mu_assert_string_eq("6162", hex);

  n = strhex_encode(hex, 6, data, sizeof(data));
  mu_assert_int_eq(8, n);
  mu_assert_string_eq("6162", hex);
}


MU_TEST(test_strhex_decode)
{
  unsigned char data[8];
  int n;

  n = strhex_decode(data, sizeof(data), "00ff", -1);
  mu_assert_int_eq(2, n);
  mu_assert_int_eq(0x00, data[0]);
  mu_assert_int_eq(0xff, data[1]);

  n = strhex_decode(data, sizeof(data), "00FF", 4);
  mu_assert_int_eq(2, n);
  mu_assert_int_eq(0x00, data[0]);
  mu_assert_int_eq(0xff, data[1]);

  n = strhex_decode(data, 2, "aabbccdd", -1);
  mu_assert_int_eq(4, n);
  mu_assert_int_eq(0xaa, data[0]);
  mu_assert_int_eq(0xbb, data[1]);

  n = strhex_decode(data, sizeof(data), "0aff", 2);
  mu_assert_int_eq(1, n);
  mu_assert_int_eq(0x0a, data[0]);

  n = strhex_decode(data, sizeof(data), "00ff", 6);
  mu_assert_int_eq(-1, n);

  n = strhex_decode(data, sizeof(data), "00ff", 3);
  mu_assert_int_eq(-1, n);

  n = strhex_decode(data, sizeof(data), "00ffa", -1);
  mu_assert_int_eq(-1, n);

  n = strhex_decode(data, sizeof(data), "0a-b", -1);
  mu_assert_int_eq(-1, n);
}




/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_strnput);
  MU_RUN_TEST(test_strquote);
  MU_RUN_TEST(test_strnquote);
  MU_RUN_TEST(test_strunquote);
  MU_RUN_TEST(test_strhex_encode);
  MU_RUN_TEST(test_strhex_decode);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
