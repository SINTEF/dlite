#include <stdlib.h>
#include <string.h>

#include "strutils.h"

#include "minunit/minunit.h"



MU_TEST(test_strsetc)
{
  char buf[5], *src;

  strsetc(buf, sizeof(buf), '{');
  mu_assert_string_eq("{", buf);

  src = "Å";
  buf[1] = 'X';
  strsetc(buf, sizeof(buf), src[0]);
  mu_assert_int_eq(0xc3, (unsigned char)buf[0]);
  mu_assert_int_eq(0, buf[1]);
  buf[1] = 'X';
  strsetc(buf, sizeof(buf), src[1]);
  mu_assert_int_eq(0x85, (unsigned char)buf[0]);
  mu_assert_int_eq(0, buf[1]);

  src = "€";
  buf[1] = 'X';
  strsetc(buf, sizeof(buf), src[0]);
  mu_assert_int_eq(0xe2, (unsigned char)buf[0]);
  mu_assert_int_eq(0, buf[1]);
  buf[1] = 'X';
  strsetc(buf, sizeof(buf), src[1]);
  mu_assert_int_eq(0x82, (unsigned char)buf[0]);
  mu_assert_int_eq(0, buf[1]);
  buf[1] = 'X';
  strsetc(buf, sizeof(buf), src[2]);
  mu_assert_int_eq(0xac, (unsigned char)buf[0]);
  mu_assert_int_eq(0, buf[1]);

  src = "Å";
  strsetc(buf, 2, src[0]);
  mu_assert_int_eq(0, buf[0]);

  strsetc(buf, 2, 0xc385);
  mu_assert_int_eq(0, buf[0]);

  strsetc(buf, 3, 0xc385);
  mu_assert_int_eq(0xc3, (unsigned char)buf[0]);

  strsetc(buf, 3, 0xe282ac);
  mu_assert_int_eq(0, buf[0]);

  strsetc(buf, 4, 0xe282ac);
  mu_assert_int_eq(0xe2, (unsigned char)buf[0]);
}


MU_TEST(test_strsets)
{
  char buf[5];
  int n;

  n = strsets(buf, sizeof(buf), "abcdef");
  mu_assert_int_eq(6, n);
  mu_assert_string_eq("abcd", buf);

  n = strsets(buf, sizeof(buf), "a=Å");
  mu_assert_int_eq(4, n);
  mu_assert_string_eq("a=Å", buf);

  n = strsets(buf, 4, "a=Å");
  mu_assert_int_eq(4, n);
  mu_assert_string_eq("a=", buf);
}


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


