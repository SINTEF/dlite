#include <stdlib.h>
#include <stdio.h>

#include "uuid4.h"

#include "minunit/minunit.h"



MU_TEST(test_uuid4)
{
  char buf[UUID4_LEN];

  mu_assert_int_eq(0, uuid4_generate(buf));
  printf("\nuuid4: '%s'\n", buf);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_uuid4);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
