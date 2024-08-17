#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "utils/integers.h"
#include "utils/boolean.h"
#include "utils/strutils.h"
#include "dlite.h"
#include "dlite-macros.h"
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
  char *shape0[] = {"N", "M"};
  char *shape1[] = {"N"};
  char *shape2[] = {"M"};
  DLiteDimension dimensions[] = {
    {"M", "Length of dimension M."},
    {"N", "Length of dimension N."}
  };
  DLiteProperty properties[] = {
    /* name          type            size           ref ndims shape  unit descr*/
    {"a-string",     dliteStringPtr, sizeof(char *),NULL, 0, NULL,  "",  "..."},
    {"a-float",      dliteFloat,     sizeof(float), NULL, 0, NULL,  "m", ""},
    {"an-int-arr",   dliteInt,       sizeof(int),   NULL, 2, shape0, "#", "..."},
    {"a-string-arr", dliteStringPtr, sizeof(char *),NULL, 1, shape1, "",  "..."},
    {"a-string3-arr",dliteFixString, 3,             NULL, 1, shape2, "",  "..."}
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

  //dlite_instance_debug((DLiteInstance *)entity);

  /* be careful here.. the expected values are for a memory-aligned 64 bit
     system */
#if (__GNUC__ && SIZEOF_VOID_P == 8)
  mu_assert_int_eq(72, sizeof(DLiteInstance));
  mu_assert_int_eq(72, entity->_dimoffset);
  mu_assert_int_eq(72, entity->_headersize);
  mu_assert_int_eq(88, entity->_propoffsets[0]);
  mu_assert_int_eq(96, entity->_propoffsets[1]);
  mu_assert_int_eq(104, entity->_propoffsets[2]);
  mu_assert_int_eq(112, entity->_propoffsets[3]);
  mu_assert_int_eq(120, entity->_propoffsets[4]);
  mu_assert_int_eq(128, entity->_reloffset);
  mu_assert_int_eq(128, entity->_propdimsoffset);
  mu_assert_int_eq(160, entity->_propdimindsoffset);
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
  n = dlite_json_sprint(buf, sizeof(buf), inst, 2, dliteJsonSingle);
  mu_assert_int_eq(346, n);
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


MU_TEST(test_instance_get)
{
  DLiteInstance *inst;
  char *path = STRINGIFY(dlite_BINARY_DIR) "/src/tests/*.json";
  const char **p;

  printf("\nStorage paths:\n");
  for (p=dlite_storage_paths_get(); p && *p; p++)
    printf("  - %s\n", *p);
  printf("\n");

  inst = dlite_instance_get("mydata");
  mu_check(!inst);

  dlite_storage_paths_append(path);
  dlite_storage_paths_append(path);
  printf("\nStorage paths:\n");
  for (p=dlite_storage_paths_get(); *p; p++)
    printf("  - %s\n", *p);
  printf("\n");

  inst = dlite_instance_get("mydata");
  mu_check(inst);
  dlite_instance_decref(inst);
}


MU_TEST(test_instance_get_hash)
{
  char *hash;
  char s[65];
  DLiteInstance *inst = dlite_instance_load_loc("json", "myentity.json",
                                                "mode=r", "mydata");
  mu_check(inst);

  hash = "90fdd20131148fa0eaec9a21705dc0f8bc2a794945929796264c576a49b9e112";
  mu_assert_string_eq(hash, gethash(s, inst));

  // metadata
  hash = "8f7e363b3873a007f01ec1cd4ff824a7f61311c9a5eec1bf65c1db4d7bdaa5e5";
  mu_assert_string_eq(hash, gethash(s, (DLiteInstance *)inst->meta));

  // metadata schema
  hash = "729f64d23039e5a83c01d20459a824984269719b1ec3b4c07fabc76091493c1d";
  mu_assert_string_eq(hash, gethash(s, (DLiteInstance *)inst->meta->meta));

  // basic metadata schema
  hash = "19d8ad9bea47c12798167ee880fe3e17cd920c719eb4b92ab8fb7092b8d0441f";
  mu_assert_string_eq(hash, gethash(s, (DLiteInstance *)inst->meta->meta->meta));

  // basic metadata schema
  mu_assert_string_eq(hash, gethash(s, (DLiteInstance *)inst->meta->meta->meta->meta));

  dlite_instance_decref(inst);
}


MU_TEST(test_transactions)
{
  int stat;
  size_t dims[] = {1, 3};
  DLiteInstance *inst = dlite_instance_get("mydata");
  DLiteInstance *inst2 = dlite_instance_create(entity, dims, NULL);
  DLiteInstance *inst3 = dlite_instance_create(entity, dims, NULL);
  mu_check(inst);
  mu_check(inst2);
  mu_check(inst3);
  dlite_errclr();

  stat = dlite_instance_set_parent(inst2, inst);
  mu_assert_int_eq(-1, stat);
  dlite_errclr();

  mu_assert_int_eq(0, dlite_instance_is_frozen(inst));
  dlite_instance_freeze(inst);
  mu_assert_int_eq(1, dlite_instance_is_frozen(inst));
  stat = dlite_instance_set_parent(inst2, inst);
  mu_assert_int_eq(0, stat);

  dlite_instance_freeze(inst2);
  stat = dlite_instance_set_parent(inst3, inst2);
  mu_assert_int_eq(0, stat);

  mu_assert_int_eq(0, dlite_instance_verify_hash(inst3, NULL, 1));
  mu_assert_int_eq(0, dlite_instance_verify_hash(inst2,inst3->_parent->hash,0));
  mu_assert_int_eq(0, dlite_instance_verify_hash(inst,inst2->_parent->hash, 0));
  mu_assert_int_eq(1, dlite_instance_verify_hash(inst,inst3->_parent->hash, 0));

  const DLiteInstance *parent = dlite_instance_get_snapshot(inst2, 1);
  mu_assert_ptr_eq(parent, inst);

  dlite_instance_decref(inst3);
  dlite_instance_decref(inst2);
  dlite_instance_decref(inst);
}

MU_TEST(test_snapshot)
{
  /* Create a simple transaction */
  char *path = STRINGIFY(dlite_BINARY_DIR) "/src/tests/transaction_store.json";
  const DLiteInstance *snapshot;
  DLiteInstance *inst = dlite_instance_get("mydata");
  DLiteStorage *s = dlite_storage_open("json", path, "mode=w");
  mu_check(s);
  mu_check(inst);
  mu_assert_int_eq(0, dlite_instance_snapshot(inst));
  mu_assert_int_eq(0, dlite_instance_snapshot(inst));
  mu_assert_int_eq(0, dlite_instance_snapshot(inst));

  snapshot = dlite_instance_get_snapshot(inst, 0);
  mu_assert_ptr_eq(snapshot, inst);

  snapshot = dlite_instance_get_snapshot(inst, 1);
  mu_assert_ptr_eq(snapshot, inst->_parent->parent);
  printf("\n");
  printf("*** snapshot: %s\n", snapshot->uri);

  snapshot = dlite_instance_get_snapshot(inst, 3);
  dlite_json_print(snapshot);
  printf("*** snapshot: %s\n", snapshot->uri);

  mu_assert_int_eq(0, dlite_instance_verify_hash(inst, NULL, 1));
  dlite_instance_print_transaction(inst);

  /*
  mu_assert_int_eq(0, dlite_instance_push_snapshot(inst, s, 1));
  snapshot = inst->_parent->parent;
  mu_check(snapshot);
  mu_assert_string_eq("8405895c-cc7f-5b91-aee7-0b679987394d",
                   snapshot->_parent->uuid);
  mu_assert_ptr_eq(NULL, snapshot->_parent->parent);

  snapshot2 = dlite_instance_pull_snapshot(inst, s, 1);
  mu_assert_ptr_eq(snapshot2, snapshot);
  mu_assert_ptr_eq(NULL, snapshot2->_parent->parent);

  // BIG ISSUE: hash and uuid of transaction parents are not described
  // by the metadata.  Hence, is this info not serialised to the
  // storage and is therefore not restored when the data is loaded.
  snapshot2 = dlite_instance_pull_snapshot(inst, s, 2);
  mu_assert_ptr_eq(snapshot2, snapshot->_parent->parent);
  mu_assert_ptr_eq(NULL, snapshot2->_parent->parent);





  printf("\n");
  printf("snapshot 1: %s (%p)\n", snapshot->uuid, (void *)inst->_parent->parent);
  printf("snapshot 2: %s (%p)\n", snapshot->_parent->uuid,
         (void *)snapshot->_parent->parent);
  */


  //dlite_instance_save(s, inst);

  //mu_assert_int_eq(0, dlite_instance_snapshot(inst));
  //mu_assert_int_eq(0, dlite_instance_snapshot(inst));


  /*
  DLiteInstance *snapshot1 = dlite_instance_snapshot(inst);
  DLiteInstance *snapshot2 = dlite_instance_snapshot(snapshot1);
  uint8_t h0[DLITE_HASH_SIZE], h1[DLITE_HASH_SIZE], h2[DLITE_HASH_SIZE];

  mu_assert_int_eq(0, dlite_instance_is_frozen(inst));
  mu_assert_int_eq(1, dlite_instance_is_frozen(snapshot1));
  mu_assert_int_eq(1, dlite_instance_is_frozen(snapshot2));

  mu_check(snapshot1 != inst);
  mu_assert_ptr_eq(snapshot1, snapshot2);

  mu_assert_int_eq(0, dlite_instance_get_hash(inst, h0, DLITE_HASH_SIZE));
  mu_assert_int_eq(0, dlite_instance_get_hash(snapshot1, h1, DLITE_HASH_SIZE));
  mu_assert_int_eq(0, dlite_instance_get_hash(snapshot2, h2, DLITE_HASH_SIZE));

  mu_assert_int_eq(1, inst->_refcount);
  mu_assert_int_eq(2, snapshot1->_refcount);
  mu_assert_int_eq(2, snapshot2->_refcount);

  dlite_instance_decref(snapshot1);
  dlite_instance_decref(snapshot2);
  */
  dlite_storage_close(s);
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

  // will fail if entity_schema.json already exists, since mode=w is not given
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
  MU_RUN_TEST(test_instance_get);
  MU_RUN_TEST(test_instance_get_hash);
  MU_RUN_TEST(test_transactions);
  MU_RUN_TEST(test_snapshot);

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
