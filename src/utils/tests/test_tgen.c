#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "test_macros.h"
#include "tgen.h"

#include "minunit/minunit.h"


MU_TEST(test_tgen_buf_append)
{
  TGenBuf buf;
  memset(&buf, 0, sizeof(buf));
  mu_check(!tgen_buf_append(&buf, "abcdef", 3));
  mu_check(!tgen_buf_append(&buf, "ABCDEF", -1));
  mu_check(!tgen_buf_append(&buf, "123456", 0));
  mu_assert_string_eq("abcABCDEF", buf.buf);

  mu_check(!tgen_buf_append_fmt(&buf, "%03d%.2s", 42, "abcdef"));
  mu_assert_string_eq("abcABCDEF042ab", buf.buf);

  free(buf.buf);
}

MU_TEST(test_tgen_lineno)
{
  /*          0 12345 678910 */
  char *t = "\n\nabc \n de\n";
  mu_assert_int_eq(1, tgen_lineno(t, t + 0));
  mu_assert_int_eq(2, tgen_lineno(t, t + 1));
  mu_assert_int_eq(3, tgen_lineno(t, t + 2));
  mu_assert_int_eq(3, tgen_lineno(t, t + 5));
  mu_assert_int_eq(3, tgen_lineno(t, t + 6));
  mu_assert_int_eq(4, tgen_lineno(t, t + 7));
  mu_assert_int_eq(4, tgen_lineno(t, t + 9));
  mu_assert_int_eq(4, tgen_lineno(t, t + 10));
  mu_assert_int_eq(5, tgen_lineno(t, t + 11));
}

MU_TEST(test_tgen_subs)
{
  TGenSubs subs;
  const TGenSub *sub;
  tgen_subs_init(&subs);
  tgen_subs_set(&subs, "n",    "42",   NULL);
  tgen_subs_set(&subs, "pi",   "3.14", NULL);
  tgen_subs_set(&subs, "name", "Adam", NULL);

  sub = tgen_subs_get(&subs, "n");
  mu_check(sub);
  mu_assert_string_eq("n", sub->var);

  sub = tgen_subs_getn(&subs, "name", -1);
  mu_check(sub);
  mu_assert_string_eq("name", sub->var);

  sub = tgen_subs_getn(&subs, "name", 0);
  mu_check(!sub);

  sub = tgen_subs_getn(&subs, "name", 1);
  mu_check(sub);
  mu_assert_string_eq("n", sub->var);

  sub = tgen_subs_getn(&subs, "name", 2);
  mu_check(!sub);

  sub = tgen_subs_get(&subs, "x");
  mu_check(!sub);

  tgen_subs_deinit(&subs);
}


static int loop(TGenBuf *s, const char *template, int len,
                const TGenSubs *subs, void *context)
{
  int i, data[] = {1, 3, 5};
  TGenSubs subs2;

  UNUSED(subs);

  tgen_subs_init(&subs2);
  for (i=0; i<3; i++) {
    tgen_subs_set_fmt(&subs2, "i", NULL, "%d", i);
    tgen_subs_set_fmt(&subs2, "data", NULL, "%d", data[i]);
    tgen_append(s, template, len, &subs2, context);
  }
  tgen_subs_deinit(&subs2);
  return 0;
}


MU_TEST(test_tgen)
{
  char *str;
  TGenSubs subs;

  tgen_subs_init(&subs);
  tgen_subs_set(&subs, "n",    "42",   NULL);
  tgen_subs_set(&subs, "pi",   "3.14", NULL);
  tgen_subs_set(&subs, "name", "Adam", NULL);
  tgen_subs_set(&subs, "f",    NULL,   tgen_append);
  tgen_subs_set(&subs, "f2",   "XX",   tgen_append);
  tgen_subs_set(&subs, "loop", NULL,   loop);

  str = tgen("{name} got n={n}!", &subs, NULL);
  mu_assert_string_eq("Adam got n=42!", str);
  free(str);

  str = tgen("simple template", &subs, NULL);
  mu_assert_string_eq("simple template", str);
  free(str);

  str = tgen("{xname} got n={n}!", &subs, NULL);
  mu_check(!str);

  str = tgen("{name:new template} got n={n}!", &subs, NULL);
  mu_assert_string_eq("Adam got n=42!", str);
  free(str);

  str = tgen("{name:invalid {{n}} got n={n}!", &subs, NULL);
  mu_check(!str);

  str = tgen("invalid } template", &subs, NULL);
  mu_check(!str);

  str = tgen("{name:valid {n} } got n={n}!", &subs, NULL);
  mu_assert_string_eq("Adam got n=42!", str);
  free(str);

  str = tgen("{name:valid {n}{} got n={n}!", &subs, NULL);
  mu_assert_string_eq("Adam got n=42!", str);
  free(str);

  str = tgen("{name:valid {n} {n}{} got n={n}!", &subs, NULL);
  mu_assert_string_eq("Adam got n=42!", str);
  free(str);

  str = tgen("should {{ work }}!", &subs, NULL);
  mu_assert_string_eq("should { work }!", str);
  free(str);

  str = tgen("pi is {pi:templ string{}...", &subs, NULL);
  mu_assert_string_eq("pi is 3.14...", str);
  free(str);

  str = tgen("pi is {pi:templ {n{}...", &subs, NULL);
  mu_assert_string_eq("pi is 3.14...", str);
  free(str);

  str = tgen("func subst {f:YY}", &subs, NULL);
  mu_assert_string_eq("func subst YY", str);
  free(str);

  str = tgen("func subst {f:pi={pi}{}", &subs, NULL);
  mu_assert_string_eq("func subst pi=3.14", str);
  free(str);

  str = tgen("func subst: {f}", &subs, NULL);
  mu_check(!str);

  str = tgen("func subst {f2:pi={pi} }", &subs, NULL);
  mu_assert_string_eq("func subst pi=3.14 ", str);
  free(str);

  str = tgen("func subst {f2}", &subs, NULL);
  mu_assert_string_eq("func subst XX", str);
  free(str);

  str = tgen("show loop:\n{loop:  i={i} - data={data}\n}", &subs, NULL);
  mu_assert_string_eq("show loop:\n"
                      "  i=0 - data=1\n"
                      "  i=1 - data=3\n"
                      "  i=2 - data=5\n", str);
  free(str);

  {
    char template[] =
      "We have:\n"
      "  pi={pi}\n"
      "  n={n}\n"
      "and the loop:\n"
      "{loop:  i={i} - data={data}\n}";
    str = tgen(template, &subs, NULL);
    mu_assert_string_eq("We have:\n"
                        "  pi=3.14\n"
                        "  n=42\n"
                        "and the loop:\n"
                        "  i=0 - data=1\n"
                        "  i=1 - data=3\n"
                        "  i=2 - data=5\n", str);
    free(str);
  }

  tgen_subs_deinit(&subs);
}






/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_tgen_buf_append);
  MU_RUN_TEST(test_tgen_lineno);
  MU_RUN_TEST(test_tgen_subs);
  MU_RUN_TEST(test_tgen);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