MU_TEST(test_strnput_escape)
{
  char *buf=NULL;
  size_t size=0;
  int n=0, m;

  m = strnput_escape(&buf, &size, 0, "1 Å", -1, strcatSpace, "%");
  mu_assert_int_eq(8, m);
  mu_assert_string_eq("1 %C3%85", buf);
  n += m;

  m = strnput_escape(&buf, &size, 0, "1 Å", -1, strcatSpace, "\\x");
  mu_assert_int_eq(10, m);
  mu_assert_string_eq("1 \\xC3\\x85", buf);
  n += m;

  m = strnput_escape(&buf, &size, 0, "1 Å", -1, strcatSubDelims, "%");
  mu_assert_int_eq(10, m);
  mu_assert_string_eq("1%20%C3%85", buf);
  n += m;

  m = strnput_escape(&buf, &size, 0, "2 €", -1, strcatSpace, "%");
  mu_assert_int_eq(11, m);
  mu_assert_string_eq("2 %E2%82%AC", buf);
  n += m;

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


MU_TEST(test_strput_unquote)
{
  char *buf=NULL;
  size_t size=0;
  int n, consumed;

  n = strnput_unquote(&buf, &size, 0, "\"123\"", 4, &consumed, 0);
  mu_assert_int_eq(3, n);
  mu_assert_int_eq(4, consumed);
  mu_assert_int_eq((size_t)4, size);
  mu_assert_string_eq("123", buf);

  n = strnput_unquote(&buf, &size, 2, "\"abc\"", 4, &consumed, 0);
  mu_assert_int_eq(3, n);
  mu_assert_int_eq(4, consumed);
  mu_assert_int_eq((size_t)6, size);
  mu_assert_string_eq("12abc", buf);

  n = strnput_unquote(&buf, &size, 0, "  \"123\" + 4 ", -1, &consumed, 0);
  mu_assert_int_eq(3, n);
  mu_assert_int_eq(7, consumed);
  mu_assert_int_eq((size_t)6, size);
  mu_assert_string_eq("123", buf);

  free(buf);
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


MU_TEST(test_strcategory)
{
  mu_assert_int_eq(strcatUpper, strcategory('A'));
  mu_assert_int_eq(strcatUpper, strcategory('Z'));
  mu_assert_int_eq(strcatLower, strcategory('a'));
  mu_assert_int_eq(strcatLower, strcategory('z'));
  mu_assert_int_eq(strcatDigit, strcategory('0'));
  mu_assert_int_eq(strcatDigit, strcategory('9'));
  mu_assert_int_eq(strcatUnreserved, strcategory('-'));
  mu_assert_int_eq(strcatUnreserved, strcategory('.'));
  mu_assert_int_eq(strcatUnreserved, strcategory('_'));
  mu_assert_int_eq(strcatUnreserved, strcategory('~'));
  mu_assert_int_eq(strcatGenDelims, strcategory(':'));
  mu_assert_int_eq(strcatGenDelims, strcategory('/'));
  mu_assert_int_eq(strcatGenDelims, strcategory('?'));
  mu_assert_int_eq(strcatGenDelims, strcategory('#'));
  mu_assert_int_eq(strcatGenDelims, strcategory('['));
  mu_assert_int_eq(strcatGenDelims, strcategory(']'));
  mu_assert_int_eq(strcatGenDelims, strcategory('@'));
  mu_assert_int_eq(strcatSubDelims, strcategory('!'));
  mu_assert_int_eq(strcatSubDelims, strcategory('$'));
  mu_assert_int_eq(strcatSubDelims, strcategory('&'));
  mu_assert_int_eq(strcatSubDelims, strcategory('\''));
  mu_assert_int_eq(strcatSubDelims, strcategory('('));
  mu_assert_int_eq(strcatSubDelims, strcategory(')'));
  mu_assert_int_eq(strcatSubDelims, strcategory('*'));
  mu_assert_int_eq(strcatSubDelims, strcategory('+'));
  mu_assert_int_eq(strcatSubDelims, strcategory(','));
  mu_assert_int_eq(strcatSubDelims, strcategory(';'));
  mu_assert_int_eq(strcatSubDelims, strcategory('='));
  mu_assert_int_eq(strcatPercent, strcategory('%'));
  mu_assert_int_eq(strcatNul, strcategory('\0'));
  mu_assert_int_eq(strcatCExtra, strcategory('"'));
  mu_assert_int_eq(strcatOther, strcategory('`'));
  mu_assert_int_eq(strcatCExtra, strcategory('<'));
  mu_assert_int_eq(strcatCExtra, strcategory('>'));
  mu_assert_int_eq(strcatOther, strcategory('\xf8'));  // ø
}


MU_TEST(test_strcatspn)
{
  char *s1 = "ABZabz019-.~!=:/%<>";
  char *s2 = "<>%/:=!~.-910zbaZBA";

  mu_assert_int_eq(3,  strcatspn(s1, strcatUpper));
  mu_assert_int_eq(0,  strcatspn(s1, strcatLower));
  mu_assert_int_eq(0,  strcatspn(s2, strcatUpper));
  mu_assert_int_eq(2,  strcatspn(s2, strcatCExtra));
  mu_assert_int_eq(0,  strcatspn(s2, strcatOther));

  mu_assert_int_eq(0,  strcatcspn(s1, strcatUpper));
  mu_assert_int_eq(3,  strcatcspn(s1, strcatLower));
  mu_assert_int_eq(16, strcatcspn(s2, strcatUpper));
  mu_assert_int_eq(0,  strcatcspn(s2, strcatCExtra));
  mu_assert_int_eq(19, strcatcspn(s2, strcatOther));

  mu_assert_int_eq(3,  strcatjspn(s1, strcatUpper));
  mu_assert_int_eq(6,  strcatjspn(s1, strcatLower));
  mu_assert_int_eq(9,  strcatjspn(s1, strcatDigit));
  mu_assert_int_eq(12, strcatjspn(s1, strcatUnreserved));
  mu_assert_int_eq(14, strcatjspn(s1, strcatSubDelims));
  mu_assert_int_eq(16, strcatjspn(s1, strcatGenDelims));
  mu_assert_int_eq(16, strcatjspn(s1, strcatReserved));
  mu_assert_int_eq(17, strcatjspn(s1, strcatPercent));
  mu_assert_int_eq(19, strcatjspn(s1, strcatOther));

  mu_assert_int_eq(16, strcatcjspn(s2, strcatUpper));
  mu_assert_int_eq(13, strcatcjspn(s2, strcatLower));
  mu_assert_int_eq(10, strcatcjspn(s2, strcatDigit));
  mu_assert_int_eq(7,  strcatcjspn(s2, strcatUnreserved));
  mu_assert_int_eq(5,  strcatcjspn(s2, strcatSubDelims));
  mu_assert_int_eq(3,  strcatcjspn(s2, strcatGenDelims));
  mu_assert_int_eq(3,  strcatcjspn(s2, strcatReserved));
  mu_assert_int_eq(2,  strcatcjspn(s2, strcatPercent));
  mu_assert_int_eq(0,  strcatcjspn(s2, strcatOther));
}


MU_TEST(test_strlst)
{
  char **strlst=NULL;
  size_t n=0;

  mu_assert_int_eq((size_t)0, strlst_count(strlst));

  strlst = strlst_append(strlst, &n, "first");
  mu_assert_int_eq((size_t)1, strlst_count(strlst));

  strlst = strlst_insert(strlst, &n, "second", 1);
  mu_assert_int_eq((size_t)2, strlst_count(strlst));

  strlst = strlst_insert(strlst, &n, "insert1", 1);
  mu_assert_int_eq((size_t)3, strlst_count(strlst));

  strlst = strlst_insert(strlst, &n, "insert2", -1);
  mu_assert_int_eq((size_t)4, strlst_count(strlst));

  mu_assert_string_eq("first",   strlst[0]);
  mu_assert_string_eq("insert1", strlst[1]);
  mu_assert_string_eq("insert2", strlst[2]);
  mu_assert_string_eq("second",  strlst[3]);
  mu_assert_string_eq(NULL,      strlst[4]);

  mu_assert_string_eq(strlst[0], strlst_get(strlst, 0));
  mu_assert_string_eq(strlst[3], strlst_get(strlst, 3));
  mu_assert_string_eq(strlst[3], strlst_get(strlst, -1));
  mu_assert_string_eq(NULL,      strlst_get(strlst, 4));
  mu_assert_string_eq(strlst[0], strlst_get(strlst, -4));
  mu_assert_string_eq(NULL,      strlst_get(strlst, -5));

  char *s = strlst_pop(strlst, -2);
  mu_assert_string_eq("insert2", s);
  mu_assert_int_eq((size_t)3, strlst_count(strlst));
  free(s);

  mu_assert_string_eq(NULL, strlst_pop(strlst, -4));
  mu_assert_int_eq((size_t)3, strlst_count(strlst));

  mu_assert_int_eq(0, strlst_remove(strlst, -2));
  mu_assert_int_eq((size_t)2, strlst_count(strlst));

  mu_check(strlst_remove(strlst, 3));

  mu_assert_string_eq("first",   strlst[0]);
  mu_assert_string_eq("second",  strlst[1]);
  mu_assert_string_eq(NULL,      strlst[2]);

  strlst_free(strlst);
}


MU_TEST(test_natoi)
{
  mu_assert_int_eq(1,  natoi("1", 10));
  mu_assert_int_eq(2,  natoi("2 ", 10));
  mu_assert_int_eq(3,  natoi(" 3", 10));
  mu_assert_int_eq(1,  natoi("+1", 10));
  mu_assert_int_eq(-2, natoi("-2", 10));
  mu_assert_int_eq(0,  natoi("*2", 10));
  mu_assert_int_eq(12, natoi("012345", 3));
  mu_assert_int_eq(0,  natoi("012345", 0));
  mu_assert_int_eq(0,  natoi("012345", -3));
}


MU_TEST(test_strchk_semver)
{
  mu_assert_int_eq(5,  strchk_semver("1.0.0"));
  mu_assert_int_eq(-1, strchk_semver(" 1.0.0"));
  mu_assert_int_eq(-1, strchk_semver("1.0.0 "));
  mu_assert_int_eq(-1, strchk_semver("1"));
  mu_assert_int_eq(-1, strchk_semver("1."));
  mu_assert_int_eq(-1, strchk_semver("1.0"));
  mu_assert_int_eq(-1, strchk_semver("1.0."));
  mu_assert_int_eq(-1, strchk_semver("v1.0.0"));
  mu_assert_int_eq(-1, strchk_semver("1.0.0-"));
  mu_assert_int_eq(-1, strchk_semver("1.0.0x"));
  mu_assert_int_eq(-1, strchk_semver("1.0.0 "));
  mu_assert_int_eq(11, strchk_semver("5.12.17-rc1"));
  mu_assert_int_eq(-1, strchk_semver("5.12.17-rc1 "));
  mu_assert_int_eq(-1, strchk_semver("5.12.17-rc1."));
  mu_assert_int_eq(13, strchk_semver("5.12.17-rc1.x"));
  mu_assert_int_eq(-1, strchk_semver("5.12.17-rc1..x"));
  mu_assert_int_eq(11, strchk_semver("5.12.17+001"));
  mu_assert_int_eq(15, strchk_semver("5.12.17-rc1+001"));
  mu_assert_int_eq(-1, strchk_semver("5.12.17-rc1+001 "));
}


MU_TEST(test_strnchk_semver)
{
  mu_assert_int_eq(5,  strnchk_semver("1.0.0", 10));
  mu_assert_int_eq(5,  strnchk_semver("1.0.0", 10));
  mu_assert_int_eq(5,  strnchk_semver("1.0.0", 5));
  mu_assert_int_eq(-1, strnchk_semver("1.0.0", 4));
  mu_assert_int_eq(5,  strnchk_semver("1.0.0 ", 5));
  mu_assert_int_eq(-1, strnchk_semver("1.0.0 ", 6));
  mu_assert_int_eq(-1, strnchk_semver(" 1.0.0", 6));
  mu_assert_int_eq(15, strnchk_semver("5.12.17-rc1+001", 20));
  mu_assert_int_eq(6,  strnchk_semver("5.12.17-rc1+001 ", 6));
  mu_assert_int_eq(7,  strnchk_semver("5.12.17-rc1+001 ", 7));
  mu_assert_int_eq(-1, strnchk_semver("5.12.17-rc1+001 ", 8));
  mu_assert_int_eq(9,  strnchk_semver("5.12.17-rc1+001 ", 9));
  mu_assert_int_eq(11, strnchk_semver("5.12.17-rc1+001 ", 11));
  mu_assert_int_eq(-1, strnchk_semver("5.12.17-rc1+001 ", 12));
  mu_assert_int_eq(13, strnchk_semver("5.12.17-rc1+001 ", 13));
  mu_assert_int_eq(15, strnchk_semver("5.12.17-rc1+001 ", 15));
  mu_assert_int_eq(-1, strnchk_semver("5.12.17-rc1+001 ", 16));
  mu_assert_int_eq(-1, strnchk_semver("5.12.17-rc1+001 ", 20));
}


MU_TEST(test_strcmp_semver)
{
  mu_assert_int_eq(0,  strcmp_semver("0.1.1", "0.1.1"));
  mu_assert_int_eq(-1, strcmp_semver("0.1.2", "0.1.11"));
  mu_assert_int_eq(-1, strcmp_semver("0.1.2", "0.2.11"));
  mu_assert_int_eq(1,  strcmp_semver("0.5.2", "0.2.11"));
  mu_assert_int_eq(-1, strcmp_semver("5.12.17", "5.12.17-rc1+001"));
  mu_assert_int_eq(0,  strcmp_semver("5.12.17", "5.12.17+001"));
  mu_assert_int_eq(1,  strcmp_semver("5.12.17", "5.11.17+001"));
  mu_assert_int_eq(1,  strcmp_semver("5.12.17-rc1.12", "5.12.17-rc1.4"));
}


MU_TEST(test_strncmp_semver)
{
  mu_assert_int_eq(0,  strncmp_semver("0.1.1", "0.1.1", 5));
  mu_assert_int_eq(0,  strncmp_semver("0.1.1", "0.1.1", 6));
  mu_assert_int_eq(0,  strncmp_semver("0.1.1", "0.1.12", 5));
  mu_assert_int_eq(-1, strncmp_semver("0.1.1", "0.1.12", 6));
  mu_assert_int_eq(1,  strncmp_semver("0.1.2", "0.1.11", 5));
  mu_assert_int_eq(-1, strncmp_semver("0.1.2", "0.1.11", 6));
  mu_assert_int_eq(0,  strncmp_semver("5.12.17", "5.12.17-rc1+001", 6));
  mu_assert_int_eq(-1, strncmp_semver("5.12.17", "5.12.17-rc1+001", 7));
  mu_assert_int_eq(-1, strncmp_semver("5.12.17", "5.12.17-rc1+001", 9));
  mu_assert_int_eq(-1, strncmp_semver("5.12.17", "5.12.17-rc1+001", 20));
  mu_assert_int_eq(0,  strncmp_semver("5.12.17", "5.12.17+001", 7));
  mu_assert_int_eq(0,  strncmp_semver("5.12.17", "5.12.17+001", 9));
  mu_assert_int_eq(0,  strncmp_semver("5.12.17", "5.12.17+001", 20));
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_strsetc);
  MU_RUN_TEST(test_strsets);
  MU_RUN_TEST(test_strnput);
  MU_RUN_TEST(test_strnput_escape);
  MU_RUN_TEST(test_strquote);
  MU_RUN_TEST(test_strnquote);
  MU_RUN_TEST(test_strunquote);
  MU_RUN_TEST(test_strput_unquote);
  MU_RUN_TEST(test_strhex_encode);
  MU_RUN_TEST(test_strhex_decode);
  MU_RUN_TEST(test_strcategory);
  MU_RUN_TEST(test_strcatspn);
  MU_RUN_TEST(test_strlst);
  MU_RUN_TEST(test_natoi);
  MU_RUN_TEST(test_strchk_semver);
  MU_RUN_TEST(test_strnchk_semver);
  MU_RUN_TEST(test_strcmp_semver);
  MU_RUN_TEST(test_strncmp_semver);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
