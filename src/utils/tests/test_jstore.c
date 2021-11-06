#include <stdlib.h>
#include <string.h>

#include "jstore.h"

#include "minunit/minunit.h"


JStore *js=NULL;


MU_TEST(test_open)
{
  js = jstore_open();
  mu_check(js);
}


MU_TEST(test_add)
{
  jstore_add(js, "pi", "3.14");
  jstore_add(js, "arr", "[1, 2, 3]");
  jstore_add(js, "str", "\"En af dem der red med fane\"");
  jstore_add(js, "bool", "true");
}

MU_TEST(test_addn)
{
  char *src = "{\"key\": \"a truncated string\", ...}";
  jstore_addn(js, src+2, 3, src+8, 20);
}

MU_TEST(test_addstolen)
{
  char *v = strdup("\"a stolen value\"");
  jstore_addstolen(js, "str", v);  // replaces previous value
}

MU_TEST(test_get)
{
  mu_assert_string_eq("3.14", jstore_get(js, "pi"));
  mu_assert_string_eq("[1, 2, 3]", jstore_get(js, "arr"));
  mu_assert_string_eq("\"a truncated string\"", jstore_get(js, "key"));
  mu_assert_string_eq("\"a stolen value\"", jstore_get(js, "str"));
  mu_assert_string_eq(NULL, jstore_get(js, "xxx"));
}

MU_TEST(test_remove)
{
  jstore_remove(js, "pi");
  jstore_remove(js, "xxx");
  mu_assert_string_eq(NULL, jstore_get(js, "pi"));
}

MU_TEST(test_update)
{
  JStore *js2 = jstore_open();
  jstore_update(js2, js);
  mu_assert_string_eq("[1, 2, 3]", jstore_get(js2, "arr"));
  jstore_close(js2);
}

MU_TEST(test_update_from_jsmn)
{
  char *src =
    "{"
    "  \"a\": 1.2,"
    "  \"bool\": false,"
    "  \"dict\": {\"k\": \"v\"},"
    "  \"arr\": [4, \"a\", 3.14]"
    "}";
  jsmn_parser parser;
  jsmntok_t *tokens=NULL;
  unsigned int ntokens=0;
  int stat;
  jsmn_init(&parser);
  stat = jsmn_parse_alloc(&parser, src, strlen(src), &tokens, &ntokens);
  mu_check(stat > 0);
  jstore_update_from_jsmn(js, src, tokens);
  free(tokens);
}

MU_TEST(test_to_string)
{
  char *buf = jstore_to_string(js);
  mu_check(buf);
  printf("\n\n");
  printf("-------------------------------------\n");
  printf("%s", buf);
  printf("-------------------------------------\n");
  free(buf);
}

MU_TEST(test_to_file)
{
  int stat = jstore_to_file(js, "jstore.json");
  mu_assert_int_eq(0, stat);
}

MU_TEST(test_update_file)
{
  int stat;
  stat = jstore_add(js, "key", "\"new value\"");
  mu_assert_int_eq(0, stat);
  stat = jstore_update_file(js, "jstore.json");
  mu_assert_int_eq(0, stat);
}

MU_TEST(test_iter)
{
  JStoreIter iter;
  const char *key;
  jstore_iter_init(js, &iter);
  printf("\n\nJSON store:\n");
  while ((key = jstore_iter_next(&iter)))
    printf("%6s : %s\n", key, jstore_get(js, key));
  jstore_iter_deinit(&iter);
}

MU_TEST(test_label)
{
  int stat;
  const char *label;
  stat = jstore_set_label(js, "key", "my label");
  mu_assert_int_eq(0, stat);

  stat = jstore_set_label(js, "key", "new label");
  mu_assert_int_eq(0, stat);

  stat = jstore_set_labeln(js, "key2", "another label", 7);
  mu_assert_int_eq(0, stat);

  label = jstore_get_label(js, "key");
  mu_assert_string_eq("new label", label);

  label = jstore_get_label(js, "key2");
  mu_assert_string_eq("another", label);

  label = jstore_get_label(js, "non-existing-key");
  mu_assert_string_eq(NULL, label);
}

MU_TEST(test_close)
{
  jstore_close(js);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_open);
  MU_RUN_TEST(test_add);
  MU_RUN_TEST(test_addn);
  MU_RUN_TEST(test_addstolen);
  MU_RUN_TEST(test_get);
  MU_RUN_TEST(test_remove);
  MU_RUN_TEST(test_update);
  MU_RUN_TEST(test_update_from_jsmn);
  MU_RUN_TEST(test_to_string);
  MU_RUN_TEST(test_to_file);
  MU_RUN_TEST(test_update_file);
  MU_RUN_TEST(test_iter);
  MU_RUN_TEST(test_label);
  MU_RUN_TEST(test_close);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
