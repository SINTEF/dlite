#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "globmatch.h"

#include "minunit/minunit.h"


MU_TEST(test_globmatch)
{
  mu_check(0 == globmatch("", ""));
  mu_check(1 == globmatch("?", ""));
  mu_check(0 == globmatch("*", ""));
  mu_check(0 == globmatch("?etc*", "/etc/fstab"));
  mu_check(0 == globmatch("/[a-f]tc*", "/etc/fstab"));
  mu_check(0 == globmatch("[^A-Za-z]etc*", "/etc/fstab"));
  mu_check(1 == globmatch("[A-Za-z]etc*", "/etc/fstab"));
  mu_check(0 == globmatch("\\/etc*", "/etc/fstab"));
  mu_check(0 == globmatch("?etc*tab", "/etc/fstab"));
}

/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_globmatch);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
