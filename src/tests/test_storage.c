#include <stdlib.h>
#include <stdio.h>

#include "minunit/minunit.h"

#include "dlite-macros.h"
#include "dlite.h"

DLiteStorage *s=NULL;


MU_TEST(test_open)
{
  char *path = STRINGIFY(DLITE_ROOT) "/src/tests/test-data.json";
  mu_check((s = dlite_storage_open("json", path, NULL)));
  mu_assert_int_eq(1, dlite_storage_is_writable(s));
}

MU_TEST(test_open_url)
{
  char *url =
    "json://" STRINGIFY(DLITE_ROOT) "/src/tests/test-data.json?mode=r";
  mu_assert_int_eq(0, dlite_storage_close(s));
  mu_check((s = dlite_storage_open_url(url)));
  mu_assert_int_eq(0, dlite_storage_is_writable(s));
}

MU_TEST(test_idflag)
{
  mu_assert_int_eq(0, dlite_storage_get_idflag(s));
  dlite_storage_set_idflag(s, dliteIDKeepID);
  mu_assert_int_eq(dliteIDKeepID, dlite_storage_get_idflag(s));
}

MU_TEST(test_uuids)
{
  char **q, **uuids;
  mu_check((uuids = dlite_storage_uuids(s)));
  printf("\nUUIDs:\n");
  for (q=uuids; *q; q++) printf("  %s\n", *q);
  printf("\n");
  dlite_storage_uuids_free(uuids);
}

MU_TEST(test_get_driver)
{
  const char *driver = dlite_storage_get_driver(s);
  mu_assert_string_eq("json", driver);
}

MU_TEST(test_close)
{
  mu_assert_int_eq(0, dlite_storage_close(s));
}




/***********************************************************************/


MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_open);   /* setup */

  MU_RUN_TEST(test_open_url);
  MU_RUN_TEST(test_idflag);
  MU_RUN_TEST(test_uuids);
  MU_RUN_TEST(test_get_driver);

  MU_RUN_TEST(test_close);  /* teardown */
}

int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
