#include "config.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "utils/err.h"
#include "dlite.h"
#include "dlite-collection.h"
#include "dlite-macros.h"


DLiteCollection *coll = NULL;



MU_TEST(test_collection_create)
{
  mu_check((coll = dlite_collection_create("mycoll")));
}


MU_TEST(test_collection_add_relation)
{
  mu_assert_int_eq(0, dlite_instance_get_dimension_size((DLiteInstance *)coll,
							"nrelations"));

  mu_check(!dlite_collection_add_relation(coll, "dog", "is_a", "animal",
                                          NULL));
  mu_check(!dlite_collection_add_relation(coll, "cat", "is_a", "animal",
                                          NULL));
  mu_check(!dlite_collection_add_relation(coll, "terrier", "is_a", "dog",
                                          NULL));
  mu_assert_int_eq(3, dlite_instance_get_dimension_size((DLiteInstance *)coll,
							"nrelations"));
  mu_assert_int_eq(3, coll->nrelations);

  /* dublicate... */
  mu_check(!dlite_collection_add_relation(coll, "terrier", "is_a", "dog",
                                          NULL));
  mu_assert_int_eq(3, dlite_instance_get_dimension_size((DLiteInstance *)coll,
							"nrelations"));
}


MU_TEST(test_collection_remove_relations)
{
  mu_assert_int_eq(2, dlite_collection_remove_relations(coll, NULL, "is_a",
                                                        "animal", NULL));
  mu_assert_int_eq(1, dlite_instance_get_dimension_size((DLiteInstance *)coll,
							"nrelations"));
}


MU_TEST(test_collection_find)
{
  const DLiteRelation *r;
  DLiteCollectionState state;
  int nanimals=0, n=0;

  mu_check(!dlite_collection_add_relation(coll, "dog", "is_a", "animal",
                                          NULL));
  mu_check(!dlite_collection_add_relation(coll, "cat", "is_a", "animal",
                                          NULL));
  mu_check(!dlite_collection_add_relation(coll, "terrier", "is_a", "dog",
                                          NULL));
  mu_check(!dlite_collection_add_relation(coll, "car", "is_a", "vehicle",
                                          NULL));
  mu_check(!dlite_collection_add_relation(coll, "cat", "has_color", "black",
                                          "xsd:string"));
  mu_check(!dlite_collection_add_relation(coll, "car", "has_color", "black",
                                          "xsd:string"));
  mu_assert_int_eq(6, dlite_instance_get_dimension_size((DLiteInstance *)coll,
							"nrelations"));

  dlite_collection_init_state(coll, &state);
  printf("\nRelations:\n");
  while ((r = dlite_collection_find(coll, &state, NULL, NULL, NULL, NULL)))
    printf("  %-10s %-10s %-10s %s\n", r->s, r->p, r->o, r->d);

  dlite_collection_reset_state(&state);
  printf("\nAnimals:\n");
  while ((r = dlite_collection_find(coll, &state, NULL, "is_a", "animal",
                                    NULL))) {
    printf("  %s\n", r->s);
    nanimals++;
  }
  mu_assert_int_eq(2, nanimals);

  dlite_collection_reset_state(&state);
  n = 0;
  while (dlite_collection_find(coll, &state, NULL, NULL, NULL, "xsd:int")) n++;
  mu_assert_int_eq(0, n);

  dlite_collection_reset_state(&state);
  n = 0;
  while (dlite_collection_find(coll, &state, NULL, NULL, NULL, "xsd:string"))
    n++;
  mu_assert_int_eq(2, n);

  dlite_collection_reset_state(&state);
  n = 0;
  while (dlite_collection_find(coll, &state, NULL, NULL, "black", "xsd:string"))
    n++;
  mu_assert_int_eq(2, n);

  dlite_collection_reset_state(&state);
  n = 0;
  while (dlite_collection_find(coll, &state, NULL, NULL, "red", "xsd:string"))
    n++;
  mu_assert_int_eq(0, n);

  dlite_collection_reset_state(&state);
  n = 0;
  while (dlite_collection_find(coll, &state, "car", NULL, NULL, "xsd:string"))
    n++;
  mu_assert_int_eq(1, n);

  dlite_collection_reset_state(&state);
  n = 0;
  while (dlite_collection_find(coll, &state, "car", NULL, NULL, ""))
    n++;
  mu_assert_int_eq(1, n);

  dlite_collection_deinit_state(&state);
}


