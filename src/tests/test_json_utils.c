#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "integers.h"
#include "boolean.h"
#include "json-utils.h"

#include "config.h"


MU_TEST(test_vector)
{
  ivec_t *v = ivec();
  ivec_add(v, 1);
  ivec_add(v, 2);
  ivec_add(v, 3);
  mu_check(ivec_size(v) == 3);
  mu_check(v->capacity == 10);
  mu_check(v->data[0] == 1);
  mu_check(v->data[1] == 2);
  mu_check(v->data[2] == 3);
  ivec_fill(v, 5);
  mu_check(v->data[0] == 5);
  mu_check(v->data[1] == 5);
  mu_check(v->data[2] == 5);
  ivec_free(v);
}


MU_TEST(test_json_array)
{
  json_error_t error;
  ivec_t *dims = NULL;
  json_data_t *data = NULL;

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

  dims = json_array_dimensions(json_object_get(root, "r4"));
  mu_check(dims->size == 2);
  mu_check(dims->data[0] == 3);
  mu_check(dims->data[1] == 3);
  ivec_free(dims);

  dims = json_array_dimensions(json_object_get(root, "i1"));
  mu_check(dims->size == 1);
  mu_check(dims->data[0] == 9);
  ivec_free(dims);

  dims = json_array_dimensions(json_object_get(root, "i2"));
  mu_check(dims->size == 2);
  mu_check(dims->data[0] == 5);
  mu_check(dims->data[1] == 3);
  ivec_free(dims);

  dims = json_array_dimensions(json_object_get(root, "i3"));
  mu_check(dims == NULL);

  data = json_get_data(json_object_get(root, "i1"));
  /*ivec_print(data->array_i, "i1");*/
  mu_check(data->dtype == 'i');
  mu_check(data->array_i->size == 9);
  mu_check(data->array_i->data[0] == 1);
  mu_check(data->array_i->data[2] == 3);
  mu_check(data->array_i->data[3] == 1);
  json_data_free(data);

  data = json_get_data(json_object_get(root, "i2"));
  /*ivec_print(data->array_i, "i2");*/
  mu_check(data->dtype == 'i');
  mu_check(data->array_i->size == 15);
  json_data_free(data);

  data = json_get_data(json_object_get(root, "r4"));
  /*vec_print(data->array_r, "r4");*/
  mu_check(data->dtype == 'r');
  mu_check(data->array_r->size == 9);
  json_data_free(data);

  data = json_get_data(json_object_get(root, "v-int"));
  mu_check(data->dtype == 'i');
  mu_check(data->dims == NULL);
  mu_check(data->scalar_i == 1);
  json_data_free(data);

  data = json_get_data(json_object_get(root, "v-real"));
  mu_check(data->dtype == 'r');
  mu_check(data->dims == NULL);
  mu_check(data->scalar_r == 2.0);
  json_data_free(data);

  data = json_get_data(json_object_get(root, "v-true"));
  mu_check(data->dtype == 'b');
  mu_check(data->dims == NULL);
  mu_check(data->scalar_i == 1);
  json_data_free(data);

  data = json_get_data(json_object_get(root, "v-false"));
  mu_check(data->dtype == 'b');
  mu_check(data->dims == NULL);
  mu_check(data->scalar_i == 0);
  json_data_free(data);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_vector);
  MU_RUN_TEST(test_json_array);
}



int main(int argc, char *argv[])
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
