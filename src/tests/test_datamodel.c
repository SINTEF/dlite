#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "integers.h"
#include "boolean.h"
#include "dlite.h"
#include "dlite-datamodel.h"

#include "config.h"

char *datafile = "testdata.h5";
char *id = "testdata";
DLiteStorage *s=NULL;
DLiteDataModel *d=NULL, *d2=NULL, *d3=NULL;


/***************************************************************
 * Test storage
 ***************************************************************/

MU_TEST(test_open)
{
  double v=45.3;

  mu_check((s = dlite_storage_open("hdf5", datafile, "w")));
  mu_check((d = dlite_datamodel(s, id)));

  mu_check((d2 = dlite_datamodel(s, "4781deed-966b-528b-be3d-2ca7ab77aab0")));
  mu_check(dlite_datamodel_set_dimension_size(d2, "mydim", 10) == 0);
  mu_check(dlite_datamodel_set_property(d2, "x", &v, dliteFloat,
                                        sizeof(v), 1, NULL) == 0);

  mu_check((d3 = dlite_datamodel(s, "y")));
  mu_check(dlite_datamodel_set_property(d3, "y", &v, dliteFloat,
                                        sizeof(v), 1, NULL) == 0);
}

MU_TEST(test_close)
{
  mu_check(dlite_datamodel_free(d) == 0);
  mu_check(dlite_datamodel_free(d2) == 0);
  mu_check(dlite_datamodel_free(d3) == 0);
  mu_check(dlite_storage_close(s) == 0);
}

int compar(const void *a, const void *b)
{
  return strcmp(*(char **)a, *(char **)b);
}

MU_TEST(test_storage_uuids)
{
  char **uuids;
  int n=0;
  mu_check((uuids = dlite_storage_uuids(s)));
  while (uuids[n]) n++;
  mu_assert_int_eq(3, n);
#ifdef HAVE_QSORT
  qsort(uuids, n, sizeof(char *), compar);
  mu_assert_string_eq("4781deed-966b-528b-be3d-2ca7ab77aab0", uuids[0]);
  mu_assert_string_eq("9c96e6ac-51f4-5ad3-add1-5f6deffde30f", uuids[1]);
  mu_assert_string_eq("a839938d-1d30-5b2a-af5c-2a23d436abdc", uuids[2]);
#endif
  dlite_storage_uuids_free(uuids);
}

MU_TEST(test_is_writable)
{
  mu_check(dlite_storage_is_writable(s) != 0);
}

/***************************************************************
 * Test data model
 ***************************************************************/

MU_TEST(test_metadata)
{
  const char *p, *metadata="http://www.sintef.no/meta/dlite/0.1/testdata";
  mu_check(dlite_datamodel_set_metadata(d, metadata) == 0);
  mu_check((p = dlite_datamodel_get_metadata(d)));
  mu_assert_string_eq(metadata, p);
  free((char *)p);
}

MU_TEST(test_get_dataname)
{
  char *dataname;
  mu_check((dataname = dlite_datamodel_get_dataname(d)));
  mu_assert_string_eq(id, dataname);
  free(dataname);
}

MU_TEST(test_dimension_size)
{
  mu_check(dlite_datamodel_set_dimension_size(d, "N", 2) == 0);
  mu_check(dlite_datamodel_set_dimension_size(d, "M", 3) == 0);
  mu_assert_int_eq(2, dlite_datamodel_get_dimension_size(d, "N"));
  mu_assert_int_eq(3, dlite_datamodel_get_dimension_size(d, "M"));
}

MU_TEST(test_blob_property)
{
  int i;
  uint8_t v[17], w[17];
  for (i=0; i<17; i++) v[i] = i*2 + 1;
  mu_check(dlite_datamodel_set_property(d, "myblob", v, dliteBlob,
                                        sizeof(v), 1, NULL) == 0);
  mu_check(dlite_datamodel_get_property(d, "myblob", w, dliteBlob,
                                        sizeof(w), 1, NULL) == 0);
  for (i=0; i<17; i++)
    mu_check(v[i] == w[i]);
}

MU_TEST(test_bool_vec_property)
{
  int i, n=4;
  bool v[4]={1, 0, 0, 1}, w[4];
  mu_check(dlite_datamodel_set_property(d, "mybool", v, dliteBool,
                                        sizeof(bool), 1, &n) == 0);
  mu_check(dlite_datamodel_get_property(d, "mybool", w, dliteBool,
                                        sizeof(bool), 1, &n) == 0);
  for (i=0; i<4; i++)
    mu_check(v[i] == w[i]);
}

MU_TEST(test_int_arr_property)
{
  int i, j, v[2][3] = {{-4, 5, 7}, {42, 0, -13}}, w[2][3];
  int dims[] = {2, 3};
  mu_check(dlite_datamodel_set_property(d, "myint", v, dliteInt,
                                        sizeof(int), 2, dims) == 0);
  mu_check(dlite_datamodel_get_property(d, "myint", w, dliteInt,
                                        sizeof(int), 2, dims) == 0);
  for (i=0; i<dims[0]; i++)
    for (j=0; j<dims[1]; j++)
      mu_check(v[i][j] == w[i][j]);
}

MU_TEST(test_uint16_property)
{
  uint16_t v=42, w;
  mu_check(dlite_datamodel_set_property(d, "myuint16", &v, dliteUInt,
                                        sizeof(uint16_t), 1, NULL) == 0);
  mu_check(dlite_datamodel_get_property(d, "myuint16", &w, dliteUInt,
                                        sizeof(uint16_t), 1, NULL) == 0);
  mu_check(v == w);
}

