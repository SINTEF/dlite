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



MU_TEST(test_exec_process)
{
  char *prog = STRINGIFY(BINDIR) "/test_uuid" EXEEXT;
  int stat = exec_process(prog, NULL, NULL);
  mu_assert_int_eq(0, stat);
}

MU_TEST(test_get_envvar)
{
  char **env = get_environment();
#ifdef WIN32
  char *name = "USERNAME";
#else
  char *name = "USER";
#endif
  mu_assert_string_eq(getenv(name), get_envvar(env, name));
  mu_assert_string_eq(NULL, get_envvar(env, "a non existing env var"));
  strlist_free(env);
}

MU_TEST(test_set_envvar)
{
  char **q, **env = get_environment();
  int len=0, n=0;
  for (q=env; *q; q++) len++;

  env = set_envvar(env, "_newVar", "42");
  for (q=env; *q; q++) n++;
  mu_assert_int_eq(len+1, n);
  mu_assert_string_eq("42", get_envvar(env, "_newVar"));

  strlist_free(env);
}




/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_exec_process);
  MU_RUN_TEST(test_get_envvar);
  MU_RUN_TEST(test_set_envvar);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