MU_TEST(test_collection_value)
{
  mu_assert_string_eq("terrier", dlite_collection_value(coll, NULL, "is_a",
                                                        "dog", NULL, NULL, 0));
  mu_assert_string_eq("dog", dlite_collection_value(coll, NULL, "is_a",
                                                    "animal", NULL, NULL, 1));

 ErrTry:
  mu_assert_string_eq(NULL, dlite_collection_value(coll, NULL, "is_a",
                                                   "animal", NULL, NULL, 0));
  mu_assert_string_eq(NULL, dlite_collection_value(coll, NULL, "is_a",
                                                   "mammal", NULL, NULL, 0));
 ErrCatch(dliteLookupError):
  break;
 ErrEnd;
}


MU_TEST(test_collection_add)
{
  char *path, *uri;
  DLiteStorage *s;
  DLiteInstance *e, *inst;

  path = STRINGIFY(dlite_SOURCE_DIR) "/src/tests/test-entity.json";
  mu_check((s = dlite_storage_open("json", path, "mode=r")));
  mu_check((e = dlite_instance_load(s, NULL)));
  mu_check(!dlite_storage_close(s));

  path = STRINGIFY(dlite_SOURCE_DIR) "/src/tests/test-data.json";
  uri = "http://data.org/my_test_instance";
  mu_check((s = dlite_storage_open("json", path, "mode=r")));
  mu_check((inst = dlite_instance_load(s, uri)));
  mu_check(!dlite_storage_close(s));

  /* Add `e` and `inst` to collection and "steal" the references. */
  mu_assert_int_eq(0, dlite_collection_count(coll));
  mu_check(!dlite_collection_add_new(coll, "e", e));
  mu_check(!dlite_collection_add(coll, "inst", inst));
  mu_check(!dlite_collection_add_new(coll, "inst2", inst));
  mu_assert_int_eq(3, dlite_collection_count(coll));

  /* Save to file */
  dlite_collection_save_url(coll, "coll.json?mode=w");
}


MU_TEST(test_collection_get)
{
  const DLiteInstance *inst;
  mu_check((inst = dlite_collection_get(coll, "inst")));
  mu_check(!dlite_collection_get(coll, "XXX"));
  dlite_errclr();
}


MU_TEST(test_collection_next)
{
  const DLiteInstance *inst;
  DLiteCollectionState state;
  int ninst=0;

  dlite_collection_init_state(coll, &state);
  printf("\nInstances:\n");
  while ((inst = dlite_collection_next(coll, &state))) {
    printf("  %s (refcount=%d)\n", (inst->uri) ? inst->uri : inst->uuid,
           inst->_refcount);
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


MU_TEST(test_collection_save)
{
  DLiteStorage *s;
  mu_check((s = dlite_storage_open("json", "coll1.json", "mode=w")));
  mu_check(dlite_instance_save(s, (DLiteInstance *)coll) == 0);
  mu_check(dlite_storage_close(s) == 0);
}



MU_TEST(test_collection_load)
{
  DLiteCollection *coll2;
  char *collpath = STRINGIFY(dlite_SOURCE_DIR) "/src/tests/coll.json";
  FILE *fp = fopen(collpath, "r");
  dlite_storage_paths_append(STRINGIFY(dlite_SOURCE_DIR) "/src/tests/*.json");
  coll2 = (DLiteCollection *)
    dlite_json_fscan(fp, NULL, "http://onto-ns.com/meta/0.1/Collection");
  fclose(fp);  // cppcheck-suppress nullPointerOutOfResources

  dlite_collection_decref(coll2);
}


MU_TEST(test_collection_free)
{
  dlite_collection_decref(coll);

  dlite_finalize();  // for checking memory leaks
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  //MU_RUN_TEST(test1);


  MU_RUN_TEST(test_collection_create);     /* setup */

  MU_RUN_TEST(test_collection_add_relation);
  MU_RUN_TEST(test_collection_remove_relations);
  MU_RUN_TEST(test_collection_find);
  MU_RUN_TEST(test_collection_value);

  MU_RUN_TEST(test_collection_add);
  MU_RUN_TEST(test_collection_get);
  MU_RUN_TEST(test_collection_next);
  MU_RUN_TEST(test_collection_remove);
  MU_RUN_TEST(test_collection_save);
  MU_RUN_TEST(test_collection_load);

  MU_RUN_TEST(test_collection_free);       /* tear down */
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
