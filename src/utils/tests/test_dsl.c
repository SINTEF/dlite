#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dsl.h"
#include "test_macros.h"

#include "minunit/minunit.h"


dsl_handle handle=NULL;

typedef int (*Sum)(int x, int y);


MU_TEST(test_dsl_open)
{
  char *path = STRINGIFY(LIBDIR) "/" DSL_PREFIX "test_dsl_lib" DSL_EXT;
  printf("\n*** path='%s'\n", path);

  handle = dsl_open(path, RTLD_NOW);
  mu_check(handle);
  mu_check(!dsl_error());
}


MU_TEST(test_dsl_sym)
{
  const char *s;
  void *f;
  Sum sum;

  f = dsl_sym(handle, "non_existent");
  mu_check(!f);
  s = dsl_error();
  mu_check(s);
  printf("\n    msg: '%s'\n", s);

  f = dsl_sym(handle, "func");
  mu_check(f);
  mu_check(!dsl_error());

  // the cast is to scilence gcc warning about that ISO C forbids
  // conversion of object pointer to function pointer type
  *(void **)(&sum) = f;

  mu_assert_int_eq(0, sum(0, 0));
  mu_assert_int_eq(5, sum(2, 3));
  mu_assert_int_eq(-1, sum(2, -3));

}


MU_TEST(test_dsl_close)
{
  mu_assert_int_eq(0, dsl_close(handle));
  mu_check(!dsl_error());
}

/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_dsl_open);       /* setup */
  MU_RUN_TEST(test_dsl_sym);
  MU_RUN_TEST(test_dsl_close);      /* tear down */
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
