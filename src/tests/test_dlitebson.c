#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-bson.h"

#include "minunit/minunit.h"



MU_TEST(test_write_meta)
{
  unsigned char doc[1024];
  int bufsize=sizeof(doc), n, m;
  char *path = STRINGIFY(dlite_SOURCE_DIR) "/src/tests/test-entity.json";
  DLiteInstance *inst = dlite_instance_load_loc("json", path, NULL, NULL);

  n = bson_init_document(doc, bufsize);
  m = dlite_bson_append_instance(doc, bufsize-n, inst);
  mu_check(m > 0);
  n += m;

  mu_assert_int_eq(bson_docsize(doc), n);

  FILE *fp = fopen("test-entity.bson", "wb");
  mu_check(fp);
  fwrite(doc, n, 1, fp);
  fclose(fp);
}


MU_TEST(test_write_instance)
{
  unsigned char doc[1024];
  int bufsize=sizeof(doc), n, m;
  char *path = STRINGIFY(dlite_SOURCE_DIR) "/src/tests/test-data.json";
  DLiteInstance *inst =
    dlite_instance_load_loc("json", path, NULL,
                            "204b05b2-4c89-43f4-93db-fd1cb70f54ef");

  n = bson_init_document(doc, bufsize);
  m = dlite_bson_append_instance(doc, bufsize-n, inst);
  mu_check(m > 0);
  n += m;

  mu_assert_int_eq(bson_docsize(doc), n);

  FILE *fp = fopen("test-data.bson", "wb");
  mu_check(fp);
  fwrite(doc, n, 1, fp);
  fclose(fp);

  dlite_instance_decref(inst);
}


MU_TEST(test_load_instance)
{
  unsigned char doc[1024];

  FILE *fp = fopen("test-data.bson", "rb");
  mu_check(fp);
  fread(doc, 1, sizeof(doc), fp);
  fclose(fp);

  DLiteInstance *inst = dlite_bson_load_instance(doc);
  mu_check(inst);
  printf("\n-------------\n");
  dlite_json_print(inst);

  dlite_meta_decref((DLiteMeta *)inst->meta);
  dlite_meta_decref((DLiteMeta *)inst->meta);
  dlite_instance_decref(inst);
}


MU_TEST(test_load_meta)
{
  unsigned char doc[4096];
  char *path = STRINGIFY(dlite_SOURCE_DIR) "/src/tests/Chemistry.bson";
  FILE *fp = fopen(path, "rb");
  mu_check(fp);
  fread(doc, 1, sizeof(doc), fp);
  fclose(fp);

  DLiteInstance *inst = dlite_bson_load_instance(doc);
  mu_check(inst);
  printf("\n-------------\n");
  dlite_json_print(inst);

  dlite_instance_decref(inst);
  dlite_instance_decref(inst);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_write_meta);
  MU_RUN_TEST(test_write_instance);
  MU_RUN_TEST(test_load_instance);
  MU_RUN_TEST(test_load_meta);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
