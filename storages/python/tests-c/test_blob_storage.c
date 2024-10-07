#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "minunit/minunit.h"

#include "utils/integers.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-storage-plugins.h"


DLiteInstance *inst = NULL;


MU_TEST(test_load)
{
  dlite_storage_plugin_load_all();


  char *url = "blob://"
    STRINGIFY(CURRENT_SOURCE_DIR)  // cppcheck-suppress unknownMacro
    "/test_blob_storage.c?mode=r";
  inst = dlite_instance_load_url(url);
  mu_check(inst);
}

MU_TEST(test_save)
{
  int stat;
  char *url = "blob://" STRINGIFY(CURRENT_BINARY_DIR) "/blob-output.c?mode=w";
  assert(inst);
  stat = dlite_instance_save_url(url, inst);
  mu_assert_int_eq(0, stat);
}

MU_TEST(test_unload_plugins)
{
  dlite_instance_decref(inst);
  dlite_storage_plugin_unload_all();
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_load);
  MU_RUN_TEST(test_save);
  MU_RUN_TEST(test_unload_plugins);
}

int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
