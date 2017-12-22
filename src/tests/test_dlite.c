#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "integers.h"
#include "boolean.h"
#include "dlite.h"


char *datafile = "testdata.h5";
char *id = "testdata";

#define abs(x) (((x) < 0) ? -(x) : (x))


int test_open_truncate()
{
  DLite *d;
  int retval = 1;
  if (!(d = dopen("hdf5", datafile, "w", id))) goto fail;
  retval = 0;
 fail:
  dclose(d);
  return retval;
}

int test_metadata()
{
  DLite *d;
  const char *p=NULL, *metadata="http://www.sintef.no/meta/dlite/testdata/0.1";
  int retval = 1;
  if (!(d = dopen("hdf5", datafile, NULL, id))) goto fail;
  if (dset_metadata(d, metadata) < 0) goto fail;
  if (!(p = dget_metadata(d))) goto fail;
  if (strcmp(p, metadata)) goto fail;;
  retval = 0;
 fail:
  dclose(d);
  return retval;
}

int test_dataname()
{
  DLite *d;
  int retval = 1;
  char *dataname=NULL;
  if (!(d = dopen("hdf5", datafile, NULL, id))) goto fail;
  if (!(dataname = dget_dataname(d))) goto fail;
  if (strcmp(dataname, id)) goto fail;
  retval = 0;
 fail:
  dclose(d);
  if (dataname) free(dataname);
  return retval;
}
int test_dimension_size()
{
  DLite *d;
  int N, M, retval = 1;
  if (!(d = dopen("hdf5", datafile, NULL, id))) goto fail;
  if (dset_dimension_size(d, "N", 2)) goto fail;
  if (dset_dimension_size(d, "M", 3)) goto fail;
  if ((N = dget_dimension_size(d, "N")) < 0) goto fail;
  if ((M = dget_dimension_size(d, "M")) < 0) goto fail;
  retval = 0;
 fail:
  dclose(d);
  return retval;
}

int test_blob_property()
{
  DLite *d;
  int i, retval = 1;
  uint8_t v[17], w[17];
  for (i=0; i<17; i++) v[i] = i;
  if (!(d = dopen("hdf5", datafile, NULL, id))) goto fail;
  if (dset_property(d, "myblob", &v, DTBlob, sizeof(v), 1, NULL)) goto fail;
  if (dget_property(d, "myblob", &w, DTBlob, sizeof(w), 1, NULL)) goto fail;
  for (i=0; i<17; i++)
    if (w[i] != v[i]) goto fail;
  retval = 0;
 fail:
  dclose(d);
  return retval;
}

int test_bool_property()
{
  DLite *d;
  int retval = 1;
  bool v=1, w;
  if (!(d = dopen("hdf5", datafile, NULL, id))) goto fail;
  if (dset_property(d, "mybool", &v, DTBool, sizeof(v), 1, NULL)) goto fail;
  if (dget_property(d, "mybool", &w, DTBool, sizeof(w), 1, NULL)) goto fail;
  if (w != v) goto fail;
  retval = 0;
 fail:
  dclose(d);
  return retval;
}

int test_int_property()
{
  DLite *d;
  int retval = 1;
  int v=42, w;
  if (!(d = dopen("hdf5", datafile, NULL, id))) goto fail;
  if (dset_property(d, "myint", &v, DTInt, sizeof(v), 1, NULL)) goto fail;
  if (dget_property(d, "myint", &w, DTInt, sizeof(w), 1, NULL)) goto fail;
  if (w != v) goto fail;
  retval = 0;
 fail:
  dclose(d);
  return retval;
}

int test_uint16_property()
{
  DLite *d;
  int retval = 1;
  uint16_t v=44, w;
  if (!(d = dopen("hdf5", datafile, NULL, id))) goto fail;
  if (dset_property(d, "myuint16", &v, DTUInt, sizeof(v), 1, NULL)) goto fail;
  if (dget_property(d, "myuint16", &w, DTUInt, sizeof(w), 1, NULL)) goto fail;
  if (w != v) goto fail;
  retval = 0;
 fail:
  dclose(d);
  return retval;
}

