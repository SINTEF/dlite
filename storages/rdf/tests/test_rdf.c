
#include "minunit/minunit.h"
#include "dlite.h"
#include "dlite-macros.h"


DLiteInstance *inst=NULL;
DLiteMeta *meta=NULL;



MU_TEST(test_load_inst)
{
  char *url;
  url = "json://"
    STRINGIFY(dlite_SOURCE_DIR)  // cppcheck-suppress unknownMacro
    "/src/tests/test-entity.json?mode=r";
  meta = dlite_meta_load_url(url);
  mu_check(meta);

  url="json://" STRINGIFY(dlite_SOURCE_DIR) "/src/tests/test-data.json?mode=r"
    "#117a8bb9-df2e-5c77-a84d-3ac45add03f0";
  inst = dlite_instance_load_url(url);
  mu_check(inst);
}


MU_TEST(test_write)
{
  DLiteStorage *s = dlite_storage_open("rdf", "db.xml",
                                       "mode=w;"
                                       "store=file;"
                                       "filename=data.ttl;"
                                       "format=turtle");
  mu_check(s);
  mu_assert_int_eq(0, dlite_instance_save(s, (DLiteInstance *)meta));
  mu_assert_int_eq(0, dlite_instance_save(s, inst));
  mu_assert_int_eq(0, dlite_storage_close(s));
}


MU_TEST(test_load)
{
  int stat, nref;
  char buf[4096];
  DLiteStorage *s = dlite_storage_open("rdf", "db.xml",
                                       "mode=r;"
                                       "store=file");
  mu_check(s);

  /* Forget instance before we load it again... */
  nref = inst->_refcount;
  while (nref--) dlite_instance_decref(inst);

  inst = dlite_instance_load(s, "117a8bb9-df2e-5c77-a84d-3ac45add03f0");
  mu_check(inst);
  dlite_json_sprint(buf, sizeof(buf), inst, 0, 0);
  printf("%s\n", buf);

  stat = dlite_storage_close(s);
  mu_assert_int_eq(0, stat);
}


MU_TEST(test_iter)
{
  int stat;
  void *iter;
  char buf[DLITE_UUID_LENGTH+1];
  char *loc = STRINGIFY(dlite_SOURCE_DIR) "/storages/rdf/tests/data.xml";
  DLiteStorage *s = dlite_storage_open("rdf", loc, "mode=r;store=file");
  mu_check(s);

  iter = dlite_storage_iter_create(s, NULL);
  printf("\n\nAll instances:\n");
  while (dlite_storage_iter_next(s, iter, buf) == 0)
    printf("- %s\n", buf);
  printf("\n");
  dlite_storage_iter_free(s, iter);

  iter = dlite_storage_iter_create(s, "*Schema");
  printf("Metadata:\n");
  while (dlite_storage_iter_next(s, iter, buf) == 0)
    printf("- %s\n", buf);
  printf("\n");
  dlite_storage_iter_free(s, iter);

  iter = dlite_storage_iter_create(s, "http://*");
  printf("Starts with http:\n");
  while (dlite_storage_iter_next(s, iter, buf) == 0)
    printf("- %s\n", buf);
  printf("\n");
  dlite_storage_iter_free(s, iter);

  stat = dlite_storage_close(s);
  mu_assert_int_eq(0, stat);
}


MU_TEST(test_freedata)
{
  if (meta) {
    dlite_meta_decref(meta);
    dlite_meta_decref(meta);
  }
  if (inst) dlite_instance_decref(inst);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_load_inst);
  MU_RUN_TEST(test_write);
  MU_RUN_TEST(test_load);
  MU_RUN_TEST(test_iter);
  MU_RUN_TEST(test_freedata);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
