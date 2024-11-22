#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "utils/integers.h"
#include "dlite.h"
#include "dlite-macros.h"

#include "minunit/minunit.h"

#define THISDIR STRINGIFY(dlite_SOURCE_DIR) "/src/tests/"
#define PREFIX  "json://" THISDIR

DLiteInstance *inst=NULL, *coll=NULL;
DLiteMeta *meta=NULL;


MU_TEST(test_load)
{
  char *url;
  url = PREFIX "test-entity.json?mode=r";  // cppcheck-suppress unknownMacro
  meta = dlite_meta_load_url(url);
  mu_check(meta);

  url = PREFIX "test-data.json?mode=r#e076a856-e36e-5335-967e-2f2fd153c17d";
  inst = dlite_instance_load_url(url);
  mu_check(inst);

  url = PREFIX "test-collection.json?mode=r#58432e52-ee57-43b0-9daf-ef37e696da25";
  coll = dlite_instance_load_url(url);
  mu_check(coll);
}


MU_TEST(test_sprint)
{
  char buf[4096];
  int m;

  /* soft7 format: dliteJsonArrays unset */
  m = dlite_json_sprint(buf, sizeof(buf), (DLiteInstance *)meta, 0,
                        dliteJsonSingle);
  //printf("\n--------------------------------------------------------\n");
  //printf("%s\n", buf);
  mu_assert_int_eq(799, m);

  m = dlite_json_sprint(buf, sizeof(buf), (DLiteInstance *)meta, 2,
                        dliteJsonWithUuid | dliteJsonSingle);
  //printf("\n--------------------------------------------------------\n");
  //printf("%s\n", buf);
  mu_assert_int_eq(923, m);

  m = dlite_json_sprint(buf, sizeof(buf), inst, 4, dliteJsonSingle);
  //printf("\n--------------------------------------------------------\n");
  //printf("%s\n", buf);
  mu_assert_int_eq(404, m);

  m = dlite_json_sprint(buf, 80, inst, 4, dliteJsonSingle);
  //printf("\n--------------------------------------------------------\n");
  //printf("%s\n", buf);
  mu_assert_int_eq(404, m);

  m = dlite_json_sprint(buf, sizeof(buf), inst, 0, 0);
  //printf("\n--------------------------------------------------------\n");
  //printf("<%.*s>\n", m, buf);
  mu_assert_int_eq(415, m);



  /* soft5 format: dliteJsonArrays set */
  m = dlite_json_sprint(buf, sizeof(buf), (DLiteInstance *)meta, 0,
                        dliteJsonArrays | dliteJsonSingle);
  //printf("\n--------------------------------------------------------\n");
  //printf("%s\n", buf);
  mu_assert_int_eq(1011, m);

  m = dlite_json_sprint(buf, sizeof(buf), (DLiteInstance *)meta, 2,
                        dliteJsonWithUuid | dliteJsonArrays | dliteJsonSingle);
  //printf("\n--------------------------------------------------------\n");
  //printf("%s\n", buf);
  mu_assert_int_eq(1165, m);

  //printf("\n========================================================\n");


  /* Tests for PR #541 */
  m = dlite_json_sprint(buf, 0, inst, 4, dliteJsonSingle);
  mu_assert_int_eq(404, m);

  m = dlite_json_sprint(NULL, 0, inst, 4, dliteJsonSingle);
  mu_assert_int_eq(404, m);


  /* More tests for issue #543 */
  m = dlite_json_sprint(NULL, 0, coll, 0, dliteJsonCompactRel);
  mu_assert_int_eq(406, m);

  m = dlite_json_sprint(buf, sizeof(buf), coll, 0, dliteJsonCompactRel);
  mu_assert_int_eq(406, m);

  m = dlite_json_sprint(buf, sizeof(buf), coll, 0, 0);
  mu_assert_int_eq(446, m); //422


  /* Tests for proper quoting */
  DLiteCollection *coll = dlite_collection_create(NULL);
  dlite_collection_add_relation(coll, "s", "p", "\"o\"", NULL);
  m = dlite_json_sprint(buf, sizeof(buf), (DLiteInstance *)coll, 2, 0);
  const DLiteRelation *rel = dlite_collection_find_first(coll, "s", "p", NULL,
                                                         NULL);
  mu_assert_string_eq("\"o\"", rel->o);
  dlite_instance_decref((DLiteInstance *)coll);
  //printf("\n--------------------------------------------------------\n");
  //printf("%s\n", buf);


  //printf("\n========================================================\n");
}


int append(const char *str)
{
  char *s = strdup(str);
  size_t size = strlen(s) + 1;
  int m, retval=0;
  //printf("\n--- append: '%s' ---\n", str);
  m = dlite_json_append(&s, &size, inst, 0);
  dlite_errclr();
  if (m < 0) retval=m;
  //printf("%s", s);
  free(s);
  return retval;
}

