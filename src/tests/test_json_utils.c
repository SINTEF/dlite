#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "integers.h"
#include "boolean.h"
#include "json-utils.h"

#include "config.h"


MU_TEST(test_array)
{
  json_error_t error;
  json_t *root = json_load_file("/home/tco/Documents/precims/dlite/src/tests/array.json", 0, &error);
  mu_check(json_char_type(root) == 'o');
  mu_check(json_array_type(json_object_get(root, "i1")) == 'i');
  mu_check(json_array_type(json_object_get(root, "i2")) == 'i');

  mu_check(json_array_type(json_object_get(root, "s1")) == 's');

  mu_check(json_array_type(json_object_get(root, "r1")) == 'm');
  mu_check(json_array_type(json_object_get(root, "r2")) == 'r');
  mu_check(json_array_type(json_object_get(root, "r3")) == 'r');
  mu_check(json_array_type(json_object_get(root, "r4")) == 'r');
  mu_check(json_array_type(json_object_get(root, "r5")) == 'x');

  mu_check(json_array_type(json_object_get(root, "o1")) == 'o');

  int ndim = -1;
  int dims[10];

  json_array_dimensions(json_object_get(root, "r4"), &ndim, &dims);
  mu_check(ndim == 2);
  mu_check(dims[0] == 3);
  mu_check(dims[1] == 3);

  json_array_dimensions(json_object_get(root, "i1"), &ndim, &dims);
  mu_check(ndim == 1);
  mu_check(dims[0] == 9);

  json_array_dimensions(json_object_get(root, "i2"), &ndim, &dims);
  mu_check(ndim == 2);
  mu_check(dims[0] == 5);
  mu_check(dims[1] == 3);

  json_array_dimensions(json_object_get(root, "i3"), &ndim, &dims);
  mu_check(ndim == -1);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_array);
}



int main(int argc, char *argv[])
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
