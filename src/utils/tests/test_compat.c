#include <stdio.h>
#include <string.h>

#include "compat.h"
#include "test_macros.h"

#include "minunit/minunit.h"



MU_TEST(test_asnpprintf)
{
  char *buf=NULL;
  size_t size=0;
  int n=0, m;

  m = asnpprintf(&buf, &size, n, "n=%d", n);
  mu_assert_int_eq(3, m);
  n += m;
  mu_check((int)size > n);

  m = asnpprintf(&buf, &size, n, ", ");
  mu_assert_int_eq(2, m);
  n += m;
  mu_check((int)size > n);

  m = asnpprintf(&buf, &size, n, "s='%s'\n", "a string");
  mu_assert_int_eq(13, m);
  n += m;
  mu_check((int)size > n);

  mu_assert_int_eq(18, n);

  free(buf);
}

/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_asnpprintf);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
