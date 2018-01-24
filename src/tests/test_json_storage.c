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

char *datafile = "db.json";
char *id = "testdata";
DLiteStorage *s=NULL;
DLiteDataModel *d=NULL, *d2=NULL, *d3=NULL;


/***************************************************************
 * Test storage
 ***************************************************************/

MU_TEST(test_open)
{
  double v=45.3;

  mu_check((s = dlite_storage_open("json", datafile, "r")));
  mu_check((d = dlite_datamodel(s, id)));

}

MU_TEST(test_close)
{
  mu_check(dlite_datamodel_free(d) == 0);
  mu_check(dlite_datamodel_free(d2) == 0);
  mu_check(dlite_datamodel_free(d3) == 0);
  mu_check(dlite_storage_close(s) == 0);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_open);  /* setup */

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
