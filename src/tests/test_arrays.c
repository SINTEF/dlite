#include <stdio.h>

#include "config.h"

#include "minunit/minunit.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-arrays.h"

int data[12];
DLiteArray *arr;



MU_TEST(test_array_create)
{
  size_t i;
  size_t ndims=2, dims[]={3, 4};
  for (i=0; i<countof(data); i++) data[i] = i;
  mu_check((arr = dlite_array_create(data, dliteInt, sizeof(int),
				     ndims, dims)));
  printf("\n");
  dlite_array_printf(stdout, arr, 2, 0);
  printf("ndims:   %d\n", arr->ndims);
  printf("dims:    [%2lu, %2lu]\n", arr->dims[0], arr->dims[1]);
  printf("strides: [%2d, %2d]\n", arr->strides[0], arr->strides[1]);
  printf("\n");
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
  int start[]={0, 1};
  int stop[]={3, -1};
  int step[]={1, 2};
  DLiteArray *a;

  printf("\n");
  a = dlite_array_slice(arr, NULL, NULL, NULL);
  mu_assert_int_eq(1, dlite_array_compare(a, arr));
  mu_check(dlite_array_is_continuous(a));
  dlite_array_free(a);

  printf("\n");
  a = dlite_array_slice(arr, start, stop, NULL);
  mu_assert_int_eq(3, a->dims[0]);
  mu_assert_int_eq(2, a->dims[1]);
  mu_assert_int_eq(16, a->strides[0]);
  mu_assert_int_eq(4, a->strides[1]);
  mu_assert_int_eq(1, *((int *)dlite_array_vindex(a, 0, 0)));
  mu_check(!dlite_array_is_continuous(a));
  dlite_array_free(a);

  printf("\n");
  a = dlite_array_slice(arr, NULL, NULL, step);
  mu_assert_int_eq(3, a->dims[0]);
  mu_assert_int_eq(2, a->dims[1]);
  mu_assert_int_eq(16, a->strides[0]);
  mu_assert_int_eq(8, a->strides[1]);
  mu_assert_int_eq(0, *((int *)dlite_array_vindex(a, 0, 0)));
  mu_check(!dlite_array_is_continuous(a));
  dlite_array_free(a);

  printf("\n");
  a = dlite_array_slice(arr, start, NULL, step);
  mu_assert_int_eq(3, a->dims[0]);
  mu_assert_int_eq(2, a->dims[1]);
  mu_assert_int_eq(16, a->strides[0]);
  mu_assert_int_eq(8, a->strides[1]);
  mu_assert_int_eq(1, *((int *)dlite_array_vindex(a, 0, 0)));
  mu_check(!dlite_array_is_continuous(a));
  dlite_array_free(a);

  printf("\n");
  a = dlite_array_slice(arr, NULL, stop, step);
  mu_assert_int_eq(3, a->dims[0]);
  mu_assert_int_eq(2, a->dims[1]);
  mu_assert_int_eq(16, a->strides[0]);
  mu_assert_int_eq(8, a->strides[1]);
  mu_assert_int_eq(0, *((int *)dlite_array_vindex(a, 0, 0)));
  mu_check(!dlite_array_is_continuous(a));
  dlite_array_free(a);

  step[1] = -1;

  printf("\n");
  a = dlite_array_slice(arr, NULL, NULL, step);
  mu_assert_int_eq(3, a->dims[0]);
  mu_assert_int_eq(4, a->dims[1]);
  mu_assert_int_eq(16, a->strides[0]);
  mu_assert_int_eq(-4, a->strides[1]);
  mu_assert_int_eq(3, *((int *)dlite_array_vindex(a, 0, 0)));
  mu_check(!dlite_array_is_continuous(a));
  dlite_array_free(a);

  start[1] = 4;

  printf("\n");
  a = dlite_array_slice(arr, start, NULL, step);
  mu_assert_int_eq(3, a->dims[0]);
  mu_assert_int_eq(4, a->dims[1]);
  mu_assert_int_eq(16, a->strides[0]);
  mu_assert_int_eq(-4, a->strides[1]);
  mu_assert_int_eq(3, *((int *)dlite_array_vindex(a, 0, 0)));
  mu_check(!dlite_array_is_continuous(a));
  dlite_array_free(a);

  start[1] = -1;
  stop[1] = 0;
  step[0] = 2;
  printf("\n");
  printf("start: [%d %d]\n", start[0], start[1]);
  printf("stop:  [%d %d]\n", stop[0], stop[1]);
  printf("step:  [%d %d]\n", step[0], step[1]);

  printf("\n");
  a = dlite_array_slice(arr, start, stop, step);

  dlite_array_printf(stdout, a, 2, 0);
  printf("ndims:   %d\n", a->ndims);
  printf("dims:    [%2lu, %2lu]\n", a->dims[0], a->dims[1]);
  printf("strides: [%2d, %2d]\n", a->strides[0], a->strides[1]);
  printf("\n");


  mu_assert_int_eq(2, a->dims[0]);
  mu_assert_int_eq(3, a->dims[1]);
  mu_assert_int_eq(32, a->strides[0]);
  mu_assert_int_eq(-4, a->strides[1]);
  mu_assert_int_eq(2, *((int *)dlite_array_vindex(a, 0, 0)));
  mu_check(!dlite_array_is_continuous(a));
  dlite_array_free(a);
}


MU_TEST(test_array_reshape)
{
  int ndims = 4;
  size_t dims[] = {2, 1, 3, 2};
  DLiteArray *a;
  mu_check((a = dlite_array_reshape(arr, ndims, dims)));
  mu_check(0 != dlite_array_is_continuous(a));
  //printf("\nreshaped:\n");
  //dlite_array_printf(stdout, a, -1, -1);
  dlite_array_free(a);
}


MU_TEST(test_array_transpose)
{
  DLiteArray *a;
  void *p;
  mu_check((a = dlite_array_transpose(arr)));
  mu_check(0 == dlite_array_is_continuous(a));
  //printf("\narr:\n");
  //dlite_array_printf(stdout, arr, -1, -1);
  //printf("\ntransposed:\n");
  //dlite_array_printf(stdout, a, -1, -1);
  mu_assert_int_eq(0, *((int *)dlite_array_vindex(a, 0, 0)));
  mu_assert_int_eq(1, *((int *)dlite_array_vindex(a, 1, 0)));
  mu_assert_int_eq(4, *((int *)dlite_array_vindex(a, 0, 1)));
  mu_assert_int_eq(9, *((int *)dlite_array_vindex(a, 1, 2)));

  mu_check((p = dlite_array_make_continuous(a)));
  mu_check(0 != dlite_array_is_continuous(a));
  //printf("\narr:\n");
  //dlite_array_printf(stdout, arr, -1, -1);
  //printf("\ntransposed:\n");
  //dlite_array_printf(stdout, a, -1, -1);
  mu_assert_int_eq(0, *((int *)dlite_array_vindex(a, 0, 0)));
  mu_assert_int_eq(1, *((int *)dlite_array_vindex(a, 1, 0)));
  mu_assert_int_eq(4, *((int *)dlite_array_vindex(a, 0, 1)));
  mu_assert_int_eq(9, *((int *)dlite_array_vindex(a, 1, 2)));

  free(p);
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
  MU_RUN_TEST(test_array_reshape);
  MU_RUN_TEST(test_array_transpose);
  MU_RUN_TEST(test_array_free);      /* tear down */
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
