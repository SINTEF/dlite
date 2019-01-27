#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
<<<<<<< ALP-42-tmp
#include "integers.h"
#include "boolean.h"
=======
#include "utils/integers.h"
#include "utils/boolean.h"
>>>>>>> local
#include "dlite.h"
#include "dlite-entity.h"
#include "dlite-storage.h"


#include "config.h"

char *datafile = "myentity.h5";
char *datafile2 = "myentity2.h5";
char *jsonfile = "myentity.json";
char *jsonfile2 = "myentity2.json";
char *uri = "http://www.sintef.no/meta/dlite/0.1/MyEntity";
char *id = "mydata";
DLiteMeta *entity=NULL;
DLiteInstance *mydata=NULL, *mydata2=NULL, *mydata3=NULL;


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
    /* name           type            size         ndims dims   unit descr */
    {"a-string",      dliteStringPtr, sizeof(char *), 0, NULL,  "",  "..."},
    {"a-float",       dliteFloat,     sizeof(float),  0, NULL,  "m", ""},
    {"an-int-arr",    dliteInt,       sizeof(int),    2, dims0, "#", "descr.."},
    {"a-string-arr",  dliteStringPtr, sizeof(char *), 1, dims1, "",  "descr.."},
    {"a-string3-arr", dliteFixString, 3,              1, dims2, "",  "descr.."}
  };

  mu_check((entity = (DLiteMeta *)dlite_entity_create(uri, "My test entity.",
                                                      2, dimensions,
                                                      5, properties)));
  mu_assert_int_eq(2, entity->ndimensions);
  mu_assert_int_eq(5, entity->nproperties);
  mu_assert_int_eq(1, entity->properties[2].dims[0]);
  mu_assert_int_eq(0, entity->properties[2].dims[1]);

  /* be careful here.. the expected values are for a memory-aligned 64 bit
     system */
#if (__GNUC__ && SIZEOF_VOID_P == 8)
  mu_assert_int_eq(64, sizeof(DLiteInstance));
  mu_assert_int_eq(64, entity->dimoffset);
  mu_assert_int_eq(80, entity->propoffsets[0]);
  mu_assert_int_eq(88, entity->propoffsets[1]);
  mu_assert_int_eq(96, entity->propoffsets[2]);
  mu_assert_int_eq(104, entity->propoffsets[3]);
  mu_assert_int_eq(112, entity->propoffsets[4]);
  mu_assert_int_eq(120, entity->reloffset);
  mu_assert_int_eq(120, entity->pooffset);
  //mu_assert_int_eq(160, entity->size);
#endif
}

MU_TEST(test_instance_create)
{
  size_t dims[]={3, 2};
  mu_check((mydata = dlite_instance_create(entity, dims, id)));
}

MU_TEST(test_instance_set_property)
{
  char *astring="string value";
  float afloat=3.14f;
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
#ifdef WITH_HDF5
  mu_check((s = dlite_storage_open("hdf5", datafile, "mode=w")));
  mu_check(dlite_instance_save(s, mydata) == 0);
  mu_check(dlite_storage_close(s) == 0);
#endif
#ifdef WITH_JSON
  mu_check((s = dlite_storage_open("json", jsonfile, "mode=w")));
  mu_check(dlite_instance_save(s, mydata) == 0);
  mu_check(dlite_storage_close(s) == 0);
#endif
}

MU_TEST(test_instance_load)
{
  DLiteStorage *s;
#ifdef WITH_HDF5
  mu_check((s = dlite_storage_open("hdf5", datafile, "mode=r")));
  mu_check((mydata2 = dlite_instance_load(s, id)));
  mu_check(dlite_storage_close(s) == 0);
#endif
#ifdef WITH_JSON
  mu_check((s = dlite_storage_open("json", jsonfile, "mode=r")));
  mu_check((mydata3 = dlite_instance_load(s, id)));
  mu_check(dlite_storage_close(s) == 0);
#endif
}

MU_TEST(test_instance_save2)
{
  DLiteStorage *s;
#ifdef WITH_HDF5
  mu_check((s = dlite_storage_open("hdf5", datafile2, "mode=w")));
  mu_check(dlite_instance_save(s, mydata2) == 0);
  mu_check(dlite_storage_close(s) == 0);
#endif
#ifdef WITH_JSON
  mu_check((s = dlite_storage_open("json", jsonfile2, "mode=w")));
  mu_check(dlite_instance_save(s, mydata3) == 0);
  mu_check(dlite_storage_close(s) == 0);
#endif
}

