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
  // cppcheck-suppress leakReturnValNotUsed
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


#define errA  1
#define errB  2
#define errC  4
#define errD  8

void fun2(void)
{
  printf("\nfun2...\n");
}

void fun4(int eval)
{
  err_raise(eval, "new exception");
  printf("\nfun4...\n");
}

void fun8(int eval)
{
  err(eval, "fun8");
  printf("\nfun8...\n");
}

int tryfun(int eval, int action)
{
  int cval=0;
  errno = 0;
  err_set_prefix("");
  err_set_debug_mode(0);
  err_clear();

  mu_assert_int_eq(0, err_geteval());

  ErrTry:
    if (action & 1) err(eval, "err1");
    if (action & 2) fun2();
    if (action & 4) fun4(eval);
    if (action & 8) fun8(eval);
    break;
  ErrCatch(errA):
    cval |= 1;
    printf("\n*** ErrCatch A: '%s'\n", err_getmsg());
    break;
  ErrCatch(errB):
  ErrCatch(errC):
    cval |= 4;
    printf("\n*** ErrCatch B, C: '%s'\n", err_getmsg());
    break;
  ErrOther:
    cval |= 8;
    printf("\n*** ErrOther: '%s'\n", err_getmsg());
    break;
  ErrElse:
    cval |= 16;
    printf("\n*** ErrElse: '%s'\n", err_getmsg());
  ErrFinally:
    cval |= 32;
    printf("\n*** ErrFinally: '%s'\n", err_getmsg());
    fflush(stderr);
  ErrEnd;
  return cval;
}

MU_TEST(test_errtry)
{
  mu_assert_int_eq(16    | 32, tryfun(errA, 0));
  //mu_assert_int_eq(1     | 32, tryfun(errA, 1));
  //mu_assert_int_eq(2 | 4 | 32, tryfun(errB, 1));
  //mu_assert_int_eq(4     | 32, tryfun(errC, 1));
  mu_assert_int_eq(8     | 32, tryfun(errD, 1));
}

/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_err_functions);
  MU_RUN_TEST(test_errtry);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
