#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "utils/integers.h"
#include "utils/boolean.h"
#include "utils/strutils.h"
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

MU_TEST(test_meta_create)
{
  char *dims0[] = {"N", "M"};
  char *dims1[] = {"N"};
  char *dims2[] = {"M"};
  DLiteDimension dimensions[] = {
    {"M", "Length of dimension M."},
    {"N", "Length of dimension N."}
  };
  DLiteProperty properties[] = {
    /* name          type            size           ref ndims dims  unit descr*/
    {"a-string",     dliteStringPtr, sizeof(char *),NULL, 0, NULL,  "",  "..."},
    {"a-float",      dliteFloat,     sizeof(float), NULL, 0, NULL,  "m", ""},
    {"an-int-arr",   dliteInt,       sizeof(int),   NULL, 2, dims0, "#", "..."},
    {"a-string-arr", dliteStringPtr, sizeof(char *),NULL, 1, dims1, "",  "..."},
    {"a-string3-arr",dliteFixString, 3,             NULL, 1, dims2, "",  "..."}
  };

  mu_check((entity = (DLiteMeta *)dlite_meta_create(uri, "My test entity.",
                                                    2, dimensions,
                                                    5, properties)));
  mu_assert_int_eq(2, entity->_refcount);  /* refs: global+store */

  mu_assert_int_eq(2, entity->_ndimensions);
  mu_assert_int_eq(5, entity->_nproperties);
  mu_assert_int_eq(2, DLITE_PROP_DIM(entity->meta, 4, 0));
  mu_assert_int_eq(6, DLITE_PROP_DIM(entity->meta, 5, 0));

  mu_assert_int_eq(0, dlite_instance_is_data((DLiteInstance *)entity));
  mu_assert_int_eq(1, dlite_instance_is_meta((DLiteInstance *)entity));
  mu_assert_int_eq(0, dlite_instance_is_metameta((DLiteInstance *)entity));

  dlite_instance_debug((DLiteInstance *)entity);

  /* be careful here.. the expected values are for a memory-aligned 64 bit
     system */
#if (__GNUC__ && SIZEOF_VOID_P == 8)
  mu_assert_int_eq(64, sizeof(DLiteInstance));
  mu_assert_int_eq(64, entity->_dimoffset);
  mu_assert_int_eq(64, entity->_headersize);
  mu_assert_int_eq(80, entity->_propoffsets[0]);
  mu_assert_int_eq(88, entity->_propoffsets[1]);
  mu_assert_int_eq(96, entity->_propoffsets[2]);
  mu_assert_int_eq(104, entity->_propoffsets[3]);
  mu_assert_int_eq(112, entity->_propoffsets[4]);
  mu_assert_int_eq(120, entity->_reloffset);
  mu_assert_int_eq(120, entity->_propdimsoffset);
  mu_assert_int_eq(152, entity->_propdimindsoffset);
#endif
}

MU_TEST(test_instance_create)
{
  size_t dims[]={3, 2};
  mu_check((mydata = dlite_instance_create(entity, dims, id)));
  mu_assert_int_eq(1, mydata->_refcount);
  mu_assert_int_eq(3, entity->_refcount);  /* refs: global+store+mydata */
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
  mu_assert_int_eq(1, mydata->_refcount);
  mu_assert_int_eq(3, entity->_refcount);  /* refs: global+store+mydata */
}

MU_TEST(test_instance_get_dimension_size)
{
  mu_assert_int_eq(3, dlite_instance_get_dimension_size_by_index(mydata, 0));
  mu_assert_int_eq(2, dlite_instance_get_dimension_size_by_index(mydata, 1));

  mu_assert_int_eq(3, dlite_instance_get_dimension_size(mydata, "M"));
  mu_assert_int_eq(2, dlite_instance_get_dimension_size(mydata, "N"));
  mu_assert_int_eq(1, mydata->_refcount);
  mu_assert_int_eq(3, entity->_refcount);  /* refs: global+store+mydata */
}