int test_float_property()
{
  DLite *d;
  int retval = 1;
  float v=3.1415, w;
  if (!(d = dopen("hdf5", datafile, NULL, id))) goto fail;
  if (dset_property(d, "myshort", &v, DTFloat, sizeof(v), 1, NULL)) goto fail;
  if (dget_property(d, "myshort", &w, DTFloat, sizeof(w), 1, NULL)) goto fail;
  if (abs(w - v) > abs(v) * 1e-5) goto fail;
  retval = 0;
 fail:
  dclose(d);
  return retval;
}

int test_double_property()
{
  DLite *d;
  int retval = 1;
  double v=-1.2345e-6, w;
  if (!(d = dopen("hdf5", datafile, NULL, id))) goto fail;
  if (dset_property(d, "mydouble", &v, DTFloat, sizeof(v), 1, NULL)) goto fail;
  if (dget_property(d, "mydouble", &w, DTFloat, sizeof(w), 1, NULL)) goto fail;
  if (abs(w - v) > abs(v) * 1e-9) goto fail;
  retval = 0;
 fail:
  dclose(d);
  return retval;
}

int test_string_property()
{
  DLite *d;
  int retval = 1;
  char v[]="A test string", w[256], *p=NULL;
  if (!(d = dopen("hdf5", datafile, NULL, id))) goto fail;
  if (dset_property(d, "mystring", v, DTString, sizeof(v), 1, NULL)) goto fail;
  if (dget_property(d, "mystring", w, DTString, sizeof(w), 1, NULL)) goto fail;
  if (strcmp(v, w)) goto fail;
  /*
   * Also test implicit DTString to DTStringPtr */
  if (dget_property(d, "mystring", &p, DTStringPtr, sizeof(p), 1, NULL)) goto fail;
  if (strcmp(v, p)) goto fail;
  /* Done */
  retval = 0;
 fail:
  dclose(d);
  if (p) free(p);
  return retval;
}

int test_stringptr_property()
{
  DLite *d;
  int retval = 1;
  char *v="Another test string", *w=NULL, u[256];
  if (!(d = dopen("hdf5", datafile, NULL, id))) goto fail;
  if (dset_property(d, "mystr", &v, DTStringPtr, sizeof(v), 1, NULL)) goto fail;
  if (dget_property(d, "mystr", &w, DTStringPtr, sizeof(w), 1, NULL)) goto fail;
  if (strcmp(v, w)) goto fail;
  /*
   * Also test implicit DTStringPtr to DTString */
  if (dget_property(d, "mystr", u, DTString, sizeof(u), 1, NULL)) goto fail;
  if (strcmp(v, u)) goto fail;
  /* Done */
  retval = 0;
 fail:
  dclose(d);
  if (w) free(w);
  return retval;
}

int test_uint64_arr_property()
{
  DLite *d;
  int retval = 1;
  int i, j, k, dims[] = {1, 2, 3};
  uint64_t v[1][2][3]={{{10, 12, 9}, {3, 0, 100}}}, w[6];
  if (!(d = dopen("hdf5", datafile, NULL, id))) goto fail;
  if (dset_property(d, "myuint64_arr", v, DTUInt, 8, 3, dims)) goto fail;
  if (dget_property(d, "myuint64_arr", w, DTUInt, 8, 3, dims)) goto fail;
  for (i=0; i<dims[0]; i++)
    for (j=0; j<dims[1]; j++)
      for (k=0; k<dims[2]; k++)
        if (w[k + dims[2]*(j + dims[1]*i)] != v[i][j][k]) goto fail;
  /* Done */
  retval = 0;
 fail:
  dclose(d);
  return retval;
}

