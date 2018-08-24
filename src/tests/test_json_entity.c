#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "dlite.h"

#include "config.h"

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s


DLiteEntity *entity = NULL;


/***************************************************************
 * Test entity
 ***************************************************************/

MU_TEST(test_entity_load)
{
  DLiteStorage *s;
  DLiteInstance *e;  /* the entity casted to a DLiteInstance */
  mu_check((s =
	    dlite_storage_open("json",
		       STRINGIFY(DLITE_ROOT) "/tools/tests/Chemistry-0.1.json",
			       "r")));
  mu_check((entity =
	    dlite_entity_load(s, "http://www.sintef.no/calm/0.1/Chemistry")));

  e = (DLiteInstance *)entity;

  mu_assert_int_eq(2, dlite_instance_get_dimension_size(e, "dimensions"));
  mu_assert_int_eq(8, dlite_instance_get_dimension_size(e, "properties"));
  /*
  mu_assert_string_eq(
  dlite_instance_get_property((*DLiteInstance)entity,
  */
}


MU_TEST(test_instance_create)
{
  size_t dims[] = {3, 2};
  char *elements[] = {"Al", "Mg", "Si"};
  DLiteInstance *inst = dlite_instance_create(entity, dims, "myinst");

  mu_check(dlite_instance_set_property(inst, "alloy", "6063"));
  mu_check(dlite_instance_set_property(inst, "elements", elements));
}


MU_TEST(test_entity_free)
{
  dlite_entity_decref(entity);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_entity_load);     /* setup */
  MU_RUN_TEST(test_instance_create);
  MU_RUN_TEST(test_entity_free);     /* tear down */
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
