#include "config.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "dlite.h"
#include "dlite-collection.h"

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s


DLiteCollection *coll = NULL;


/***************************************************************
 * Test collection
 ***************************************************************/

MU_TEST(test_collection_create)
{
  mu_check((coll = dlite_collection_create("mycoll")));
}


MU_TEST(test_collection_add_relation)
{
  mu_assert_int_eq(0, dlite_instance_get_dimension_size((DLiteInstance *)coll,
							"nrelations"));
  mu_check(!dlite_collection_add_relation(coll, "dog", "is_a", "animal"));
  mu_check(!dlite_collection_add_relation(coll, "cat", "is_a", "animal"));
  mu_check(!dlite_collection_add_relation(coll, "terrier", "is_a", "dog"));
  mu_assert_int_eq(3, dlite_instance_get_dimension_size((DLiteInstance *)coll,
							"nrelations"));

  /* dublicate... */
  mu_check(!dlite_collection_add_relation(coll, "terrier", "is_a", "dog"));
  mu_assert_int_eq(3, dlite_instance_get_dimension_size((DLiteInstance *)coll,
							"nrelations"));
}


MU_TEST(test_collection_remove_relations)
{
  mu_assert_int_eq(2, dlite_collection_remove_relations(coll, NULL,
							"is_a", "animal"));
  mu_assert_int_eq(1, dlite_instance_get_dimension_size((DLiteInstance *)coll,
							"nrelations"));
}


MU_TEST(test_collection_find)
{
  const DLiteRelation *r;
  DLiteCollectionState state;
  int nanimals=0;

  mu_check(!dlite_collection_add_relation(coll, "dog", "is_a", "animal"));
  mu_check(!dlite_collection_add_relation(coll, "cat", "is_a", "animal"));
  mu_check(!dlite_collection_add_relation(coll, "terrier", "is_a", "dog"));
  mu_check(!dlite_collection_add_relation(coll, "car", "is_a", "vehicle"));
  mu_assert_int_eq(4, dlite_instance_get_dimension_size((DLiteInstance *)coll,
							"nrelations"));

  dlite_collection_init_state(coll, &state);
  printf("\nAnimals:\n");
  while ((r = dlite_collection_find(coll, &state, NULL, "is_a", "animal"))) {
    printf("  %s\n", r->s);
    nanimals++;
  }
  mu_assert_int_eq(2, nanimals);
}


MU_TEST(test_collection_add)
{
  char *path, *uri;
  DLiteStorage *s;
  DLiteInstance *e, *inst;

  path = STRINGIFY(DLITE_ROOT) "/src/tests/test-entity.json";
  mu_check((s = dlite_storage_open("json", path, "mode=r")));
  mu_check((e = dlite_instance_load(s, NULL)));
  mu_check(!dlite_storage_close(s));

  path = STRINGIFY(DLITE_ROOT) "/src/tests/test-data.json";
  uri = "my_test_instance";
  //uri = "e076a856-e36e-5335-967e-2f2fd153c17d";
  mu_check((s = dlite_storage_open("json", path, "mode=r")));
  mu_check((inst = dlite_instance_load(s, uri)));
  mu_check(!dlite_storage_close(s));

  mu_assert_int_eq(0, dlite_collection_count(coll));
  mu_check(!dlite_collection_add_new(coll, "e", e));
  mu_check(!dlite_collection_add(coll, "inst", inst));
  mu_check(!dlite_collection_add_new(coll, "inst2", inst));
  mu_assert_int_eq(3, dlite_collection_count(coll));
}


MU_TEST(test_collection_get)
{
  const DLiteInstance *inst;
  mu_check((inst = dlite_collection_get(coll, "inst")));
  mu_check(!dlite_collection_get(coll,"XXX"));
}


MU_TEST(test_collection_next)
{
  const DLiteInstance *inst;
  DLiteCollectionState state;
  int ninst=0;

  dlite_collection_init_state(coll, &state);
  printf("\nInstances:\n");
  while ((inst = dlite_collection_next(coll, &state))) {
    printf("  %s\n", (inst->uri) ? inst->uri : inst->uuid);
    ninst++;
  }
  dlite_collection_deinit_state(&state);
  mu_assert_int_eq(3, ninst);
}


MU_TEST(test_collection_remove)
{
  mu_assert_int_eq(3, dlite_collection_count(coll));

  mu_check(dlite_collection_remove(coll, "nonexisting"));  // fail
  mu_assert_int_eq(3, dlite_collection_count(coll));

  mu_check(!dlite_collection_remove(coll, "e"));
  mu_assert_int_eq(2, dlite_collection_count(coll));

  mu_check(!dlite_collection_remove(coll, "inst2"));
  mu_assert_int_eq(1, dlite_collection_count(coll));

  mu_check(dlite_collection_remove(coll, "inst2"));  // fail, already removed
  mu_assert_int_eq(1, dlite_collection_count(coll));

  mu_check(!dlite_collection_remove(coll, "inst"));
  mu_assert_int_eq(0, dlite_collection_count(coll));
}


MU_TEST(test_collection_free)
{
  dlite_collection_decref(coll);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_collection_create);     /* setup */

  MU_RUN_TEST(test_collection_add_relation);
  MU_RUN_TEST(test_collection_remove_relations);

  MU_RUN_TEST(test_collection_find);

#ifdef WITH_JSON
  MU_RUN_TEST(test_collection_add);
  MU_RUN_TEST(test_collection_get);
  MU_RUN_TEST(test_collection_next);
  MU_RUN_TEST(test_collection_remove);
#endif

  MU_RUN_TEST(test_collection_free);       /* tear down */
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
