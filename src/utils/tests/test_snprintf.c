/*
  snprintf() and vsnprintf() are broken on some systems
  (e.g. Windows).  This test makes sure that we are using the
  implementation in compat/snprintf.c on those systems.
 */
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "compat.h"
#include "minunit/minunit.h"


int wrap_vsnprintf(char *str, size_t size, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int n;
  n = vsnprintf(str, size, fmt, ap);
  va_end(ap);
  return n;
}


MU_TEST(test_vsnprintf)
{
  char buf[10];
  char *short_string = "abc";
  char *long_string = "0123456789abcdef";
  int n;

  printf("\n\n--- test_vsnprintf\n");
  n = wrap_vsnprintf(buf, sizeof(buf), "%s", short_string);
  mu_assert_int_eq(strlen(short_string), n);

  n = wrap_vsnprintf(buf, sizeof(buf), "%s", long_string);
  mu_assert_int_eq(strlen(long_string), n);
}


MU_TEST(test_snprintf)
{
  char buf[10];
  char *short_string = "abc";
  char *long_string = strdup("0123456789abcdef");
  int n;

  printf("\n\n--- test_snprintf\n");
  n = snprintf(buf, sizeof(buf), "%s", short_string);
  mu_assert_int_eq(strlen(short_string), n);

  memset(buf, 0, sizeof(buf));
  n = snprintf(buf, 4, "%s", long_string);
  printf("\n*** n=%d, buf='%.10s'\n", n, buf);


  n = snprintf(buf, sizeof(buf), "%s", long_string);
  mu_assert_int_eq(strlen(long_string), n);

  free(long_string);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_vsnprintf);
  MU_RUN_TEST(test_snprintf);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
