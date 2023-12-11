#include <assert.h>
#include <stdio.h>

#include "rng.h"

#define MINUNIT_PROGRESS 0
#include "minunit/minunit.h"


MU_TEST(test_rng)
{
  int i;
  MWCState state_mwc;
  MSWS32State state_msws32;
  MSWS64State state_msws64;

  srand_mwc(13);
  srand_mwc_r(&state_mwc, 13);

  srand_msws32(13);
  srand_msws32_r(&state_msws32, 13);

  srand_msws64(13);
  srand_msws64_r(&state_msws64, 13);

  for (i=0; i<10000; i++) {
    uint32_t a = rand_mwc();
    uint32_t b = rand_mwc_r(&state_mwc);
    mu_assert_int_eq(a, b);
    //printf("%u\n", a);
  }

  for (i=0; i<10000; i++) {
    uint32_t a = rand_msws32();
    uint32_t b = rand_msws32_r(&state_msws32);
    mu_assert_int_eq(a, b);
    //printf("%u\n", a);
  }

  for (i=0; i<10000; i++) {
    double a = drand_msws32();
    double b = drand_msws32_r(&state_msws32);
    mu_assert_double_eq(a, b);
    //printf("%u\n", a);
  }

  for (i=0; i<10000; i++) {
    uint64_t a = rand_msws64();
    uint64_t b = rand_msws64_r(&state_msws64);
    mu_check(a == b);
    //printf("%lu\n", a);
  }

  for (i=0; i<10000; i++) {
    double a = drand_msws64();
    double b = drand_msws64_r(&state_msws64);
    mu_assert_double_eq(a, b);
    //printf("%u\n", a);
  }
}




/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_rng);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