MU_TEST(test_instance_set_dimension_sizes)
{
  DLiteStorage *s;
  int newdims1[] = {-1, 4};
  int newdims2[] = {2, 1};

  mu_check(dlite_instance_set_dimension_sizes(mydata, newdims1) == 0);
  mu_check((s = dlite_storage_open("json", "myentity4.json", "mode=w")));
  mu_check(dlite_instance_save(s, mydata) == 0);
  mu_check(dlite_storage_close(s) == 0);

  mu_check(dlite_instance_set_dimension_sizes(mydata, newdims2) == 0);
  mu_check((s = dlite_storage_open("json", "myentity5.json", "mode=w")));
  mu_check(dlite_instance_save(s, mydata) == 0);
  mu_check(dlite_storage_close(s) == 0);

  mu_assert_int_eq(1, mydata->_refcount);
  mu_assert_int_eq(3, entity->_refcount);  /* refs: global+store+mydata */
}

MU_TEST(test_instance_copy)
{
  DLiteStorage *s;
  DLiteInstance *inst;
  mu_assert_int_eq(1, mydata->_refcount);
  mu_check((inst = dlite_instance_copy(mydata, NULL)));
  mu_assert_int_eq(1, mydata->_refcount);

  mu_check((s = dlite_storage_open("json", "myentity_copy.json", "mode=w")));
  mu_check(dlite_instance_save(s, inst) == 0);
  mu_check(dlite_storage_close(s) == 0);

  mu_assert_int_eq(1, mydata->_refcount);
  mu_assert_int_eq(1, inst->_refcount);
  dlite_instance_decref(inst);
  mu_assert_int_eq(3, entity->_refcount);  /* refs: global+store+mydata */
}


MU_TEST(test_instance_print_property)
{
  DLiteInstance *inst;
  int n;
  char buf[1024];
  inst = dlite_instance_load_url("json://myentity.json?mode=r#mydata");
  mu_check(inst);

  /* test dlite_instance_print_property() */
  n = dlite_instance_print_property(buf, sizeof(buf), inst, "a-float",
                                    0, -2, 0);
  mu_assert_int_eq(4, n);
  mu_assert_string_eq("3.14", buf);

  n = dlite_instance_print_property(buf, sizeof(buf), inst, "a-float",
                                    2, -2, 0);
  mu_assert_int_eq(4, n);
  mu_assert_string_eq("3.14", buf);

  n = dlite_instance_print_property(buf, sizeof(buf), inst, "a-float",
                                    8, -2, 0);
  mu_assert_int_eq(8, n);
  mu_assert_string_eq("    3.14", buf);

  n = dlite_instance_print_property(buf, 2, inst, "a-float",
                                    0, -2, 0);
  mu_assert_int_eq(4, n);
  mu_assert_string_eq("3", buf);

  n = dlite_instance_print_property(buf, sizeof(buf), inst, "a-float",
                                    0, 2, 0);
  mu_assert_int_eq(3, n);
  mu_assert_string_eq("3.1", buf);

  n = dlite_instance_print_property(buf, sizeof(buf), inst, "a-string-arr",
                                    0, -2, 0);
  mu_assert_int_eq(16, n);
  mu_assert_string_eq("[\"first string\"]", buf);

  n = dlite_instance_print_property(buf, sizeof(buf), inst, "a-string-arr",
                                    0, -2, dliteFlagRaw);
  mu_assert_int_eq(14, n);
  mu_assert_string_eq("[first string]", buf);

  n = dlite_instance_print_property(buf, sizeof(buf), inst, "a-string-arr",
                                    0, -2, dliteFlagQuoted);
  mu_assert_int_eq(16, n);
  mu_assert_string_eq("[\"first string\"]", buf);

  /* test dlite_instance_aprint_property() */
  char *q=NULL;
  size_t size=0;
  n = dlite_instance_aprint_property(&q, &size, 0, inst, "a-string-arr",
                                     0, -2, 0);
  mu_assert_int_eq(16, n);
  mu_assert_int_eq(17, size);
  mu_assert_string_eq("[\"first string\"]", q);

  n = dlite_instance_aprint_property(&q, &size, 2, inst, "a-string",
                                     0, -2, 0);
  mu_assert_int_eq(14, n);
  mu_assert_int_eq(17, size);
  mu_assert_string_eq("[\"\"string value\"", q);

  free(q);

  /* test dlite_instance_scan_property() */
  void *ptr;

  n = dlite_instance_scan_property("123.456", inst, "a-float", 0);
  ptr = dlite_instance_get_property(inst, "a-float");
  double v = *((float *)ptr);
  mu_assert_int_eq(7, n);
  mu_assert_float_eq(123.456, v);

  n = dlite_instance_scan_property("[\"a longer string value\"]", inst,
                                   "a-string-arr", 0);
  ptr = dlite_instance_get_property(inst, "a-string-arr");
  char **strarr = ptr;
  mu_assert_int_eq(25, n);
  mu_assert_string_eq("a longer string value", strarr[0]);

  n = dlite_instance_scan_property("[[-1, 123]]", inst, "an-int-arr", 0);
  ptr = dlite_instance_get_property(inst, "an-int-arr");
  int *intarr = ptr;
  mu_assert_int_eq(11, n);
  mu_assert_int_eq(-1, intarr[0]);
  mu_assert_int_eq(123, intarr[1]);

  dlite_instance_decref(inst);
}


