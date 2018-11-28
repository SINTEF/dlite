#include <stdio.h>

#include "config.h"

#include "minunit/minunit.h"
#include "dlite.h"
#include "dlite-arrays.h"

int data[12];
DLiteArray *arr;



MU_TEST(test_array_create)
{
  size_t i;
  int ndims=2, dims[]={3,4};
  for (i=0; i<sizeof(data); i++) data[i] = i;
  mu_check((arr = dlite_array_create(data, dliteInt, sizeof(int),
				     ndims, dims)));
}


MU_TEST(test_array_size)
{
  mu_assert_int_eq(12*sizeof(int), dlite_array_size(arr));
}


MU_TEST(test_array_index)
{
  int *p, ind[]={2,1};
  mu_check((p = dlite_array_index(arr, ind)));
  mu_assert_int_eq(9, *p);

  mu_check((p = dlite_array_vindex(arr, 2, 1)));
  mu_assert_int_eq(9, *p);

  mu_check((p = dlite_array_vindex(arr, 0, 0)));
  mu_assert_int_eq(0, *p);
}


MU_TEST(test_array_iter)
{
  DLiteArrayIter iter;
  int *p, n=0, sum=0, sum0=0, sum1=0;
  mu_check(0 == dlite_array_iter_init(&iter, arr));
  //printf("\n");
  while ((p = dlite_array_iter_next(&iter))) {
    //printf("  %2d : %d, %d\n", *p, iter.ind[0], iter.ind[1]);
    n++;
    sum += *p;
    sum0 += iter.ind[0];
    sum1 += iter.ind[1];
  }
  mu_assert_int_eq(12, n);
  mu_assert_int_eq(66, sum);
  mu_assert_int_eq(12, sum0);
  mu_assert_int_eq(18, sum1);

  dlite_array_iter_deinit(&iter);
}


MU_TEST(test_array_slice)
{
  int *start=NULL;
  int *stop=NULL;
  int *step=NULL;
  DLiteArray *a = dlite_array_slice(arr, start, stop, step);

  printf("\narr:\n");
  dlite_array_printf(stdout, arr);

  printf("\n\na:\n");
  dlite_array_printf(stdout, a);
  printf("\n");

  mu_assert_int_eq(1, dlite_array_compare(a, arr));

  dlite_array_free(a);
}


MU_TEST(test_array_free)
{
  dlite_array_free(arr);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_array_create);     /* setup */
  MU_RUN_TEST(test_array_size);
  MU_RUN_TEST(test_array_index);
  MU_RUN_TEST(test_array_iter);
  MU_RUN_TEST(test_array_slice);
  MU_RUN_TEST(test_array_free);      /* tear down */
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
