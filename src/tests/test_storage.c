#include <stdlib.h>
#include <stdio.h>

#include "minunit/minunit.h"

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-storage-plugins.h"

DLiteStorage *s=NULL;


MU_TEST(test_open)
{
  char *path = STRINGIFY(dlite_SOURCE_DIR) "/src/tests/test-data.json";
  mu_check((s = dlite_storage_open("json", path, NULL)));
  mu_assert_int_eq(1, dlite_storage_is_writable(s));
}

MU_TEST(test_open_url)
{
  char *url =
    "json://" STRINGIFY(dlite_SOURCE_DIR) "/src/tests/test-data.json?mode=r";  // cppcheck-suppress unknownMacro
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
  mu_check((uuids = dlite_storage_uuids(s, NULL)));
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

MU_TEST(test_storage_iter)
{
  /* Iterate over UUIDs of all instances in the storage */
  char uuid[DLITE_UUID_LENGTH+1];
  void *iter = dlite_storage_iter_create(s, NULL);
  int n = 0;
  printf("\nUUIDs (iterated over):\n");
  while (dlite_storage_iter_next(s, iter, uuid) == 0) {
    printf("  %s\n", uuid);
    n++;
  }
  printf("\n");
  dlite_storage_iter_free(s, iter);
  mu_assert_int_eq(2, n);
}

MU_TEST(test_storage_iter_pattern)
{
  /* Iterate over UUIDs of instances who's metadata matches pattern */
  char uuid[DLITE_UUID_LENGTH+1];
  void *iter = dlite_storage_iter_create(s, "http://onto-ns.com/meta/0.1/test-entity");
  int n = 0;
  printf("\nUUIDs (pattern iteration, ok):\n");
  while (dlite_storage_iter_next(s, iter, uuid) == 0) {
    DLiteInstance *inst = dlite_instance_get(uuid);
    printf("  %s\n", inst->uuid);
    //dlite_instance_decref(inst);
    n++;
  }
  printf("\n");
  dlite_storage_iter_free(s, iter);
  mu_assert_int_eq(2, n);
}

MU_TEST(test_storage_iter_bad_pattern)
{
  /* Iterate over UUIDs of instances who's metadata matches invalid pattern */
  char uuid[DLITE_UUID_LENGTH+1];
  void *iter = dlite_storage_iter_create(s, "xxx");
  int n = 0;
  printf("\nUUIDs (pattern iteration, bad):\n");
  while (dlite_storage_iter_next(s, iter, uuid) == 0) {
    printf("  %s\n", uuid);
    n++;
  }
  printf("\n");
  dlite_storage_iter_free(s, iter);
  mu_assert_int_eq(0, n);
}

MU_TEST(test_plugin_iter)
{
  int n=0;
  const DLiteStoragePlugin *api;
  DLiteStoragePluginIter *iter = dlite_storage_plugin_iter_create();
  printf("\nStorage plugins:\n");
  while ((api = dlite_storage_plugin_iter_next(iter))) {
    printf("  - api %d: '%s'\n", n, api->name);
    n++;
  }
  dlite_storage_plugin_iter_free(iter);
}

MU_TEST(test_load_all)
{
  int n=0;
  const DLiteStoragePlugin *api;
  DLiteStoragePluginIter *iter;
  mu_assert_int_eq(0, dlite_storage_plugin_load_all());
  iter = dlite_storage_plugin_iter_create();
  printf("\n\nStorage plugins (after calling load_all):\n");
  while ((api = dlite_storage_plugin_iter_next(iter))) {
    printf("  - api %d: '%s'\n", n, api->name);
    n++;
  }
  dlite_storage_plugin_iter_free(iter);
}


MU_TEST(test_close)
{
  mu_assert_int_eq(0, dlite_storage_close(s));
}


MU_TEST(unload_plugins)
{
  dlite_storage_plugin_unload_all();
}




/***********************************************************************/


MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_open);   /* setup */

  MU_RUN_TEST(test_open_url);
  MU_RUN_TEST(test_idflag);
  MU_RUN_TEST(test_uuids);
  MU_RUN_TEST(test_get_driver);
  MU_RUN_TEST(test_storage_iter);
  MU_RUN_TEST(test_storage_iter_pattern);
  MU_RUN_TEST(test_storage_iter_bad_pattern);
  MU_RUN_TEST(test_plugin_iter);
  MU_RUN_TEST(test_load_all);

  MU_RUN_TEST(test_close);  /* teardown */
  MU_RUN_TEST(unload_plugins);
}

int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
