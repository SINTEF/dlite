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

char *datafile = "testdata.json";
char *id = "testdata";
DLiteStorage *s=NULL;
DLiteStorage *db=NULL;
DLiteDataModel *d1=NULL, *d2=NULL, *d3=NULL;


/***************************************************************
 * Test storage
 ***************************************************************/

MU_TEST(test_open)
{
  double v=45.3;
  mu_check((s = dlite_storage_open("json", datafile, "w")));
  /*
  mu_check((d1 = dlite_datamodel(s, id)));

  mu_check((d2 = dlite_datamodel(s, "4781deed-966b-528b-be3d-2ca7ab77aab0")));
  mu_check(dlite_datamodel_set_dimension_size(d2, "mydim", 10) == 0);
  mu_check(dlite_datamodel_set_property(d2, "x", &v, dliteFloat,
                                        sizeof(v), 1, NULL) == 0);

  mu_check((d3 = dlite_datamodel(s, "y")));
  mu_check(dlite_datamodel_set_property(d3, "y", &v, dliteFloat,
                                        sizeof(v), 1, NULL) == 0);
  */
  mu_check((db = dlite_storage_open("json", "test-read-data.json", "r")));

}

MU_TEST(test_uuids)
{
  char **ids = dlite_storage_uuids(db);
  size_t i, n;
  n = 0;
  for(i=0; ids[i]; i++) {
    /*printf("%d: %s\n", i, ids[i]);*/
    n++;
  }
  /*printf("n=%d\n", n);*/
  mu_check(n == 4);
}

MU_TEST(test_close)
{
  /*
  mu_check(dlite_datamodel_free(d1) == 0);
  mu_check(dlite_datamodel_free(d2) == 0);
  mu_check(dlite_datamodel_free(d3) == 0);
  */
  mu_check(dlite_storage_close(s) == 0);
  mu_check(dlite_storage_close(db) == 0);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_open);  /* setup */

  MU_RUN_TEST(test_uuids);

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
