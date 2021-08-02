#include <stdlib.h>
#include <string.h>

#include "jsmnx.h"
#include "minunit/minunit.h"



MU_TEST(test_jsmn)
{
  char *js = "{"
    "\"name\": \"field\", "
    "\"type\": \"blob3\", "
    "\"dims\": [\"N+1\", \"M\"], "
    "\"unit\": \"m\""
    "}";

  jsmn_parser p;
  jsmntok_t tokens[128];
  const jsmntok_t *t; /* We expect no more than 128 JSON tokens */
  int r;

  jsmn_init(&p);
  r = jsmn_parse(&p, js, strlen(js), tokens, 128);
  mu_assert_int_eq(11, r);
  mu_assert_int_eq(JSMN_OBJECT, tokens->type);
  mu_assert_int_eq(4, tokens->size);

  t = jsmn_item(js, tokens, "dims");
  mu_assert_int_eq(JSMN_ARRAY, t->type);
  mu_assert_int_eq(2, t->size);

  t = jsmn_element(js, t, 1);
  mu_assert_int_eq(JSMN_STRING, t->type);
  mu_assert_strn_eq("M", js+t->start, 1);

}





/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_jsmn);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
