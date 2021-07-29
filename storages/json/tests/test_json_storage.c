#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "utils/integers.h"
#include "utils/boolean.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-datamodel.h"

#include "config.h"



/***************************************************************
 * Test JSON storage
 ***************************************************************/

MU_TEST(test_read)
{
  char *dbname = STRINGIFY(DLITE_ROOT) "/src/tests/test-read-data.json";
  DLiteStorage *db = NULL;
  char **ids;
  size_t i, n;

  db = dlite_storage_open("json", dbname, "mode=r");
  mu_check(db);

  printf("\n");
  ids = dlite_storage_uuids(db, NULL);
  n = 0;
  for(i=0; ids[i]; i++) {
    printf("%d: %s\n", (int)i, ids[i]);
    n++;
  }
  /*printf("n=%d\n", n);*/
  mu_assert_int_eq(5, n);
  dlite_storage_uuids_free(ids);

  mu_check(dlite_storage_close(db) == 0);
}

MU_TEST(test_write)
{
  DLiteStorage *s=NULL;

  mu_check((s = dlite_storage_open("json", "test-json-write.json", "mode=w")));

  dlite_storage_close(s);
}

/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_read);
  MU_RUN_TEST(test_write);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