MU_TEST(test_instance_save)
{
  DLiteStorage *s;
#ifdef WITH_HDF5
  mu_check((s = dlite_storage_open("hdf5", datafile, "mode=w")));
  mu_check(dlite_instance_save(s, mydata) == 0);
  mu_check(dlite_storage_close(s) == 0);
#endif

  mu_check((s = dlite_storage_open("json", jsonfile, "mode=w")));
  mu_check(dlite_instance_save(s, mydata) == 0);
  mu_check(dlite_storage_close(s) == 0);

  mu_assert_int_eq(1, mydata->_refcount);
  mu_assert_int_eq(0, dlite_instance_decref(mydata));
  mu_assert_int_eq(2, entity->_refcount);  /* refs: global+store */
}

MU_TEST(test_instance_hdf5)
{
#ifdef WITH_HDF5
  DLiteStorage *s;
  mu_check((s = dlite_storage_open("hdf5", datafile, "mode=r")));
  mu_check((mydata2 = dlite_instance_load(s, id)));
  mu_check(dlite_storage_close(s) == 0);

  mu_check((s = dlite_storage_open("hdf5", datafile2, "mode=w")));
  mu_check(dlite_instance_save(s, mydata2) == 0);
  mu_check(dlite_storage_close(s) == 0);

  mu_assert_int_eq(1, mydata2->_refcount);
  mu_assert_int_eq(0, dlite_instance_decref(mydata2));
#endif
  mu_assert_int_eq(2, entity->_refcount);  /* refs: global+store */
}

MU_TEST(test_instance_json)
{
  DLiteStorage *s;
  mu_check((s = dlite_storage_open("json", jsonfile, "mode=r")));
  mu_check((mydata3 = dlite_instance_load(s, id)));
  mu_check(dlite_storage_close(s) == 0);

  mu_check((s = dlite_storage_open("json", jsonfile2, "mode=w")));
  mu_check(dlite_instance_save(s, mydata3) == 0);
  mu_check(dlite_storage_close(s) == 0);

  mu_assert_int_eq(1, mydata3->_refcount);
  mu_assert_int_eq(0, dlite_instance_decref(mydata3));
  mu_assert_int_eq(2, entity->_refcount);  /* refs: global+store */
}

MU_TEST(test_instance_load_url)
{
  DLiteInstance *inst;
  mu_check((inst = dlite_instance_load_url("json://myentity.json#mydata")));
  mu_check(0 == dlite_instance_save_url("json://myentity6.json?mode=w", inst));
  mu_assert_int_eq(0, dlite_instance_decref(inst));
  mu_assert_int_eq(2, entity->_refcount);  /* refs: global+store */
}

MU_TEST(test_instance_snprint)
{
  DLiteInstance *inst;
  int n;
  char buf[1024];
  inst = dlite_instance_load_url("json://myentity.json?mode=r#mydata");
  mu_check(inst);
  n = dlite_json_sprint(buf, sizeof(buf), inst, 2, 0);
  printf("\n=========================================================\n");
  printf("%s\n", buf);
  printf("=========================================================\n");
  printf("n=%d\n", n);
  //mu_assert_int_eq(22, n);
  dlite_instance_decref(inst);
}


/* Write hex encoded hash to string `s`, which must  be at least 65 bytes. */
static char *gethash(char *s, DLiteInstance *inst)
{
  unsigned char buf[32];
  size_t hashsize = sizeof(buf);
  dlite_instance_get_hash(inst, buf, hashsize);
  if (strhex_encode(s, 65, buf, hashsize) < 0) return NULL;
  return s;
}