int test_string_arr_property()
{
  DLite *d;
  int retval = 1;
  int i, j, dims[] = {2, 2};
  char v[2][2][6]={{"this", "is"}, {"some", "words"}}, w[2][2][6];
  char *u[2][2]={{NULL}};
  if (!(d = dopen("hdf5", datafile, NULL, id))) goto fail;
  if (dset_property(d, "mystring_arr", v, DTString, 6, 2, dims)) goto fail;
  if (dget_property(d, "mystring_arr", w, DTString, 6, 2, dims)) goto fail;
  for (i=0; i<dims[0]; i++)
    for (j=0; j<dims[1]; j++)
      if (strcmp(w[i][j], v[i][j])) goto fail;
  /*
   * Also test implicit DTString to DTStringPtr */
  if (dget_property(d, "mystring_arr", u, DTStringPtr, sizeof(char *),
                    2, dims)) goto fail;
  for (i=0; i<dims[0]; i++)
    for (j=0; j<dims[1]; j++)
      if (strcmp(u[i][j], v[i][j])) goto fail;
  /* Done */
  retval = 0;
 fail:
  dclose(d);
  if (u[0][0])
    for (i=0; i<dims[0]; i++)
      for (j=0; j<dims[1]; j++)
        free(u[i][j]);
  return retval;
}

int test_stringptr_arr_property()
{
  DLite *d;
  int retval = 1;
  int i, j, dims[] = {2, 2};
  char *v[2][2]={{"this", "is"}, {"some", "words"}}, *w[2][2]={{NULL}};
  char u[2][2][10];
  if (!(d = dopen("hdf5", datafile, NULL, id))) goto fail;
  if (dset_property(d, "mystringptr_arr", v, DTStringPtr, sizeof(char *),
                    2, dims)) goto fail;
  if (dget_property(d, "mystringptr_arr", w, DTStringPtr, sizeof(char *),
                    2, dims)) goto fail;
  for (i=0; i<dims[0]; i++)
    for (j=0; j<dims[1]; j++)
      if (strcmp(w[i][j], v[i][j])) goto fail;
  /*
   * Also test implicit DTStringPtr to DTString */
  if (dget_property(d, "mystringptr_arr", u, DTString, 10, 2, dims)) goto fail;
  for (i=0; i<dims[0]; i++)
    for (j=0; j<dims[1]; j++)
      if (strcmp(u[i][j], v[i][j])) goto fail;
  /* Done */
  retval = 0;
 fail:
  dclose(d);
  if(w[0][0])
    for (i=0; i<dims[0]; i++)
      for (j=0; j<dims[1]; j++)
        free(w[i][j]);
  return retval;
}


/***********************************************************************/

int nerr = 0;  /* global error count */

#define Assert(name, cond)                    \
  do {                                        \
    if (cond) {                               \
      printf("%-28s OK\n", (name));           \
    } else {                                  \
      nerr++;                                 \
      printf("%-28s Failed\n", (name));       \
    }                                         \
  } while (0)


int main(int argc, char *argv[]) {
  char **names=NULL;
  if (argc > 1) datafile = argv[1];
  if (argc > 2) id = argv[2];

  if (!id) {
    names = dget_instance_names("hdf5", datafile, NULL);
    id = names[0];
  }
  printf("datafile: '%s'\n", datafile);
  printf("id:       '%s'\n", id);

  Assert("open_truncate",          test_open_truncate() == 0);
  Assert("dataname",               test_dataname() == 0);
  Assert("metadata",               test_metadata() == 0);
  Assert("dimension_size",         test_dimension_size() == 0);
  Assert("blob_property",          test_blob_property() == 0);
  Assert("bool_property",          test_bool_property() == 0);
  Assert("int_property",           test_int_property() == 0);
  Assert("uint16_property",        test_uint16_property() == 0);
  Assert("float_property",         test_float_property() == 0);
  Assert("double_property",        test_double_property() == 0);
  Assert("string_property",        test_string_property() == 0);
  Assert("stringptr_property",     test_stringptr_property() == 0);
  Assert("uint64_arr_property",    test_uint64_arr_property() == 0);
  Assert("string_arr_property",    test_string_arr_property() == 0);
  Assert("stringptr_arr_property", test_stringptr_arr_property() == 0);

  dfree_instance_names(names);
  return nerr;
}