MU_TEST(test_append)
{
  mu_assert_int_eq(0, append("{}"));
  mu_assert_int_eq(0, append("{ \t}"));
  mu_assert_int_eq(0, append("{\"a\": 1, \"b\": [2, 3]}"));
  mu_assert_int_eq(0, append("{\"a\": 1, \"b\": [2, 3] }"));
  mu_assert_int_eq(0, append("{\"a\": 1, }"));  // be forgiving
  mu_assert_int_eq(-1, append(""));
  mu_assert_int_eq(-1, append(" "));
  mu_assert_int_eq(-1, append("1"));
  mu_assert_int_eq(-1, append("[1, ]"));
  mu_assert_int_eq(-1, append(","));
  mu_assert_int_eq(-1, append("{"));
  mu_assert_int_eq(-1, append("[ "));
}


MU_TEST(test_decref)
{
  int n;
  for (n=coll->_refcount; n>0; n--) dlite_instance_decref(coll);
  for (n=inst->_refcount; n>0; n--) dlite_instance_decref(inst);
  for (n=meta->_refcount; n>1; n--) dlite_meta_decref(meta);  // metadata
  coll = NULL;
  inst = NULL;
  meta = NULL;
}


MU_TEST(test_scan)
{
  DLiteInstance *inst;
  char *path = THISDIR "test-read-data.json";
  FILE *fp = fopen(path, "r");
  int stat;
  mu_check(fp);
  stat = dlite_storage_paths_append(path);
  mu_check(stat >= 0);
  inst = dlite_json_fscan(fp, "dbd9d597-16b4-58f5-b10f-7e49cf85084b", NULL);
  fclose(fp);
  mu_check(inst);

  printf("\n");
  dlite_json_fprint(stdout, inst, 0, 0);

  /* cleanup */
  dlite_instance_decref(inst);
}


MU_TEST(test_check)
{
  DLiteJsonFormat fmt;
  DLiteJsonFlag flags;

  fmt = dlite_json_checkfile(THISDIR "alloys.json", NULL, &flags);
  mu_assert_int_eq(dliteJsonDataFormat, fmt);
  mu_assert_int_eq(dliteJsonWithMeta, flags);

  fmt = dlite_json_checkfile(THISDIR "coll.json", NULL, &flags);
  mu_assert_int_eq(dliteJsonDataFormat, fmt);
  mu_assert_int_eq(dliteJsonWithMeta, flags);

  fmt = dlite_json_checkfile(THISDIR "test-data.json", NULL, &flags);
  mu_assert_int_eq(dliteJsonDataFormat, fmt);
  mu_assert_int_eq(dliteJsonWithMeta, flags);

  fmt = dlite_json_checkfile(THISDIR "test-entity.json", NULL, &flags);
  mu_assert_int_eq(dliteJsonMetaFormat, fmt);
  mu_assert_int_eq(dliteJsonSingle | dliteJsonWithMeta | dliteJsonArrays,
                   flags);

  fmt = dlite_json_checkfile(THISDIR "test-read-data.json", NULL, &flags);
  mu_assert_int_eq(dliteJsonMetaFormat, fmt);
  mu_assert_int_eq(dliteJsonWithMeta | dliteJsonArrays, flags);

  fmt = dlite_json_checkfile(THISDIR "test-read-data.json",
                             "0a75c038-1ea1-5916-bbd4-6d0298894f4c", &flags);
  mu_assert_int_eq(dliteJsonMetaFormat, fmt);
  mu_assert_int_eq(dliteJsonWithMeta | dliteJsonArrays, flags);

  fmt = dlite_json_checkfile(THISDIR "test-read-data.json",
                             "dlite/1/test-c", &flags);
  mu_assert_int_eq(dliteJsonMetaFormat, fmt);
  mu_assert_int_eq(dliteJsonUriKey | dliteJsonWithMeta | dliteJsonArrays,
                   flags);


  fmt = dlite_json_checkfile(THISDIR "test-read-data.json",
                             "dlite/1/empty", &flags);
  mu_assert_int_eq(dliteJsonMetaFormat, fmt);
  mu_assert_int_eq(dliteJsonUriKey | dliteJsonWithMeta | dliteJsonArrays,
                   flags);

  fmt = dlite_json_checkfile(THISDIR "test-read-data.json",
                             "dbd9d597-16b4-58f5-b10f-7e49cf85084b", &flags);
  mu_assert_int_eq(dliteJsonDataFormat, fmt);
  mu_assert_int_eq(dliteJsonWithMeta, flags);

  fmt = dlite_json_checkfile(THISDIR "test-read-data.json",
                             "2f4ae7b7-247a-5cc2-b6c5-5ac0ccd8cc5c", &flags);
  mu_assert_int_eq(dliteJsonDataFormat, fmt);
  mu_assert_int_eq(dliteJsonWithMeta, flags);

  fmt = dlite_json_checkfile(THISDIR "test-read-data.json",
                             "invalid", &flags);
  mu_assert_int_eq(-1, fmt);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_load);
  MU_RUN_TEST(test_sprint);
  MU_RUN_TEST(test_append);
  MU_RUN_TEST(test_decref);
  MU_RUN_TEST(test_scan);
  MU_RUN_TEST(test_check);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
