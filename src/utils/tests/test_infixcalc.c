#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "infixcalc.h"

#include "minunit/minunit.h"


int calc(const char *expr, InfixCalcVariable *vars, size_t nvars)
{
  char err[256];
  int val = infixcalc(expr, vars, nvars, err, sizeof(err));
  printf("\n%s = ", expr);
  if (err[0])
    printf("\n*** Error: %s\n", err);
  else
    printf("%d\n", val);
  return val;
}


MU_TEST(test_infixcalc)
{
  InfixCalcVariable vars[] = {
    {"N", 3},
    {"M", 2},
    {"ten", 10},
    {"zero", 0},
    {"m", -1}
  };
  size_t nvars = sizeof(vars)/sizeof(InfixCalcVariable);

  printf("nvars=%zu\n", nvars);
  printf("sizeof(vars)=%zu\n", sizeof(vars));
  printf("sizeof(var)=%zu\n", sizeof(InfixCalcVariable));

  mu_assert_int_eq(4,  calc("2+2", NULL, 0));
  mu_assert_int_eq(1,  calc("2-1", NULL, 0));
  mu_assert_int_eq(4,  calc("2*2", NULL, 0));
  mu_assert_int_eq(3,  calc("6/2", NULL, 0));
  mu_assert_int_eq(2,  calc("5/2", NULL, 0));
  mu_assert_int_eq(0,  calc("6%2", NULL, 0));
  mu_assert_int_eq(1,  calc("5%2", NULL, 0));
  mu_assert_int_eq(4,  calc("2^2", NULL, 0));
  mu_assert_int_eq(8,  calc("2^3", NULL, 0));
  mu_assert_int_eq(9,  calc("3^2", NULL, 0));
  mu_assert_int_eq(4,  calc(" 2 + 2 ", NULL, 0));
  mu_assert_int_eq(14, calc("2 + 3 * 4", NULL, 0));
  mu_assert_int_eq(10, calc("2 * 3 + 4", NULL, 0));
  mu_assert_int_eq(14, calc("2 * (3 + 4)", NULL, 0));
  mu_assert_int_eq(14, calc("(3 + 4) * 2", NULL, 0));
  mu_assert_int_eq(20, calc("2 * ((3^2 + 4) - 3)", NULL, 0));
  mu_assert_int_eq(1,  calc("1", NULL, 0));

  mu_assert_int_eq(1,  calc("2 | 5", NULL, 0));
  mu_assert_int_eq(1,  calc("0 | 10", NULL, 0));
  mu_assert_int_eq(1,  calc("1 | 0", NULL, 0));
  mu_assert_int_eq(0,  calc("0 | 0", NULL, 0));
  mu_assert_int_eq(1,  calc("2 & 5", NULL, 0));
  mu_assert_int_eq(0,  calc("0 & 10", NULL, 0));
  mu_assert_int_eq(0,  calc("1 & 0", NULL, 0));
  mu_assert_int_eq(0,  calc("0 & 0", NULL, 0));
  mu_assert_int_eq(1,  calc("0 = 0", NULL, 0));
  mu_assert_int_eq(1,  calc("5 = 5", NULL, 0));
  mu_assert_int_eq(0,  calc("5 = 6", NULL, 0));
  mu_assert_int_eq(0,  calc("5 ! 5", NULL, 0));
  mu_assert_int_eq(1,  calc("5 ! 6", NULL, 0));
  mu_assert_int_eq(1,  calc("5 > 4", NULL, 0));
  mu_assert_int_eq(0,  calc("5 > 5", NULL, 0));
  mu_assert_int_eq(0,  calc("5 > 6", NULL, 0));
  mu_assert_int_eq(0,  calc("5 < 4", NULL, 0));
  mu_assert_int_eq(0,  calc("5 < 5", NULL, 0));
  mu_assert_int_eq(1,  calc("5 < 6", NULL, 0));
  mu_assert_int_eq(2,  calc("(2)", NULL, 0));

  /* Some failures */
  mu_assert_int_eq(INT_MIN, calc("-1", NULL, 0));  // unary operator
  mu_assert_int_eq(INT_MIN, calc("+1", NULL, 0));  // unary operator
  mu_assert_int_eq(INT_MIN, calc("1--1", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc("1+-1", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc("1==1", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc("+", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc("a", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc("5 / pi", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc("0.5", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc("1 1", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc("3 +", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc("3 + ( 4", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc("3 + )4 * 5)", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc("3 + 4) * 5", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc("( )", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc("(*)", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc("", NULL, 0));
  mu_assert_int_eq(INT_MIN, calc(" ", NULL, 0));

  /* Test variables */
  mu_assert_int_eq(0,  calc("zero", vars, nvars));
  mu_assert_int_eq(10, calc("ten", vars, nvars));
  mu_assert_int_eq(-1, calc("m", vars, nvars));
  mu_assert_int_eq(2,  calc("M", vars, nvars));
  mu_assert_int_eq(3,  calc("N", vars, nvars));
  mu_assert_int_eq(12, calc("2+ten", vars, nvars));
  mu_assert_int_eq(-3, calc("(N+zero)*m", vars, nvars));
  mu_assert_int_eq(3,  calc("N+zero*m", vars, nvars));
  mu_assert_int_eq(50, calc("ten*(M+N)", vars, nvars));
}

/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_infixcalc);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
