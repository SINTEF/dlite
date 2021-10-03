#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "utils/integers.h"
#include "dlite.h"
#include "dlite-macros.h"

#include "minunit/minunit.h"



MU_TEST(test_print)
{
  DLiteProperty prop;
  char *s=NULL;
  size_t size=0;
  int m, n=0;
  memset(&prop, 0, sizeof(prop));

  unsigned char blob[] = {0xff, 0x0f, 0x10, 0x01};
  prop.type = dliteBlob;
  prop.size = sizeof(blob);
  m = dlite_property_aprint(&s, &size, n, blob, &prop, NULL, 0, -2, 0);
  mu_assert_int_eq(10, m);
  mu_assert_string_eq("\"ff0f1001\"", s);

  bool b = 1;
  prop.type = dliteBool;
  prop.size = sizeof(b);
  m = dlite_property_aprint(&s, &size, n, &b, &prop, NULL, 0, -2, 0);
  mu_assert_int_eq(4, m);
  mu_assert_string_eq("true", s);
  b = 0;
  m = dlite_property_aprint(&s, &size, n, &b, &prop, NULL, 0, -2, 0);
  mu_assert_int_eq(5, m);
  mu_assert_string_eq("false", s);

  int i = -42;
  prop.type = dliteInt;
  prop.size = sizeof(int);
  m = dlite_property_aprint(&s, &size, n, &i, &prop, NULL, 0, -2, 0);
  mu_assert_int_eq(3, m);
  mu_assert_string_eq("-42", s);

  unsigned short u = 42;
  prop.type = dliteUInt;
  prop.size = sizeof(u);
  m = dlite_property_aprint(&s, &size, n, &u, &prop, NULL, 0, -2, 0);
  mu_assert_int_eq(2, m);
  mu_assert_string_eq("42", s);

  float f = 3.14f;
  prop.type = dliteFloat;
  prop.size = sizeof(f);
  m = dlite_property_aprint(&s, &size, n, &f, &prop, NULL, 0, -2, 0);
  mu_assert_int_eq(4, m);
  mu_assert_string_eq("3.14", s);

  char sfix[] = "a fix string";
  prop.type = dliteFixString;
  prop.size = sizeof(sfix);
  m = dlite_property_aprint(&s, &size, n, sfix, &prop, NULL, 0, -2, 0);
  mu_assert_int_eq(14, m);
  mu_assert_string_eq("\"a fix string\"", s);

  char *str = "a string";
  prop.type = dliteStringPtr;
  prop.size = sizeof(str);
  m = dlite_property_aprint(&s, &size, n, &str, &prop, NULL, 0, -2, 0);
  mu_assert_int_eq(10, m);
  mu_assert_string_eq("\"a string\"", s);

  DLiteDimension dim = {"N", "number of something"};
  prop.type = dliteDimension;
  prop.size = sizeof(dim);
  m = dlite_property_aprint(&s, &size, n, &dim, &prop, NULL, 0, -2, 0);
  mu_assert_int_eq(51, m);
  mu_assert_string_eq("{\"name\": \"N\", "
                      "\"description\": \"number of something\"}", s);

  char *dims[] = {"M", "N"};
  DLiteProperty p = {"x", dliteInt, 4, 2, dims, "m", NULL, "about x..."};
  prop.type = dliteProperty;
  prop.size = sizeof(p);
  m = dlite_property_aprint(&s, &size, n, &p, &prop, NULL, 0, -2, 0);
  mu_assert_int_eq(104, m);
  mu_assert_string_eq("{\"name\": \"x\", \"type\": \"int32\", \"ndims\": 2, "
                      "\"dims\": [\"M\", \"N\"], \"unit\": \"m\", "
                      "\"description\": \"about x...\"}", s);

  DLiteRelation rel = {"subject", "predicate", "object", "id"};
  prop.type = dliteRelation;
  prop.size = sizeof(rel);
  m = dlite_property_aprint(&s, &size, n, &rel, &prop, NULL, 0, -2, 0);
  mu_assert_int_eq(34, m);
  mu_assert_string_eq("[\"subject\", \"predicate\", \"object\"]", s);

  free(s);
}


