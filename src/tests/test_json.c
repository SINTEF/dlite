#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "utils/integers.h"
#include "dlite.h"
#include "dlite-macros.h"

#include "minunit/minunit.h"


DLiteInstance *inst=NULL;
DLiteMeta *meta=NULL;


MU_TEST(test_load)
{
  char *url;
  url="json://"STRINGIFY(dlite_SOURCE_DIR)"/src/tests/test-entity.json?mode=r";
  meta = dlite_meta_load_url(url);
  mu_check(meta);

  url="json://" STRINGIFY(dlite_SOURCE_DIR) "/src/tests/test-data.json?mode=r"
    "#e076a856-e36e-5335-967e-2f2fd153c17d";
  inst = dlite_instance_load_url(url);
  mu_check(inst);
}


MU_TEST(test_sprint)
{
  char buf[4096];
  int m;

  m = dlite_json_sprint(buf, sizeof(buf), (DLiteInstance *)meta, 0, 0);
  printf("\n--------------------------------------------------------\n");
  printf("%s\n", buf);
  mu_assert_int_eq(1011, m);

  m = dlite_json_sprint(buf, sizeof(buf), (DLiteInstance *)meta, 2,
                        dliteJsonWithUuid | dliteJsonArrays);
  printf("\n--------------------------------------------------------\n");
  printf("%s\n", buf);
  mu_assert_int_eq(1165, m);

  printf("\n========================================================\n");
  m = dlite_json_sprint(buf, sizeof(buf), inst, 4, 0);
  printf("%s\n", buf);
  mu_assert_int_eq(404, m);
  printf("\n--------------------------------------------------------\n");

  m = dlite_json_sprint(buf, 80, inst, 4, 0);
  mu_assert_int_eq(404, m);
}


int append(const char *str)
{
  char *s = strdup(str);
  size_t size = strlen(s) + 1;
  int m, retval=0;
  printf("\n--- append: '%s' ---\n", str);
  m = dlite_json_append(&s, &size, inst, 0);
  dlite_errclr();
  if (m < 0) retval=m;
  printf("%s", s);
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
  dlite_instance_decref(inst);
  dlite_meta_decref(meta);
  inst = NULL;
  meta = NULL;
}


MU_TEST(test_sscan)
{
  DLiteInstance *inst;
  char *path = STRINGIFY(dlite_SOURCE_DIR) "/src/tests/test-read-data.json";
  FILE *fp = fopen(path, "r");
  int stat;
  mu_check(fp);
  stat = dlite_storage_paths_append(path);
  mu_check(stat > 0);
  inst = dlite_json_fscan(fp, "dbd9d597-16b4-58f5-b10f-7e49cf85084b", NULL);
  fclose(fp);
  mu_check(inst);

  printf("\n");
  dlite_json_fprint(stdout, inst, 0, 0);

  /* cleanup */
  dlite_instance_decref(inst);
}




/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_load);
  MU_RUN_TEST(test_sprint);
  MU_RUN_TEST(test_append);
  MU_RUN_TEST(test_decref);
  MU_RUN_TEST(test_sscan);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
