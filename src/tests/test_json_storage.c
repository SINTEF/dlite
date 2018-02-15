#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "integers.h"
#include "boolean.h"
#include "dlite.h"
#include "dlite-datamodel.h"

#include "str.h"
#include "vector.h"

#include "config.h"


/***************************************************************
 * Test JSON storage
 ***************************************************************/

MU_TEST(test_read)
{
  char *dbname = "../../../src/tests/test-read-data.json";
  DLiteStorage *db = NULL;
  DLiteDataModel *d = NULL;
  char *s;
  size_t dim;
  int p1;
  int8_t p1_i8;
  vec_t *p2;

  db = dlite_storage_open("json", dbname, "r");
  mu_check(db);

  char **ids = dlite_storage_uuids(db);
  size_t i, n;
  n = 0;
  for(i=0; ids[i]; i++) {
    printf("%d: %s\n", i, ids[i]);
    n++;
  }
  /*printf("n=%d\n", n);*/
  mu_check(n == 4);

  d = dlite_datamodel(db, "unknown");
  mu_check(d == NULL);
/*
  d = dlite_datamodel(db, "empty");
  mu_check(d);
  mu_check(dlite_datamodel_free(d) == 0);
*/
  d = dlite_datamodel(db, "4781deed-966b-528b-be3d-2ca7ab77aab0");
  mu_check(d);
  s = dlite_datamodel_get_meta_uri(d);
  mu_check(str_equal(s, "dlite/1/A"));

  mu_check(dlite_datamodel_has_dimension(d, "N"));
  dim = dlite_datamodel_get_dimension_size(d, "N");
  mu_check(dim == 5);
  mu_check(!dlite_datamodel_has_dimension(d, "M"));
  dim = dlite_datamodel_get_dimension_size(d, "M");
  mu_check(dim == 0);

  mu_check(dlite_datamodel_has_property(d, "P1"));
  mu_check(dlite_datamodel_has_property(d, "P2"));
  mu_check(!dlite_datamodel_has_property(d, "P3"));

  dlite_datamodel_get_property(d, "P1", (void*)(&p1), dliteInt,
                               sizeof(int), 0, NULL);
  mu_check(p1 == 24);
  dlite_datamodel_get_property(d, "P1", (void*)(&p1_i8), dliteInt,
                               1, 0, NULL);
  mu_check(p1_i8 == 24);

  dim = dlite_datamodel_get_dimension_size(d, "N");
  p2 = vecn(dim, 0.0);
  dlite_datamodel_get_property(d, "P2", (void*)(p2->data), dliteFloat,
                               sizeof(double), 1, &dim);
  mu_check(p2->data[0] == 1.0);
  mu_check(p2->data[1] == 2.0);
  mu_check(p2->data[4] == 5.5);
  vec_free(p2);

  mu_check(dlite_datamodel_free(d) == 0);

  mu_check(dlite_storage_close(db) == 0);
}

MU_TEST(test_write)
{
  DLiteStorage *s=NULL;
  DLiteDataModel *d;

  double v = 45.3;
  int i = 11;
  vec_t *ar;
  ivec_t *ai, *dim1, *dim2;
  size_t dims[3] = {0, 0, 0};

  mu_check((s = dlite_storage_open("json", "test-json-write.json", "w")));

  mu_check((d = dlite_datamodel(s, NULL)));
  mu_check(dlite_datamodel_set_meta_uri(d, "dlite/1.0/xx") == 0);
  mu_check(dlite_datamodel_set_property(d, "x", &v, dliteFloat,
                                        sizeof(double), 0, NULL) == 0);
  mu_check(dlite_datamodel_set_property(d, "y", &i, dliteInt,
                                        sizeof(int), 0, NULL) == 0);
  mu_check(dlite_datamodel_free(d) == 0);

  dim1 = ivec1(5);
  ar = vecn(ivec_cumprod(dim1), 23.0);
  ar->data[2] = -2.0;

  dim2 = ivec2(3, 4);
  ai = ivecn(ivec_cumprod(dim2), 22);
  ai->data[2] = -2;

  mu_check((d = dlite_datamodel(s, NULL)));
  mu_check(dlite_datamodel_set_meta_uri(d, "dlite/1.0/yy") == 0);

  ivec_copy_cast(dim1, dliteUInt, sizeof(size_t), &dims[0]);
  mu_check(dlite_datamodel_set_dimension_size(d, "a", dims[0]) == 0);
  mu_check(dlite_datamodel_set_property(d, "x", ar->data, dliteFloat,
                                        sizeof(double), 1, &dims[0]) == 0);

  ivec_copy_cast(dim2, dliteUInt, sizeof(size_t), &dims[0]);
  mu_check(dlite_datamodel_set_dimension_size(d, "b", dims[0]) == 0);
  mu_check(dlite_datamodel_set_dimension_size(d, "c", dims[1]) == 0);
  mu_check(dlite_datamodel_set_property(d, "y", ai->data, dliteInt,
                                        sizeof(int), 2, &dims[0]) == 0);
  mu_check(dlite_datamodel_free(d) == 0);

  vec_free(ar);
  ivec_free(ai);
  ivec_free(dim1);
  ivec_free(dim2);

  dlite_storage_close(s);
}

/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_read);

  MU_RUN_TEST(test_write);
}



int main(int argc, char *argv[])
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
