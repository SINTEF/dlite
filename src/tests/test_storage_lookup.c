#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "dlite.h"
#include "dlite-macros.h"

#include "config.h"


MU_TEST(test_storage_lookup)
{
  //DLiteMeta *e;
  DLiteInstance *inst;
  DLiteStorage *s;
  char *path = STRINGIFY(dlite_SOURCE_DIR) "/src/tests/*.json";
  dlite_storage_paths_append(path);
  //mu_check((e = dlite_meta_get("http://onto-ns.com/meta/0.1/test-entity")));
  mu_check((inst = dlite_instance_get("204b05b2-4c89-43f4-93db-fd1cb70f54ef")));

  mu_check((s = dlite_storage_open("json", "storage_lookup.json", "mode=w")));
  //mu_assert_int_eq(0, dlite_meta_save(s, e));
  mu_assert_int_eq(0, dlite_instance_save(s, inst));
  mu_assert_int_eq(0, dlite_storage_close(s));

  dlite_instance_decref(inst);
  //dlite_meta_decref(e);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_storage_lookup);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
