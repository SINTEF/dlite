#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "err.h"
#include "test_macros.h"

#include "minunit/minunit.h"


MU_TEST(test_err_functions)
{
  char *msg;
  mu_assert_int_eq(0, err_geteval());
  mu_assert_string_eq("", err_getmsg());

  msg = "Error 3: my errmsg";
  mu_assert_int_eq(3, err(3, "my errmsg"));
  mu_assert_int_eq(3, err_geteval());
  mu_assert_string_eq(msg, err_getmsg());

  msg = "Error 3: my errmsg 1";
  mu_assert_int_eq(3, err(3, "my errmsg %d", 1));
  mu_assert_string_eq(msg, err_getmsg());

  msg = "Error 3: my errmsg arg1, 3.14";
  mu_assert_int_eq(3, err(3, "my errmsg %s, %.2f", "arg1", 3.1415));
  mu_assert_int_eq(3, err_geteval());
  mu_assert_string_eq(msg, err_getmsg());

  /* Failing system call */
  fopen("", "r");

  msg = "Error 2: my errmsg: ";
  mu_assert_int_eq(2, err(2, "my errmsg"));
  mu_assert_int_eq(2, err_geteval());
  mu_check(strlen(err_getmsg()) > strlen(msg));
  mu_assert_int_eq(0, strncmp(err_getmsg(), msg, strlen(msg)));

  /* Adding prefix */
  mu_assert_string_eq("", err_set_prefix("test_err"));

  msg = "test_err: Error 2: my errmsg: ";
  mu_assert_int_eq(2, err(2, "my errmsg"));
  mu_check(strlen(err_getmsg()) > strlen(msg));
  mu_assert_int_eq(0, strncmp(err_getmsg(), msg, strlen(msg)));

  msg = "test_err: Error 2: my errmsg2";
  mu_assert_int_eq(2, errx(2, "my errmsg2"));
  mu_assert_int_eq(2, err_geteval());
  mu_assert_string_eq(msg, err_getmsg());

  msg = "test_err: Warning: my msg: ";
  mu_assert_int_eq(0, warn("my msg"));
  mu_assert_int_eq(0, err_geteval());
  mu_check(strlen(err_getmsg()) > strlen(msg));
  mu_assert_int_eq(0, strncmp(err_getmsg(), msg, strlen(msg)));

  msg = "test_err: Warning: my msg2";
  mu_assert_int_eq(0, warnx("my msg2"));
  mu_assert_int_eq(0, err_geteval());
  mu_assert_string_eq(msg, err_getmsg());

  err_clear();
  mu_assert_int_eq(0, err_geteval());
  mu_assert_string_eq("", err_getmsg());

  /* Enabling debugging mode */
  err_set_debug_mode(1);
  err(1, "errmsg");

  mu_assert_int_eq(1, err_set_debug_mode(2));
  err(1, "errmsg");
}

void nested_func()
{
  err(2, "nested error");  /* swallowed error... */
  raise(6, "new exception");
  printf("\nprocessing...\n");
}


MU_TEST(test_err_try)
{
  errno = 0;
  err_set_prefix("");
  err_set_debug_mode(0);
  err_clear();

  mu_assert_int_eq(0, err_geteval());

  ErrTry:
    nested_func();
    break;
  ErrCatch(1):
    printf("\n*** ErrCatch 1: '%s'\n", err_getmsg());
    break;
  ErrCatch(2):
  ErrCatch(3):
    printf("\n*** ErrCatch 2, 3: '%s'\n", err_getmsg());
    mu_assert_int_eq(2, err_geteval());
    break;
  ErrOther:
    printf("\n*** ErrOther: '%s'\n", err_getmsg());
    break;
  ErrElse:
    printf("\n*** ErrElse: '%s'\n", err_getmsg());
  ErrFinally:
    printf("\n*** ErrFinally: '%s'\n", err_getmsg());
    fflush(stderr);
  ErrEnd;

  printf("\n*** continue... (eval=%d)\n", err_geteval());

  mu_assert_int_eq(0, err_geteval());
}

/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_err_functions);
  MU_RUN_TEST(test_err_try);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
