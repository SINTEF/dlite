#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "err.h"
#include "test_macros.h"

#include "minunit/minunit.h"


MU_TEST(test_err_functions)
{
  char *msg;

  /* Override old errors - so we don't need to clear the error for each test */
  err_set_override_mode(errOverrideOld);

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
  FILE *fp = fopen("", "r");
  mu_check(fp == NULL);

  msg = "Error 2: my errmsg: ";
  mu_assert_int_eq(2, err(2, "my errmsg"));
  mu_assert_int_eq(2, err_geteval());
  mu_check(strlen(err_getmsg()) > strlen(msg));
  mu_assert_int_eq(0, strncmp(err_getmsg(), msg, strlen(msg)));

  /* Adding prefix */
  mu_assert_string_eq("", err_set_prefix("test_err"));
  msg = "test_err: Error 2: my errmsg";
  mu_assert_int_eq(2, err(2, "my errmsg"));
  mu_assert_int_eq(0, strcmp(err_getmsg(), msg));

  msg = "test_err: Error 2: my errmsg2";
  mu_assert_int_eq(2, errx(2, "my errmsg2"));
  mu_assert_int_eq(2, err_geteval());
  mu_assert_string_eq(msg, err_getmsg());

  msg = "test_err: Warning: my msg";
  mu_assert_int_eq(0, warn("my msg"));
  mu_assert_int_eq(0, err_geteval());
  mu_assert_int_eq(0, strcmp(err_getmsg(), msg));

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

  /* Reset */
  err_set_warn_mode(0);
  err_set_debug_mode(0);
  err_set_override_mode(0);
}


enum {
  errA = 1,
  errB = 2,
  errC = 3,
  errD = 4,
  errE = 5,
  errF = 6,
};
enum {
  vCtA = 1,
  vCtB = 2,
  vCtC = 4,
  vCtD = 8,
  vOth = 16,
  vEls = 32,
  vFin = 64
};

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

  printf("\n-------------- eval=%d, action=%d --------------\n", eval, action);
  assert(err_geteval() == 0);

  ErrTry:
    if (action & 1) err(eval, "err1");
    if (action & 2) fun2();
    if (action & 4) fun4(eval);
    if (action & 8) fun8(eval);
  ErrCatch(errA):
    cval |= vCtA;
    printf("*** ErrCatch A: '%s'\n", err_getmsg());
    break;
  ErrCatch(errB):
    cval |= vCtB;
    printf("*** ErrCatch B: '%s'\n", err_getmsg());
    // fall-through
  ErrCatch(errC):
    cval |= vCtC;
    printf("*** ErrCatch C: '%s'\n", err_getmsg());
    break;
  ErrCatch(errD):
    cval |= vCtD;
    printf("*** ErrCatch D: '%s'\n", err_getmsg());
    err(errF, "errF when handling errD\n");
    break;
  ErrOther:
    cval |= vOth;
    printf("*** ErrOther: '%s'\n", err_getmsg());
  ErrElse:
    cval |= vEls;
    printf("*** ErrElse: '%s'\n", err_getmsg());
  ErrFinally:
    cval |= vFin;
    printf("*** ErrFinally: '%s'\n", err_getmsg());
    fflush(stderr);
  ErrEnd;
  return cval;
}

MU_TEST(test_errtry)
{
  mu_assert_int_eq(vEls | vFin, tryfun(errA, 0));
  mu_assert_int_eq(vEls | vFin, tryfun(errB, 0));
  mu_assert_int_eq(vEls | vFin, tryfun(errC, 0));
  mu_assert_int_eq(vEls | vFin, tryfun(errD, 0));
  mu_assert_int_eq(vEls | vFin, tryfun(errE, 0));
  mu_assert_int_eq(0, err_geteval());

  mu_assert_int_eq(vCtA | vFin, tryfun(errA, 1));
  mu_assert_int_eq(vCtB | vCtC | vFin, tryfun(errB, 1));
  mu_assert_int_eq(vCtC | vFin, tryfun(errC, 1));
  mu_assert_int_eq(vCtD | vFin, tryfun(errD, 1));
  mu_assert_int_eq(errF, err_geteval());
  mu_assert_int_eq(vOth | vFin, tryfun(errE, 1));
  mu_assert_int_eq(0, err_geteval());

  mu_assert_int_eq(vEls | vFin, tryfun(errA, 2));
  mu_assert_int_eq(vEls | vFin, tryfun(errB, 2));
  mu_assert_int_eq(vEls | vFin, tryfun(errC, 2));
  mu_assert_int_eq(vEls | vFin, tryfun(errD, 2));
  mu_assert_int_eq(0, err_geteval());
  mu_assert_int_eq(vEls | vFin, tryfun(errE, 2));
  mu_assert_int_eq(0, err_geteval());

  mu_assert_int_eq(vCtA | vFin, tryfun(errA, 3));
  mu_assert_int_eq(0, err_geteval());

  mu_assert_int_eq(vCtA | vFin, tryfun(errA, 4));
  mu_assert_int_eq(vCtB |vCtC | vFin, tryfun(errB, 4));
  mu_assert_int_eq(vCtC | vFin, tryfun(errC, 4));
  mu_assert_int_eq(vCtD | vFin, tryfun(errD, 4));
  mu_assert_int_eq(errF, err_geteval());
  mu_assert_int_eq(vOth | vFin, tryfun(errE, 4));
  mu_assert_int_eq(0, err_geteval());

  mu_assert_int_eq(vCtA | vFin, tryfun(errA, 8));
  mu_assert_int_eq(vCtB | vCtC | vFin, tryfun(errB, 8));
  mu_assert_int_eq(vCtC | vFin, tryfun(errC, 8));
  mu_assert_int_eq(vCtD | vFin, tryfun(errD, 8));
  mu_assert_int_eq(errF, err_geteval());
  mu_assert_int_eq(vOth | vFin, tryfun(errE, 8));
  mu_assert_int_eq(0, err_geteval());
}

