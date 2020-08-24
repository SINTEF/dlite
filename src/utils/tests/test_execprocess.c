#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "execprocess.h"
#include "test_macros.h"

#include "minunit/minunit.h"


#ifdef WIN32
#define EXEEXT ".exe"
#else
#define EXEEXT
#endif



MU_TEST(test_execprocess)
{
  char *prog = STRINGIFY(BINDIR) "/test_uuid" EXEEXT;
  int stat = exec_process(prog, NULL, NULL);
  mu_assert_int_eq(0, stat);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_execprocess);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
