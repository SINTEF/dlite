#include <stdio.h>

#include "minunit/minunit.h"

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-mapping.h"
#include "dlite-mapping-plugins.h"


MU_TEST(test_mapping_path)
{
  DLiteMeta *e;
  const DLiteMappingPlugin *mp;
  char *mpath = STRINGIFY(DLITE_BINARY_ROOT) "/src/tests/mappings";
  char *spath = STRINGIFY(DLITE_ROOT) "/src/tests/mappings/*.json";

  dlite_mapping_plugin_path_insert(0, mpath);
  dlite_storage_paths_insert(0, spath);

  mu_check((e = dlite_meta_get("http://meta.sintef.no/0.1/ent1")));
  mu_check((mp = dlite_mapping_plugin_get("mapA")));
  dlite_meta_decref(e);
}


MU_TEST(test_mapping)
{
  DLiteInstance *inst, *inst2, *insts[1];
  const DLiteInstance **instances = (const DLiteInstance **)insts;
  DLiteMapping *m;
  const char *output_uri = "http://meta.sintef.no/0.1/ent2";
  const char *input_uris[] = { "http://meta.sintef.no/0.1/ent1" };
  char *str;

  UNUSED(inst2);

  mu_check((inst = dlite_instance_get("2daa6967-8ecd-4248-97b2-9ad6fefeac14")));
  instances[0] = inst;

  m = dlite_mapping_create(output_uri, input_uris, 1);
  mu_check(m);

  str = dlite_mapping_string(m);
  mu_check(str);
  printf("\n%s\n", str);
  free(str);

  inst2 = dlite_mapping_map(m, instances, 1);
  mu_check(inst2);


  //inst2 = dlite_mapping("http://meta.sintef.no/0.1/ent2", instances, 1);
  //mu_check(inst2);

  dlite_instance_decref(inst);
  dlite_instance_decref(inst2);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_mapping_path);
  MU_RUN_TEST(test_mapping);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
