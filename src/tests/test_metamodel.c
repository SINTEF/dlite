#include <stdio.h>

#include "config.h"

#include "minunit/minunit.h"
#include "dlite.h"


DLiteMetaModel *model;


MU_TEST(test_metamodel_create)
{
  model = dlite_metamodel_create("http://meta.sintef.no/0.1/Vehicle",
                                 DLITE_ENTITY_SCHEMA, NULL);
  mu_check(model);
}

MU_TEST(test_metamodel_add_value)
{
  char *descr = "A vehicle like car, bike, etc...";
  mu_assert_int_eq(0, dlite_metamodel_add_string(model, "description", descr));
}

MU_TEST(test_metamodel_add_dimension)
{
  char *descr = "Number of checks it has been through.";
  mu_assert_int_eq(0, dlite_metamodel_add_dimension(model, "nchecks", descr));
}

MU_TEST(test_metamodel_add_property)
{
  int stat;
  stat = dlite_metamodel_add_property(model, "brand", "string32", NULL, NULL,
                                      "Brand of the vehicle.");
  mu_assert_int_eq(0, stat);

  stat = dlite_metamodel_add_property(model, "checks", "int32", NULL, NULL,
                                      "Year of each check.");
  mu_assert_int_eq(0, stat);
  stat = dlite_metamodel_add_property_dim(model, "checks", "nchecks");
  mu_assert_int_eq(0, stat);
}

MU_TEST(test_metamodel_create_meta)
{
  DLiteMeta *meta = dlite_meta_create_from_metamodel(model);
  mu_check(meta);
  //dlite_instance_print((DLiteInstance *)meta);
  dlite_instance_save_url("json://Vehicle.json?mode=w&meta=yes",
                          (DLiteInstance *)meta);
  dlite_meta_decref(meta);
}

MU_TEST(test_metamodel_free)
{
  dlite_metamodel_free(model);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_metamodel_create);     /* setup */
  MU_RUN_TEST(test_metamodel_add_value);
  MU_RUN_TEST(test_metamodel_add_dimension);
  MU_RUN_TEST(test_metamodel_add_property);
  MU_RUN_TEST(test_metamodel_create_meta);
  MU_RUN_TEST(test_metamodel_free);      /* tear down */
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
