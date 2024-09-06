#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uri_encode.h"

#include "minunit/minunit.h"


char buf[256];

/* tests for encode_uri */
MU_TEST(test_encode_empty) {
  int n = uri_encode("", 0, buf);
  mu_assert_string_eq("", buf);
  mu_assert_int_eq(0, n);
}
MU_TEST(test_encode_something) {
  int n = uri_encode("something", 9, buf);
  mu_assert_string_eq("something", buf);
  mu_assert_int_eq(9, n);
}
MU_TEST(test_encode_something_percent) {
  int n = uri_encode("something%", 10, buf);
  mu_assert_string_eq("something%25", buf);
  mu_assert_int_eq(12, n);
}
MU_TEST(test_encode_something_zslash) {
  int n = uri_encode("something%z/", 12, buf);
  mu_assert_string_eq("something%25z%2F", buf);
  mu_assert_int_eq(16, n);
}
MU_TEST(test_encode_space) {
  int n = uri_encode(" ", 1, buf);
  mu_assert_string_eq("%20", buf);
  mu_assert_int_eq(3, n);
}
MU_TEST(test_encode_percent) {
  int n = uri_encode("%%20", 4, buf);
  mu_assert_string_eq("%25%2520", buf);
  mu_assert_int_eq(8, n);
}
MU_TEST(test_encode_latin1) {
  int n = uri_encode("|abcå", 6, buf);
  mu_assert_string_eq("%7Cabc%C3%A5", buf);
  mu_assert_int_eq(12, n);
}
MU_TEST(test_encode_symbols) {
  int n = uri_encode("~*'()", 5, buf);
  mu_assert_string_eq("~%2A%27%28%29", buf);
  mu_assert_int_eq(13, n);
}
MU_TEST(test_encode_angles) {
  int n = uri_encode("<\">", 3, buf);
  mu_assert_string_eq("%3C%22%3E", buf);
  mu_assert_int_eq(9, n);
}
MU_TEST(test_encode_middle_null) {
  int n = uri_encode("ABC\0DEF", 3, buf);
  mu_assert_string_eq("ABC", buf);
  mu_assert_int_eq(3, n);
}
MU_TEST(test_encode_middle_null_len) {
  int n = uri_encode("ABC\0DEF", 7, buf);
  mu_assert_string_eq("ABC%00DEF", buf);
  mu_assert_int_eq(9, n);
}
MU_TEST(test_encode_latin1_utf8) {
  int n = uri_encode("åäö", strlen("åäö"), buf);
  mu_assert_string_eq("%C3%A5%C3%A4%C3%B6", buf);
  mu_assert_int_eq(18, n);
}
MU_TEST(test_encode_utf8) {
  int n = uri_encode("❤", strlen("❤"), buf);
  mu_assert_string_eq("%E2%9D%A4", buf);
  mu_assert_int_eq(9, n);
}

/* tests for decode_uri */
MU_TEST(test_decode_empty) {
  int n = uri_decode("", 0, buf);
  mu_assert_string_eq("", buf);
  mu_assert_int_eq(0, n);
}
MU_TEST(test_decode_something) {
  int n = uri_decode("something", 9, buf);
  mu_assert_string_eq("something", buf);
  mu_assert_int_eq(9, n);
}
MU_TEST(test_decode_something_percent) {
  int n = uri_decode("something%", 10, buf);
  mu_assert_string_eq("something%", buf);
  mu_assert_int_eq(10, n);
}
MU_TEST(test_decode_something_percenta) {
  int n = uri_decode("something%a", 11, buf);
  mu_assert_string_eq("something%a", buf);
  mu_assert_int_eq(11, n);
}
MU_TEST(test_decode_something_zslash) {
  int n = uri_decode("something%Z/", 12, buf);
  mu_assert_string_eq("something%Z/", buf);
  mu_assert_int_eq(12, n);
}
MU_TEST(test_decode_space) {
  int n = uri_decode("%20", 3, buf);
  mu_assert_string_eq(" ", buf);
  mu_assert_int_eq(1, n);
}
MU_TEST(test_decode_percents) {
  int n = uri_decode("%25%2520", 8, buf);
  mu_assert_string_eq("%%20", buf);
  mu_assert_int_eq(4, n);
}
MU_TEST(test_decode_latin1) {
  int n = uri_decode("%7Cabc%C3%A5", 12, buf);
  mu_assert_string_eq("|abcå", buf);
  mu_assert_int_eq(6, n);
}
MU_TEST(test_decode_symbols) {
  int n = uri_decode("~%2A%27%28%29", 13, buf);
  mu_assert_string_eq("~*'()", buf);
  mu_assert_int_eq(5, n);
}
MU_TEST(test_decode_angles) {
  int n = uri_decode("%3C%22%3E", 9, buf);
  mu_assert_string_eq("<\">", buf);
  mu_assert_int_eq(3, n);
}
MU_TEST(test_decode_middle_null) {
  int n = uri_decode("ABC%00DEF", 6, buf);
  mu_assert_string_eq("ABC\0", buf);
  mu_assert_int_eq(4, n);
}
MU_TEST(test_decode_middle_null2) {
  int n = uri_decode("ABC%00DEF", 5, buf);
  mu_assert_string_eq("ABC%0", buf);
  mu_assert_int_eq(5, n);
}
MU_TEST(test_decode_middle_full) {
  int n = uri_decode("ABC%00DEF", 9, buf);
  mu_assert_string_eq("ABC\0DEF", buf);
  mu_assert_int_eq(7, n);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_encode_empty);
  MU_RUN_TEST(test_encode_something);
  MU_RUN_TEST(test_encode_something_percent);
  MU_RUN_TEST(test_encode_something_zslash);
  MU_RUN_TEST(test_encode_percent);
  MU_RUN_TEST(test_encode_space);
  MU_RUN_TEST(test_encode_empty);
  MU_RUN_TEST(test_encode_latin1);
  MU_RUN_TEST(test_encode_symbols);
  MU_RUN_TEST(test_encode_angles);
  MU_RUN_TEST(test_encode_middle_null);
  MU_RUN_TEST(test_encode_middle_null_len);
  MU_RUN_TEST(test_encode_latin1_utf8);
  MU_RUN_TEST(test_encode_utf8);

  MU_RUN_TEST(test_decode_empty);
  MU_RUN_TEST(test_decode_something);
  MU_RUN_TEST(test_decode_something_percent);
  MU_RUN_TEST(test_decode_something_percenta);
  MU_RUN_TEST(test_decode_something_zslash);
  MU_RUN_TEST(test_decode_space);
  MU_RUN_TEST(test_decode_percents);
  MU_RUN_TEST(test_decode_latin1);
  MU_RUN_TEST(test_decode_symbols);
  MU_RUN_TEST(test_decode_angles);
  MU_RUN_TEST(test_decode_middle_null);
  MU_RUN_TEST(test_decode_middle_null2);
  MU_RUN_TEST(test_decode_middle_full);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
