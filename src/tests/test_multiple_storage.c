/* Test looking up instances with dlite_instance_get() when several
   storages are open at the same time.

   Opened readable storages are added to the storage hotlist, which
   dlite_instance_get() searches before the storage search paths.
   Instances should be found in the right storage and the storages
   should be closed cleanly afterwards. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"

#include "dlite.h"
#include "dlite-macros.h"

#define THISDIR STRINGIFY(dlite_SOURCE_DIR) "/src/tests/"


MU_TEST(test_multiple_storage)
{
  DLiteStorage *s1=NULL, *s2=NULL;
  DLiteInstance *meta=NULL, *inst=NULL;
  char *metaid = "http://onto-ns.com/meta/0.1/test-entity";
  char *id = "204b05b2-4c89-43f4-93db-fd1cb70f54ef";

  mu_check((s1 = dlite_storage_open("json", THISDIR "test-entity.json",
                                    "mode=r")));
  mu_check((s2 = dlite_storage_open("json", THISDIR "test-data.json",
                                    "mode=r")));

  /* Look up the instances via the hotlist */
  mu_check((meta = dlite_instance_get(metaid)));
  mu_assert_string_eq(metaid, meta->uri);
  mu_check((inst = dlite_instance_get(id)));
  mu_assert_string_eq(id, inst->uuid);
  mu_check(inst->meta == (DLiteMeta *)meta);

  /* Close the storages before releasing the instances */
  mu_assert_int_eq(0, dlite_storage_close(s2));
  mu_assert_int_eq(0, dlite_storage_close(s1));

  mu_assert_int_eq(0, dlite_instance_decref(inst));
  dlite_instance_decref(meta);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_multiple_storage);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