MU_TEST(test_instance_load_url)
{
  DLiteInstance *inst;
#ifdef WITH_JSON
  mu_check((inst = dlite_instance_load_url("json://myentity.json#mydata")));
  mu_check(0 == dlite_instance_save_url("json://myentity6.json?mode=w", inst));
  mu_check(dlite_instance_decref(inst));
#endif
}

MU_TEST(test_instance_copy)
{
  DLiteStorage *s;
  DLiteInstance *inst;
  mu_check((inst = dlite_instance_copy(mydata, NULL)));
#ifdef WITH_JSON
  mu_check((s = dlite_storage_open("json", "myentity_copy.json", "mode=w")));
  mu_check(dlite_instance_save(s, inst) == 0);
  mu_check(dlite_storage_close(s) == 0);
#endif
  dlite_instance_decref(inst);

}

MU_TEST(test_instance_set_dimension_sizes)
{
  DLiteStorage *s;
  int newdims1[] = {-1, 4};
  int newdims2[] = {2, 1};

  mu_check(dlite_instance_set_dimension_sizes(mydata, newdims1) == 0);
#ifdef WITH_JSON
  mu_check((s = dlite_storage_open("json", "myentity4.json", "mode=w")));
  mu_check(dlite_instance_save(s, mydata) == 0);
  mu_check(dlite_storage_close(s) == 0);
#endif

  mu_check(dlite_instance_set_dimension_sizes(mydata, newdims2) == 0);
#ifdef WITH_JSON
  mu_check((s = dlite_storage_open("json", "myentity5.json", "mode=w")));
  mu_check(dlite_instance_save(s, mydata) == 0);
  mu_check(dlite_storage_close(s) == 0);
#endif
}

MU_TEST(test_instance_free)
{
  if (mydata)  dlite_instance_decref(mydata);
  if (mydata2) dlite_instance_decref(mydata2);
  if (mydata3) dlite_instance_decref(mydata3);
}

MU_TEST(test_meta_save)
{
  DLiteStorage *s;
#ifdef WITH_JSON
  mu_check((s = dlite_storage_open("json", "MyEntity.json", "mode=w;meta=1")));
  mu_check(dlite_meta_save(s, entity) == 0);
  mu_check(dlite_storage_close(s) == 0);
#endif

#ifdef WITH_JSON
  mu_check((s = dlite_storage_open("json", "MyEntity2.json", "mode=w;meta=0")));
  mu_check(dlite_meta_save(s, entity) == 0);
  mu_check(dlite_storage_close(s) == 0);
#endif
  //mu_check((s = dlite_storage_open("hdf5", "MyEntity.h5", "mode=w")));
  //mu_check(dlite_entity_save(s, entity) == 0);
  //mu_check(dlite_storage_close(s) == 0);
}

MU_TEST(test_meta_load)
{
  DLiteStorage *s;
  DLiteMeta *e, *e2;
#ifdef WITH_JSON
  mu_check((s = dlite_storage_open("json", "MyEntity.json", "mode=r")));
  mu_check((e = dlite_meta_load(s, uri)));
  mu_check(dlite_storage_close(s) == 0);

  mu_check((s = dlite_storage_open("json", "MyEntity2.json", "mode=r")));
  mu_check((e2 = dlite_meta_load(s, uri)));
  mu_check(dlite_storage_close(s) == 0);

  mu_check((s = dlite_storage_open("json", "MyEntity3.json", "mode=w;meta=1")));
  mu_check(dlite_meta_save(s, e) == 0);
  mu_check(dlite_storage_close(s) == 0);

  mu_check((s = dlite_storage_open("json", "MyEntity4.json", "mode=w;meta=1")));
  mu_check(dlite_meta_save(s, e2) == 0);
  mu_check(dlite_storage_close(s) == 0);

  dlite_meta_decref(e);
  dlite_meta_decref(e2);
#endif
}


MU_TEST(test_meta_free)
{
  dlite_meta_decref(entity);
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
  MU_RUN_TEST(test_instance_load_url);
  MU_RUN_TEST(test_instance_copy);
  MU_RUN_TEST(test_instance_set_dimension_sizes);
  MU_RUN_TEST(test_instance_free);

  MU_RUN_TEST(test_meta_save);
  MU_RUN_TEST(test_meta_load);
  MU_RUN_TEST(test_meta_free);     /* tear down */
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