int tryfun2(int eval, int action)
{
  int cval=0;
  errno = 0;
  err_set_prefix("");
  err_set_debug_mode(0);
  err_clear();

  printf("\n-------------- eval=%d, action=%d -------------- tryfun2\n",
         eval, action);
  assert(err_geteval() == 0);

  ErrTry:
    if (action & 1) err(eval, "err1");
    if (action & 2) fun2();
    if (action & 4) fun4(eval);
    if (action & 8) fun8(eval);
  ErrCatch(errA):
    cval |= vCtA;
    printf("*** ErrCatch A: '%s'\n", err_getmsg());
    err_reraise();
    break;
  ErrCatch(errB):
    cval |= vCtB;
    printf("*** ErrCatch B: '%s'\n", err_getmsg());
    // fall-through
  ErrCatch(errC):
    cval |= vCtC;
    printf("*** ErrCatch C: '%s'\n", err_getmsg());
    break;
  ErrCatch(errD):
    cval |= vCtD;
    printf("*** ErrCatch D: '%s'\n", err_getmsg());
    err(errF, "errF when handling errD\n");
    break;
  ErrEnd;
  return cval;
}

MU_TEST(test_errtry2)
{
  mu_assert_int_eq(0, tryfun2(errA, 0));
  mu_assert_int_eq(0, err_geteval());
  mu_assert_int_eq(0, tryfun2(errB, 0));
  mu_assert_int_eq(0, err_geteval());
  mu_assert_int_eq(0, tryfun2(errC, 0));
  mu_assert_int_eq(0, err_geteval());
  mu_assert_int_eq(0, tryfun2(errD, 0));
  mu_assert_int_eq(0, err_geteval());
  mu_assert_int_eq(0, tryfun2(errE, 0));
  mu_assert_int_eq(0, err_geteval());

  mu_assert_int_eq(vCtA, tryfun2(errA, 1));
  mu_assert_int_eq(vCtA, err_geteval());
  mu_assert_int_eq(vCtB | vCtC, tryfun2(errB, 1));
  mu_assert_int_eq(0, err_geteval());
  mu_assert_int_eq(vCtC, tryfun2(errC, 1));
  mu_assert_int_eq(0, err_geteval());
  mu_assert_int_eq(vCtD, tryfun2(errD, 1));
  mu_assert_int_eq(errF, err_geteval());
  mu_assert_int_eq(0, tryfun2(errE, 1));
  mu_assert_int_eq(errE, err_geteval());

  mu_assert_int_eq(0, tryfun2(errA, 2));
  mu_assert_int_eq(0, tryfun2(errB, 2));
  mu_assert_int_eq(0, tryfun2(errC, 2));
  mu_assert_int_eq(0, tryfun2(errD, 2));
  mu_assert_int_eq(0, err_geteval());
  mu_assert_int_eq(0, tryfun2(errE, 2));
  mu_assert_int_eq(0, err_geteval());

  mu_assert_int_eq(vCtA, tryfun2(errA, 3));
  mu_assert_int_eq(errA, err_geteval());

  //mu_assert_int_eq(vCtA, tryfun2(errA, 4));  // terminates; uncaught exception
  mu_assert_int_eq(vCtB | vCtC, tryfun2(errB, 4));
  mu_assert_int_eq(0, err_geteval());
  mu_assert_int_eq(vCtC, tryfun2(errC, 4));
  mu_assert_int_eq(0, err_geteval());
  mu_assert_int_eq(vCtD, tryfun2(errD, 4));
  mu_assert_int_eq(errF, err_geteval());
  //mu_assert_int_eq(0, tryfun2(errE, 4));  // terminates; uncaught exception

  mu_assert_int_eq(vCtA, tryfun2(errA, 8));
  mu_assert_int_eq(errA, err_geteval());
  mu_assert_int_eq(vCtB | vCtC, tryfun2(errB, 8));
  mu_assert_int_eq(0, err_geteval());
  mu_assert_int_eq(vCtC, tryfun2(errC, 8));
  mu_assert_int_eq(0, err_geteval());
  mu_assert_int_eq(vCtD, tryfun2(errD, 8));
  mu_assert_int_eq(errF, err_geteval());
  mu_assert_int_eq(0, tryfun2(errE, 8));
  mu_assert_int_eq(errE, err_geteval());
}

/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_err_functions);
  MU_RUN_TEST(test_errtry);
  MU_RUN_TEST(test_errtry2);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