MU_TEST(test_float_property)
{
  float v=3.1415, w;
  mu_check(dlite_datamodel_set_property(d, "myfloat", &v, dliteFloat,
                                        sizeof(float), 1, NULL) == 0);
  mu_check(dlite_datamodel_get_property(d, "myfloat", &w, dliteFloat,
                                        sizeof(float), 1, NULL) == 0);
  mu_check(v == w);
}

MU_TEST(test_double_property)
{
  double v=-1.2345e-6, w;
  mu_check(dlite_datamodel_set_property(d, "mydouble", &v, dliteFloat,
                                        sizeof(double), 1, NULL) == 0);
  mu_check(dlite_datamodel_get_property(d, "mydouble", &w, dliteFloat,
                                        sizeof(double), 1, NULL) == 0);
  mu_check(v == w);
}

MU_TEST(test_string_property)
{
  char v[]="A test string", w[256], *p;
  mu_check(dlite_datamodel_set_property(d, "mystring", &v, dliteString,
                                        sizeof(v), 1, NULL) == 0);
  mu_check(dlite_datamodel_get_property(d, "mystring", &w, dliteString,
                                        sizeof(w), 1, NULL) == 0);
  mu_assert_string_eq(v, w);
  /*
   * Also test implicit DTString to DTStringPtr */
  mu_check(dlite_datamodel_get_property(d, "mystring", &p, dliteStringPtr,
                                        sizeof(p), 1, NULL) == 0);
  mu_assert_string_eq(v, p);
  free(p);
}

MU_TEST(test_stringptr_vec_property)
{
  char *v[]={"Another test string", "next"}, *w[2], u[2][256];
  int i, dims[]={2};
  mu_check(dlite_datamodel_set_property(d, "mystringptr", v, dliteStringPtr,
                                        sizeof(char *), 1, dims) == 0);
  mu_check(dlite_datamodel_get_property(d, "mystringptr", w, dliteStringPtr,
                                        sizeof(char *), 1, dims) == 0);
  for (i=0; i<dims[0]; i++) {
    mu_assert_string_eq(v[i], w[i]);
    free(w[i]);
  }
  /*
   * Also test implicit DTStringPtr to DTString */
  mu_check(dlite_datamodel_get_property(d, "mystringptr", u, dliteString,
                                        256, 1, dims) == 0);
  for (i=0; i<dims[0]; i++)
    mu_assert_string_eq(v[i], u[i]);
}

MU_TEST(test_string_arr_property)
{
  int i, j, dims[] = {2, 2};
  char v[2][2][6]={{"this", "is"}, {"some", "words"}}, w[2][2][6];
  mu_check(dlite_datamodel_set_property(d, "mystring_arr", v, dliteString,
                                        6, 2, dims) == 0);
  mu_check(dlite_datamodel_get_property(d, "mystring_arr", w, dliteString,
                                        6, 2, dims) == 0);
  for (i=0; i<dims[0]; i++)
    for (j=0; j<dims[1]; j++)
      mu_assert_string_eq(v[i][j], w[i][j]);
}

MU_TEST(test_uint64_arr_property)
{
  int i, j, k, dims[] = {1, 2, 3};
  uint64_t v[1][2][3]={{{10, 12, 9}, {3, 0, 100}}}, w[6];
  mu_check(dlite_datamodel_set_property(d, "myuint64", v, dliteUInt,
                                        sizeof(uint64_t), 3, dims) == 0);
  mu_check(dlite_datamodel_get_property(d, "myuint64", w, dliteUInt,
                                        sizeof(uint64_t), 3, dims) == 0);
  for (i=0; i<dims[0]; i++)
    for (j=0; j<dims[1]; j++)
      for (k=0; k<dims[2]; k++)
        mu_check(v[i][j][k] == w[k + dims[2]*(j + dims[1]*i)]);
}

MU_TEST(test_has_dimension)
{
  mu_check(dlite_datamodel_has_dimension(d, "N") > 0);
  mu_check(dlite_datamodel_has_dimension(d, "xxx") == 0);
}

MU_TEST(test_has_property)
{
  mu_check(dlite_datamodel_has_property(d, "mystring") > 0);
  mu_check(dlite_datamodel_has_property(d, "xxx") == 0);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_open);  /* setup */
  MU_RUN_TEST(test_storage_uuids);
  MU_RUN_TEST(test_is_writable);

  /* data model tests */
  MU_RUN_TEST(test_metadata);
  MU_RUN_TEST(test_get_dataname);
  MU_RUN_TEST(test_dimension_size);
  MU_RUN_TEST(test_blob_property);
  MU_RUN_TEST(test_bool_vec_property);
  MU_RUN_TEST(test_int_arr_property);
MU_RUN_TEST(test_uint16_property);
  MU_RUN_TEST(test_float_property);
  MU_RUN_TEST(test_double_property);
  MU_RUN_TEST(test_string_property);
  MU_RUN_TEST(test_stringptr_vec_property);
  MU_RUN_TEST(test_string_arr_property);
  MU_RUN_TEST(test_uint64_arr_property);
  MU_RUN_TEST(test_has_dimension);
  MU_RUN_TEST(test_has_property);

  MU_RUN_TEST(test_close);  /* tear down */
}



int main(int argc, char *argv[])
{
  if (argc > 1) datafile = argv[1];
  if (argc > 2) id = argv[2];
  printf("datafile: '%s'\n", datafile);
  printf("id:       '%s'\n", id);

  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
