#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "dlite.h"
#include "dlite-macros.h"

#include "minunit/minunit.h"



MU_TEST(test_scan)
{
  int n;
  DLiteProperty prop;
  memset(&prop, 0, sizeof(prop));

  double d;
  prop.type = dliteFloat;
  prop.size = sizeof(double);
  n = dlite_property_scan("3.14", &d, &prop, NULL, 0);
  mu_assert_int_eq(4, n);
  mu_assert_double_eq(3.14, d);
  n = dlite_property_scan("3.14", &d, &prop, NULL, dliteFlagQuoted);
  mu_assert_int_eq(4, n);
  mu_assert_double_eq(3.14, d);

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



}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_scan);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
