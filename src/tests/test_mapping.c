#include <stdio.h>

#include "minunit/minunit.h"

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-storage.h"
#include "dlite-mapping.h"
#include "dlite-mapping-plugins.h"





MU_TEST(test_mapping_path)
{
  char *mpath = STRINGIFY(dlite_BINARY_DIR) "/src/tests/mappings";
  char *spath1 = STRINGIFY(dlite_SOURCE_DIR) "/src/tests/*.json";
  char *spath2 = STRINGIFY(dlite_SOURCE_DIR) "/src/tests/mappings/*.json";

  dlite_mapping_plugin_path_insert(0, mpath);
  dlite_storage_paths_insert(0, spath1);
  dlite_storage_paths_insert(0, spath2);

  const char **paths, **p;
  printf("\nStorage plugin paths:\n");
  paths = dlite_storage_plugin_paths();
  for (p=paths; *p; p++)
    printf("  - '%s'\n", *p);

  printf("\nMapping plugin paths:\n");
  paths = dlite_mapping_plugin_paths();
  for (p=paths; *p; p++)
    printf("  - '%s'\n", *p);

  printf("\nStorages:\n");
  paths = dlite_storage_paths_get();
  for (p=paths; *p; p++)
    printf("  - '%s'\n", *p);

  printf("\nStorages (wildcard-expanded):\n");
  const char *path;
  DLiteStoragePathIter *iter = dlite_storage_paths_iter_start();
  while ((path = dlite_storage_paths_iter_next(iter)))
    printf("  - '%s'\n", path);
  dlite_storage_paths_iter_stop(iter);

  printf("\n");
}


MU_TEST(test_create_from_id)
{
  int b=-13, *p;
  DLiteInstance *inst;
  inst = dlite_instance_create_from_id("http://onto-ns.com/meta/0.1/ent2",
                                       NULL, NULL);
  mu_check(inst);

  dlite_instance_set_property(inst, "b", &b);
  p = dlite_instance_get_property(inst, "b");
  mu_assert_int_eq(-13, *p);
  dlite_instance_decref(inst);
}


MU_TEST(test_mapping)
{
  DLiteInstance *inst, *inst2, *insts[1];
  const DLiteInstance **instances = (const DLiteInstance **)insts;
  DLiteMapping *m;
  const char *output_uri = "http://onto-ns.com/meta/0.1/ent2";
  const char *input_uris[] = { "http://onto-ns.com/meta/0.1/ent1" };
  char *str;

  mu_check((inst = dlite_instance_get("2daa6967-8ecd-4248-97b2-9ad6fefeac14")));
  instances[0] = inst;

  m = dlite_mapping_create(output_uri, input_uris, 1);
  mu_check(m);

  str = dlite_mapping_string(m);
  mu_check(str);
  printf("\nmapping:\n");
  printf("%s\n", str);
  free(str);

  inst2 = dlite_mapping_map(m, instances, 1);
  mu_check(inst2);
  dlite_mapping_free(m);

  dlite_instance_decref(inst2);
  dlite_instance_decref(inst);
}


MU_TEST(test_get_casted)
{
  DLiteInstance *inst;
  const char *output_uri = "http://onto-ns.com/meta/0.1/ent2";
  mu_check((inst =
            dlite_instance_get_casted("2daa6967-8ecd-4248-97b2-9ad6fefeac14",
                                      output_uri)));
  dlite_instance_decref(inst);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_mapping_path);
  MU_RUN_TEST(test_create_from_id);
  MU_RUN_TEST(test_mapping);
  MU_RUN_TEST(test_get_casted);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