MU_TEST(test_print_arr)
{
  DLiteProperty prop;
  char *s=NULL;
  size_t size=0;
  int m, n=0;
  memset(&prop, 0, sizeof(prop));

  size_t dims[] = {2, 3};
  bool b[2][3] = {{1, 0, 1}, {0, 0, 1}};
  prop.type = dliteBool;
  prop.size = sizeof(bool);
  prop.ndims = 2;
  m = dlite_property_aprint(&s, &size, n, b, &prop, dims, 0, -2, 0);
  mu_assert_int_eq(43, m);
  mu_assert_string_eq("[[true, false, true], [false, false, true]]", s);

  free(s);
}


MU_TEST(test_scan)
{
  int n;
  DLiteProperty prop;
  memset(&prop, 0, sizeof(prop));

  unsigned char blob[4];
  prop.type = dliteBlob;
  prop.size = sizeof(blob);
  n = dlite_property_scan("\"ff0a1008\"", &blob, &prop, NULL, dliteFlagQuoted);
  mu_assert_int_eq(10, n);
  mu_assert_int_eq(255, blob[0]);
  mu_assert_int_eq(10, blob[1]);
  mu_assert_int_eq(16, blob[2]);
  mu_assert_int_eq(8, blob[3]);

  bool b;
  prop.type = dliteBool;
  prop.size = sizeof(b);
  n = dlite_property_scan("True", &b, &prop, NULL, 0);
  mu_assert_int_eq(4, n);
  mu_assert_int_eq(1, b);
  n = dlite_property_scan("OFF", &b, &prop, NULL, 0);
  mu_assert_int_eq(3, n);
  mu_assert_int_eq(0, b);
  n = dlite_property_scan(" 1 ", &b, &prop, NULL, 0);
  mu_assert_int_eq(2, n);
  mu_assert_int_eq(1, b);

  int64_t i;
  prop.type = dliteInt;
  prop.size = sizeof(i);
  n = dlite_property_scan("-123456789", &i, &prop, NULL, 0);
  mu_assert_int_eq(10, n);
  mu_assert_int_eq(-123456789, i);

  uint8_t i8;
  prop.type = dliteUInt;
  prop.size = sizeof(i8);
  n = dlite_property_scan("254", &i8, &prop, NULL, 0);
  mu_assert_int_eq(3, n);
  mu_assert_int_eq(254, i8);

  double d;
  prop.type = dliteFloat;
  prop.size = sizeof(double);
  n = dlite_property_scan("3.14", &d, &prop, NULL, 0);
  mu_assert_int_eq(4, n);
  mu_assert_double_eq(3.14, d);
  n = dlite_property_scan("3.14", &d, &prop, NULL, dliteFlagQuoted);
  mu_assert_int_eq(4, n);
  mu_assert_double_eq(3.14, d);
  n = dlite_property_scan("3.14", &d, &prop, NULL, dliteFlagRaw);
  mu_assert_int_eq(4, n);
  mu_assert_double_eq(3.14, d);

  char buf[10];
  prop.type = dliteFixString;
  prop.size = sizeof(buf);
  n = dlite_property_scan("\"3.14\"", buf, &prop, NULL, dliteFlagQuoted);
  mu_assert_int_eq(6, n);
  mu_assert_string_eq("3.14", buf);
  n = dlite_property_scan("\"0123456789abcdef\"", buf, &prop, NULL,
                          dliteFlagQuoted);
  mu_assert_int_eq(18, n);
  mu_assert_string_eq("012345678", buf);

  char *s=NULL;
  prop.type = dliteStringPtr;
  prop.size = sizeof(char *);
  n = dlite_property_scan(" \"3.14\"  ", &s, &prop, NULL, 0);
  mu_assert_int_eq(9, n);
  mu_assert_string_eq(" \"3.14\"  ", s);
  free(s);
  s = NULL;
  n = dlite_property_scan(" \"3.14\"  ", &s, &prop, NULL, dliteFlagQuoted);
  mu_assert_int_eq(7, n);
  mu_assert_string_eq("3.14", s);
  free(s);

  DLiteDimension dim;
  prop.type = dliteDimension;
  prop.size = sizeof(dim);
  memset(&dim, 0, sizeof(dim));
  n = dlite_property_scan("  {\"name\": \"N\", "
                          "\"description\": \"Number of something\"}",
                          &dim, &prop, NULL, 0);
  mu_assert_int_eq(51, n);
  mu_assert_string_eq("N", dim.name);
  mu_assert_string_eq("Number of something", dim.description);
  free(dim.name);
  free(dim.description);

  DLiteProperty p;
  prop.type = dliteProperty;
  prop.size = sizeof(p);
  memset(&p, 0, sizeof(p));
  n = dlite_property_scan("{"
                          "\"name\": \"x\", "
                          "\"type\": \"float32\", "
                          "\"dims\": [\"N\", \"M\"], "
                          "\"unit\": \"cm\", "
                          "\"description\": \"A number\""
                          "}", &p, &prop, NULL, 0);
  mu_assert_int_eq(93, n);
  mu_assert_string_eq("x", p.name);
  mu_assert_int_eq(dliteFloat, p.type);
  mu_assert_int_eq(4, p.size);
  mu_assert_string_eq("cm", p.unit);
  mu_assert_string_eq("A number", p.description);
  free(p.name);
  free(p.dims[0]);
  free(p.dims[1]);
  free(p.dims);
  free(p.unit);
  free(p.description);

  DLiteRelation rel;
  prop.type = dliteRelation;
  prop.size = sizeof(rel);
  memset(&rel, 0, sizeof(rel));
  n = dlite_property_scan("[\"subject\", \"predicate\", \"object\"]",
                          &rel, &prop, NULL, 0);
  mu_assert_int_eq(34, n);
  mu_assert_string_eq("subject", rel.s);
  mu_assert_string_eq("predicate", rel.p);
  mu_assert_string_eq("object", rel.o);
  mu_assert_string_eq(NULL, rel.id);
  free(rel.s);
  free(rel.p);
  free(rel.o);
}


