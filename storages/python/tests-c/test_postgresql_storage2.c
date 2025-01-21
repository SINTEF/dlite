#include <stdlib.h>
#include <stdio.h>

#include "minunit/minunit.h"

#include "dlite.h"
#include "dlite-macros.h"
#include "pyembed/dlite-pyembed-utils.h"
#include "dlite-storage-plugins.h"

/* This header should define HOST, DATABASE, USER and PASSWORD */
#include "pgconf.h"


#ifdef PASSWORD
// cppcheck-suppress [syntaxError, unknownMacro]
char *options = "database=" DATABASE ";user=" USER ";password=" PASSWORD;
#else
// cppcheck-suppress [syntaxError, unknownMacro]
char *options = "database=" DATABASE ";user=" USER;
#endif


// Not really a unit test, but check that the Python package "psycopg"
// is available.  If not, exit with code 44, indicating that the test
// should be skipped
MU_TEST(test_for_psycopg)
{
  if (!dlite_pyembed_has_module("psycopg2")) exit(44);
}


MU_TEST(test_load_meta)
{
  DLiteInstance *meta;
  char url[256], *id="http://onto-ns.com/meta/0.1/Person";
  char *paths = STRINGIFY(dlite_SOURCE_DIR) "/storage/python/tests-c/*.json";

  mu_check(dlite_storage_plugin_path_append(paths) >= 0);

  snprintf(url, sizeof(url), "postgresql://%s?%s#%s", HOST, options, id);
  mu_check((meta = dlite_instance_load_url(url)));

  mu_assert_int_eq(0, dlite_instance_save_url("json:Person2.json?mode=w",
                                              meta));
}

MU_TEST(test_load_inst)
{
  DLiteInstance *inst;
  char url[256], *id="21495524-a02f-5695-82e2-b117addc0b1e";
  snprintf(url, sizeof(url), "postgresql://%s?%s#%s", HOST, options, id);
  mu_check((inst = dlite_instance_load_url(url)));

  mu_assert_int_eq(0, dlite_instance_save_url("json:persons3.json?mode=w",
                                              inst));
}

MU_TEST(test_unload_plugins)
{
  //dlite_storage_plugin_unload_all();
}


/***********************************************************************/


MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_for_psycopg);
  MU_RUN_TEST(test_load_inst);
  MU_RUN_TEST(test_load_meta);
  MU_RUN_TEST(test_unload_plugins);
}

int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
