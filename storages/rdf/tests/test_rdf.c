
#include "minunit/minunit.h"
#include "dlite.h"

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s


DLiteInstance *inst=NULL;
DLiteMeta *meta=NULL;


MU_TEST(test_loaddata)
{
  char *url;
  url="json://"STRINGIFY(dlite_SOURCE_DIR)"/src/tests/test-entity.json?mode=r";
  meta = dlite_meta_load_url(url);
  mu_check(meta);

  url="json://" STRINGIFY(dlite_SOURCE_DIR) "/src/tests/test-data.json?mode=r"
    "#e076a856-e36e-5335-967e-2f2fd153c17d";
  inst = dlite_instance_load_url(url);
  mu_check(inst);
}


MU_TEST(test_write)
{
  DLiteStorage *s = dlite_storage_open("rdf", "test-file.xml",
                                       "mode=w;"
                                       "store=file;"
                                       "base-uri=case1;"
                                       "filename=-");
  mu_check(s);
  mu_assert_int_eq(0, dlite_instance_save(s, (DLiteInstance *)meta));
  mu_assert_int_eq(0, dlite_instance_save(s, inst));
  mu_assert_int_eq(0, dlite_storage_close(s));
}


MU_TEST(test_freedata)
{
  dlite_instance_decref(inst);
  dlite_meta_decref(meta);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_loaddata);

  MU_RUN_TEST(test_write);

  MU_RUN_TEST(test_freedata);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