MU_TEST(test_scan_arr) {
  int n;
  DLiteProperty prop;
  memset(&prop, 0, sizeof(prop));

  int arr[2][3][2];
  size_t dims[] = {2, 3, 2};
  char *dimexpr[] = {"H", "K", "L"};
  prop.type = dliteInt;
  prop.size = sizeof(int);
  prop.ndims = 3;
  prop.dims = dimexpr;
  memset(arr, -1, 12*sizeof(int));
  n = dlite_property_scan("[[[0, 1], [2, 3], [4, 5]], "
                          " [[6, 7], [8, 9], [10, 11]]]",
                          arr, &prop, dims, 0);
  mu_assert_int_eq(55, n);
  mu_assert_int_eq(0,  arr[0][0][0]);
  mu_assert_int_eq(1,  arr[0][0][1]);
  mu_assert_int_eq(2,  arr[0][1][0]);
  mu_assert_int_eq(3,  arr[0][1][1]);
  mu_assert_int_eq(4,  arr[0][2][0]);
  mu_assert_int_eq(5,  arr[0][2][1]);
  mu_assert_int_eq(6,  arr[1][0][0]);
  mu_assert_int_eq(7,  arr[1][0][1]);
  mu_assert_int_eq(8,  arr[1][1][0]);
  mu_assert_int_eq(9,  arr[1][1][1]);
  mu_assert_int_eq(10, arr[1][2][0]);
  mu_assert_int_eq(11, arr[1][2][1]);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_print);
  MU_RUN_TEST(test_print_arr);
  MU_RUN_TEST(test_scan);
  MU_RUN_TEST(test_scan_arr);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
