#include <stdlib.h>
#include <stdio.h>

#include "minunit/minunit.h"

#include "dlite.h"
#include "dlite-macros.h"
#include "pyembed/dlite-pyembed-utils.h"
#include "dlite-storage-plugins.h"

/* This header should define HOST, DATABASE, USER and PASSWORD */
#include "pgconf.h"

DLiteStorage *db=NULL;
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
  if (!dlite_pyembed_has_module("psycopg")) exit(44);
}


MU_TEST(test_open_db)
{
  mu_check((db = dlite_storage_open("postgresql", HOST, options)));
  mu_assert_int_eq(1, dlite_storage_is_writable(db));
}


MU_TEST(test_save)
{
  DLiteInstance *meta, *inst;
  size_t dims[] = {2};
  const char *name = "Ada";
  double age = 42.;
  const char *skills[] = {"jumping", "hopping"};
  int n, i;
  char *paths = STRINGIFY(dlite_SOURCE_DIR) "/storage/python/tests-c/*.json";

  mu_check(dlite_storage_paths_append(paths) >= 0);
  mu_check((meta = dlite_instance_load_url("json://Person.json?mode=r")));
  mu_check((inst = dlite_instance_create((DLiteMeta *)meta, dims, "ada")));
  mu_assert_int_eq(0, dlite_instance_set_property(inst, "name", &name));
  mu_assert_int_eq(0, dlite_instance_set_property(inst, "age", &age));
  mu_assert_int_eq(0, dlite_instance_set_property(inst, "skills", skills));

  mu_assert_int_eq(0, dlite_instance_save_url("json://persons.json?mode=w",
                                              inst));
  if (db) {
    char url[256];
    mu_assert_int_eq(0, dlite_instance_save(db, inst));
    snprintf(url, sizeof(url), "postgresql://%s?%s", HOST, options);
    mu_assert_int_eq(0, dlite_instance_save_url(url,
                                                (DLiteInstance *)inst->meta));
  }

  n = inst->_refcount;
  mu_assert_int_eq(1, n);
  for (i=0; i<n; i++) dlite_instance_decref(inst);

  n = meta->_refcount;
  mu_assert_int_eq(2, n);
  for (i=0; i<n; i++) dlite_instance_decref(meta);
}


MU_TEST(test_load)
{
  DLiteInstance *inst;
  if (db) {
    mu_check((inst = dlite_instance_load(db, "ada")));
    mu_assert_int_eq(0, dlite_instance_save_url("json:persons2.json?mode=w", inst));
  }
}


MU_TEST(test_iter)
{
  char uuid[DLITE_UUID_LENGTH+1];
  if (db) {
    void *si = dlite_storage_iter_create(db, NULL);
    mu_check(si);
    printf("\n");
    while (dlite_storage_iter_next(db, si, uuid) == 0)
      printf("  - uuid: %s\n", uuid);
    dlite_storage_iter_free(db, si);
  }
}


MU_TEST(test_close_db)
{
  if (db)
    mu_assert_int_eq(0, dlite_storage_close(db));
}


MU_TEST(test_unload_plugins)
{
  dlite_storage_plugin_unload_all();
}



/***********************************************************************/


MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_for_psycopg);
  MU_RUN_TEST(test_open_db);
  MU_RUN_TEST(test_save);
  MU_RUN_TEST(test_load);
  MU_RUN_TEST(test_iter);
  MU_RUN_TEST(test_close_db);
  MU_RUN_TEST(test_unload_plugins);
}

int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
