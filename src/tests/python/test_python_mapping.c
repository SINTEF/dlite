#include <stdio.h>

#include "minunit/minunit.h"

#include "dlite-pyembed.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-mapping.h"
#include "dlite-storage-plugins.h"
#include "dlite-python-mapping.h"


MU_TEST(test_initialize)
{
  const char **p;
  dlite_python_mapping_paths_append(STRINGIFY(TESTDIR));
  dlite_storage_paths_append(STRINGIFY(TESTDIR) "/../../tests/mappings/*.json");
  dlite_mapping_plugin_path_append(STRINGIFY(DLITE_BINARY_ROOT) "/src/pyembed");


  printf("\nStorage paths:\n");
  for (p = dlite_storage_paths_get(); *p; p++)
    printf("  - '%s'\n", *p);
  printf("\n");

  printf("Storage plugin paths:\n");
  for (p = dlite_storage_plugin_paths(); *p; p++)
    printf("  - '%s'\n", *p);
  printf("\n");

  printf("Mapping plugin paths:\n");
  for (p = dlite_mapping_plugin_paths(); *p; p++)
    printf("  - '%s'\n", *p);
  printf("\n\n");

  printf("Python mapping paths:\n");
  for (p = dlite_python_mapping_paths_get(); *p; p++)
    printf("  - '%s'\n", *p);
  printf("\n\n");
}


MU_TEST(test_map)
{
  DLiteInstance *insts[1], *inst3, *ent3;
  const DLiteInstance **instances = (const DLiteInstance **)insts;
  void *p;
  instances[0] = dlite_instance_get("2daa6967-8ecd-4248-97b2-9ad6fefeac14");
  mu_check(instances[0]);

  ent3 = dlite_instance_get("http://onto-ns.com/meta/0.1/ent3");
  mu_check(ent3);
  inst3 = dlite_mapping("http://onto-ns.com/meta/0.1/ent3", instances, 1);
  mu_check(inst3);
  mu_check((p = dlite_instance_get_property(inst3, "c")));
  mu_assert_double_eq(54.0, *(double *)p);

  dlite_instance_save_url("json://inst3.json", inst3);

  dlite_instance_decref(inst3);
  //dlite_instance_decref((DLiteInstance *)instances[0]);
}


MU_TEST(test_finalize)
{
  dlite_python_mapping_paths_clear();
  dlite_python_mapping_unload();
  mu_assert_int_eq(0, dlite_pyembed_finalise());
}


MU_TEST(test_plugin_unload_all)
{
  dlite_storage_plugin_unload_all();
  dlite_mapping_plugin_unload_all();
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_initialize);
  MU_RUN_TEST(test_map);
  MU_RUN_TEST(test_finalize);
  MU_RUN_TEST(test_plugin_unload_all);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
