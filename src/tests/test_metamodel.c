#include <stdio.h>

#include "config.h"

#include "minunit/minunit.h"
#include "dlite.h"


DLiteMetaModel *model;
DLiteMetaModel *nodim;


MU_TEST(test_metamodel_create)
{
  model = dlite_metamodel_create("http://onto-ns.com/meta/0.1/Vehicle",
                                 DLITE_ENTITY_SCHEMA, NULL);
  mu_check(model);

  nodim = dlite_metamodel_create("http://onto-ns.com/meta/1.0/NoDimension",
                                 DLITE_ENTITY_SCHEMA, NULL);
  mu_check(nodim);

}

MU_TEST(test_metamodel_add_value)
{
  char *descr = "A vehicle like car, bike, etc...";
  mu_assert_int_eq(0, dlite_metamodel_add_string(model, "description", descr));

  char *descr2 = "A metadata without dimension";
  mu_assert_int_eq(0, dlite_metamodel_add_string(nodim, "description", descr2));
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

  stat = dlite_metamodel_add_property(nodim, "name", "string32", NULL, NULL,
                                      "Name of the instance.");
  mu_assert_int_eq(0, stat);
  stat = dlite_metamodel_add_property(nodim, "value", "float", "mm", NULL,
                                      "Value of the instance.");
  mu_assert_int_eq(0, stat);
}

MU_TEST(test_metamodel_create_meta)
{
  int stat;

  DLiteMeta *meta = dlite_meta_create_from_metamodel(model);
  mu_check(meta);
  //dlite_instance_print((DLiteInstance *)meta);
  dlite_instance_save_url("json://Vehicle.json?mode=w&with-uuid=yes",
                          (DLiteInstance *)meta);

  size_t dims[] = {0};
  DLiteInstance *vehicle = dlite_instance_create(meta, dims, NULL);
  mu_check(vehicle);
  char *brand = "Ford";
  stat = dlite_instance_set_property(vehicle, "brand", brand);
  mu_assert_int_eq(0, stat);
  stat = dlite_instance_save_url("json://Ford.json?mode=w", vehicle);
  mu_assert_int_eq(0, stat);
  dlite_instance_decref(vehicle);
  dlite_meta_decref(meta);

  DLiteMeta *meta2 = dlite_meta_create_from_metamodel(nodim);
  mu_check(meta2);
  //dlite_instance_print((DLiteInstance *)meta);
  dlite_instance_save_url("json://NoDimension.json?mode=w&with-uuid=yes",
                          (DLiteInstance *)meta2);

  DLiteInstance *inst = dlite_instance_create(meta2, NULL, NULL);
  mu_check(inst);
  char *name = "John";
  stat = dlite_instance_set_property(inst, "name", name);
  mu_assert_int_eq(0, stat);
  float value = 33.0;
  stat = dlite_instance_set_property(inst, "value", &value);
  mu_assert_int_eq(0, stat);
  stat = dlite_instance_save_url("json://JohnNoDim.json?mode=w", inst);
  mu_assert_int_eq(0, stat);

  dlite_instance_decref(inst);
  dlite_meta_decref(meta2);
}

MU_TEST(test_metamodel_free)
{
  dlite_metamodel_free(model);
  dlite_metamodel_free(nodim);
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