MU_TEST(test_instance_get_hash)
{
  char *hash;
  char s[65];
  DLiteInstance *inst = dlite_instance_get("mydata");
  mu_check(inst);

  hash = "90fdd20131148fa0eaec9a21705dc0f8bc2a794945929796264c576a49b9e112";
  mu_assert_string_eq(hash, gethash(s, inst));

  // metadata
  hash = "e0c1a877a0aaa47369fda3beacbe34a47feb297282f308befd7d6fad8748f9c3";
  mu_assert_string_eq(hash, gethash(s, (DLiteInstance *)inst->meta));

  // metadata schema
  hash = "642932d7cb62d6f6c11adce773c4fcab88b5c4b8668707db999ce3c9464c89c2";
  mu_assert_string_eq(hash, gethash(s, (DLiteInstance *)inst->meta->meta));

  // basic metadata schema
  hash = "fc7a634b6f98306b04bfd44f94bddb1d2a29970e8c0314e4d6cc977e8d6920da";
  mu_assert_string_eq(hash, gethash(s, (DLiteInstance *)inst->meta->meta->meta));

  // basic metadata schema
  mu_assert_string_eq(hash, gethash(s, (DLiteInstance *)inst->meta->meta->meta->meta));

  dlite_instance_decref(inst);
}


MU_TEST(test_meta_save)
{
  DLiteStorage *s;

  mu_check((s = dlite_storage_open("json", "MyEntity.json", "mode=w")));
  mu_check(dlite_meta_save(s, entity) == 0);
  mu_check(dlite_storage_close(s) == 0);

  mu_check((s = dlite_storage_open("json", "MyEntity2.json",
                                   "mode=w;with-uuid=0")));
  mu_check(dlite_meta_save(s, entity) == 0);
  mu_check(dlite_storage_close(s) == 0);
  mu_assert_int_eq(2, entity->_refcount);  /* refs: global+store */

  DLiteInstance *schema = dlite_instance_get(DLITE_ENTITY_SCHEMA);
  dlite_instance_save_url("json://entity_schema.json", schema);
}

MU_TEST(test_meta_load)
{
  DLiteStorage *s;
  DLiteMeta *e, *e2;

  mu_check((s = dlite_storage_open("json", "MyEntity.json", "mode=r")));
  mu_check((e = dlite_meta_load(s, uri)));
  mu_check(dlite_storage_close(s) == 0);
  mu_assert_int_eq(3, entity->_refcount);  /* refs: global+store+e */

  mu_check((s = dlite_storage_open("json", "MyEntity2.json", "mode=r")));
  mu_check((e2 = dlite_meta_load(s, uri)));
  mu_check(dlite_storage_close(s) == 0);
  mu_assert_int_eq(4, entity->_refcount);  /* refs: global+store+e+e2 */

  mu_check((s = dlite_storage_open("json", "MyEntity3.json",
                                   "mode=w;with-uuid=1")));
  mu_check(dlite_meta_save(s, e) == 0);
  mu_check(dlite_storage_close(s) == 0);
  mu_assert_int_eq(4, entity->_refcount);  /* refs: global+store+e+e2 */

  mu_check((s = dlite_storage_open("json", "MyEntity4.json",
                                   "mode=w;with-uuid=1")));
  mu_check(dlite_meta_save(s, e2) == 0);
  mu_check(dlite_storage_close(s) == 0);
  mu_assert_int_eq(4, entity->_refcount);  /* refs: global+store+e+e2 */

  dlite_meta_decref(e);
  dlite_meta_decref(e2);

  mu_assert_int_eq(2, entity->_refcount);  /* refs: global+store */
}


MU_TEST(test_meta_free)
{
  dlite_meta_decref(entity);  /* refs: store */
  dlite_meta_decref(entity);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_meta_create);    /* setup */
  MU_RUN_TEST(test_instance_create);
  MU_RUN_TEST(test_instance_set_property);
  MU_RUN_TEST(test_instance_get_dimension_size);
  MU_RUN_TEST(test_instance_set_dimension_sizes);
  MU_RUN_TEST(test_instance_copy);
  MU_RUN_TEST(test_instance_print_property);
  MU_RUN_TEST(test_instance_save);
  MU_RUN_TEST(test_instance_hdf5);
  MU_RUN_TEST(test_instance_json);
  MU_RUN_TEST(test_instance_load_url);
  MU_RUN_TEST(test_instance_snprint);
  MU_RUN_TEST(test_instance_get_hash);

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
