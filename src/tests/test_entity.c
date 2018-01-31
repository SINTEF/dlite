#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "integers.h"
#include "boolean.h"
#include "dlite.h"
#include "dlite-entity.h"
#include "dlite-storage.h"


#include "config.h"

char *datafile = "myentity.h5";
char *datafile2 = "myentity2.h5";
char *uri = "http://www.sintef.no/meta/dlite/0.1/MyEntity";
char *id = "mydata";
DLiteEntity *entity=NULL;
DLiteInstance *mydata=NULL, *mydata2=NULL;


/***************************************************************
 * Test entity
 ***************************************************************/

MU_TEST(test_entity_create)
{
  int dims0[] = {1, 0};  /* [N, M] */
  int dims1[] = {1};     /* [N] */
  int dims2[] = {0};     /* [M] */
  DLiteDimension dimensions[] = {
    {"M", "Length of dimension M."},
    {"N", "Length of dimension N."}
  };
  DLiteProperty properties[] = {
    {"a-string",        dliteStringPtr, sizeof(char *), 0, NULL,  "",  "a"},
    {"a-float",         dliteFloat,     sizeof(double), 0, NULL,  "m", "b"},
    {"an-int-arr",      dliteInt,       sizeof(int),    2, dims0, "#", "c"},
    {"a-string-arr",    dliteStringPtr, sizeof(char *), 1, dims1, "",  "d"},
    {"a-string3-arr",   dliteFixString, 3,              1, dims2, "",  "e"}
  };

  mu_check((entity = dlite_entity_create(uri, "My test entity.",
					 2, dimensions,
					 5, properties)));
  mu_assert_int_eq(2, entity->ndimensions);
  mu_assert_int_eq(5, entity->nproperties);
  mu_assert_int_eq(1, entity->properties[2]->dims[0]);
  mu_assert_int_eq(0, entity->properties[2]->dims[1]);
  mu_assert_int_eq(56, sizeof(DLiteInstance));
  mu_assert_int_eq(56, entity->dimoffset);
  mu_assert_int_eq(72, entity->propoffsets[0]);
  mu_assert_int_eq(80, entity->propoffsets[1]);
  mu_assert_int_eq(88, entity->propoffsets[2]);
  mu_assert_int_eq(96, entity->propoffsets[3]);
  mu_assert_int_eq(104, entity->propoffsets[4]);
  mu_assert_int_eq(104, entity->reloffset);
  mu_assert_int_eq(112, entity->size);
}

MU_TEST(test_instance_create)
{
  size_t dims[]={3, 2};
  mu_check((mydata = dlite_instance_create(entity, dims, id)));
}

MU_TEST(test_instance_set_property)
{
  char *astring="string value";
  double afloat=3.14;
  int intarr[2][3] = {{0, 1, 2}, {3, 4, 5}};
  char *strarr[] = {"first string", "second string"};
  char str3arr[3][3] = {"Al", "Mg", "Si"};
  mu_check(dlite_instance_set_property(mydata, "a-string", &astring) == 0);
  mu_check(dlite_instance_set_property(mydata, "a-float", &afloat) == 0);
  mu_check(dlite_instance_set_property(mydata, "an-int-arr", intarr) == 0);
  mu_check(dlite_instance_set_property(mydata, "a-string-arr", strarr) == 0);
  mu_check(dlite_instance_set_property(mydata, "a-string3-arr", str3arr) == 0);
}

MU_TEST(test_instance_get_dimension_size)
{
  mu_assert_int_eq(3, dlite_instance_get_dimension_size_by_index(mydata, 0));
  mu_assert_int_eq(2, dlite_instance_get_dimension_size_by_index(mydata, 1));

  mu_assert_int_eq(3, dlite_instance_get_dimension_size(mydata, "M"));
  mu_assert_int_eq(2, dlite_instance_get_dimension_size(mydata, "N"));
}

MU_TEST(test_instance_save)
{
  DLiteStorage *s;
  mu_check((s = dlite_storage_open("hdf5", datafile, "w")));
  mu_check(dlite_instance_save(s, mydata) == 0);
  mu_check(dlite_storage_close(s) == 0);
}

MU_TEST(test_instance_load)
{
  DLiteStorage *s;
  mu_check((s = dlite_storage_open("hdf5", datafile, "r")));
  mu_check((mydata2 = dlite_instance_load(s, id, entity)));
  mu_check(dlite_storage_close(s) == 0);
}

MU_TEST(test_instance_save2)
{
  DLiteStorage *s;
  mu_check((s = dlite_storage_open("hdf5", datafile2, "w")));
  mu_check(dlite_instance_save(s, mydata2) == 0);
  mu_check(dlite_storage_close(s) == 0);
}

MU_TEST(test_instance_free)
{
  dlite_instance_free(mydata);
  dlite_instance_free(mydata2);
}

/*
MU_TEST(test_storage_save_entity)
{
  DLiteStorage *s;
  mu_check((s = dlite_storage_open("hdf5", datafile, "w")));
  mu_check(dlite_storage_entity_save(s, entity) == 0);
  mu_check(dlite_storage_close(s) == 0);
}

MU_TEST(test_entity_load)
{
  DLiteStorage *s;
  DLiteEntity *e;
  mu_check((s = dlite_storage_open("hdf5", datafile, "r")));
  mu_check((e = dlite_storage_load_entity(s, id)));
  mu_check(dlite_storage_close(s) == 0);

  mu_check((s = dlite_storage_open("hdf5", "myentity2.h5", "w")));
  mu_check(dlite_entity_save(s, entity) == 0);
  mu_check(dlite_storage_close(s) == 0);
}
*/

MU_TEST(test_entity_free)
{
  dlite_entity_decref(entity);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_entity_create);    /* setup */
  MU_RUN_TEST(test_instance_create);
  MU_RUN_TEST(test_instance_get_dimension_size);
  MU_RUN_TEST(test_instance_set_property);
  MU_RUN_TEST(test_instance_save);
  MU_RUN_TEST(test_instance_load);
  MU_RUN_TEST(test_instance_save2);
  MU_RUN_TEST(test_instance_free);

  /*
  MU_RUN_TEST(test_entity_save);
  MU_RUN_TEST(test_entity_load);
  */
  MU_RUN_TEST(test_entity_free);     /* tear down */
}



int main(int argc, char *argv[])
{
  if (argc > 1) datafile = argv[1];
  if (argc > 2) id = argv[2];
  printf("datafile: '%s'\n", datafile);
  printf("id:       '%s'\n", id);

  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
