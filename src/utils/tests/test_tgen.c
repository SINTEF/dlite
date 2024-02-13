#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "test_macros.h"
#include "err.h"
#include "tgen.h"

#include "minunit/minunit.h"


MU_TEST(test_tgen_escaped_copy)
{
  char dest[32], *src="a\\nbb\\tcc\\\n-d";

  mu_assert_int_eq(2, tgen_escaped_copy(dest, src, 2));
  dest[2] = '\0';
  mu_assert_string_eq("a\\", dest);

  mu_assert_int_eq(2, tgen_escaped_copy(dest, src, 3));
  dest[2] = '\0';
  mu_assert_string_eq("a\n", dest);

  mu_assert_int_eq(3, tgen_escaped_copy(dest, src, 4));
  dest[3] = '\0';
  mu_assert_string_eq("a\nb", dest);

  mu_assert_int_eq(9, tgen_escaped_copy(dest, src, -1));
  dest[9] = '\0';
  mu_assert_string_eq("a\nbb\tcc-d", dest);

}

MU_TEST(test_tgen_setcase)
{
  char s[] = "A String - To Test!";

  mu_check(!tgen_setcase(s, -1, 's'));
  mu_assert_string_eq("A String - To Test!", s);

  mu_check(!tgen_setcase(s, -1, 'c'));
  mu_assert_string_eq("a string - to test!", s);

  mu_check(!tgen_setcase(s, -1, 'C'));
  mu_assert_string_eq("A STRING - TO TEST!", s);

  mu_check(!tgen_setcase(s, -1, 'T'));
  mu_assert_string_eq("A string - to test!", s);

  mu_check(!tgen_setcase(s, 4, 'C'));
  mu_assert_string_eq("A STring - to test!", s);

  mu_check(tgen_setcase(s, -1, 'S'));
  mu_check(tgen_setcase(s, -1, '\0'));
}

MU_TEST(test_tgen_convert_case)
{
  char *p,  *s = "AVery mixed_Sentence: 1+2pi";

  mu_check((p = tgen_convert_case(s, -1, 's')));
  mu_assert_string_eq("AVery mixed_Sentence: 1+2pi", p);
  free(p);

  mu_check((p = tgen_convert_case(s, -1, 'c')));
  mu_assert_string_eq("avery mixed_sentence: 1+2pi", p);
  free(p);

  mu_check((p = tgen_convert_case(s, -1, 'C')));
  mu_assert_string_eq("AVERY MIXED_SENTENCE: 1+2PI", p);
  free(p);

  mu_check((p = tgen_convert_case(s, -1, 'u')));
  mu_assert_string_eq("a_very_mixed_sentence:_1+2pi", p);
  free(p);

  mu_check((p = tgen_convert_case(s, -1, 'U')));
  mu_assert_string_eq("A_VERY_MIXED_SENTENCE:_1+2PI", p);
  free(p);

  mu_check((p = tgen_convert_case(s, -1, 'm')));
  mu_assert_string_eq("aVeryMixedSentence:1+2Pi", p);
  free(p);

  mu_check((p = tgen_convert_case(s, -1, 'M')));
  mu_assert_string_eq("AVeryMixedSentence:1+2Pi", p);
  free(p);

  mu_check((p = tgen_convert_case(s, -1, 'T')));
  mu_assert_string_eq("Avery mixed_sentence: 1+2pi", p);
  free(p);

  mu_check((p = tgen_convert_case(s, -1, 'i')));
  mu_assert_string_eq("AVery_mixed_Sentence__1_2pi", p);
  free(p);

  mu_check(!tgen_convert_case(s, -1, 'I'));

  mu_check((p = tgen_convert_case("  n-Atoms  ", -1, 'i')));
  mu_assert_string_eq("n_Atoms", p);
  free(p);

  mu_check((p = tgen_convert_case("  n+Atoms  ", -1, 'i')));
  mu_assert_string_eq("n_Atoms", p);
  free(p);

  mu_check((p = tgen_convert_case("  n-Atoms  ", -1, 'I')));
  mu_assert_string_eq("n_Atoms", p);
  free(p);

  mu_check(!tgen_convert_case("  n+Atoms  ", -1, 'I'));

  mu_check((p = tgen_convert_case(" ab  cd e ", 4, 'u')));
  mu_assert_string_eq("ab", p);
  free(p);

  mu_check((p = tgen_convert_case(" ab  cd e ", -1, 'u')));
  mu_assert_string_eq("ab_cd_e", p);
  free(p);

  mu_check((p = tgen_convert_case(" ab  cd e ", 4, 'M')));
  mu_assert_string_eq("Ab", p);
  free(p);

  mu_check((p = tgen_convert_case(" ab  cd e ", -1, 'M')));
  mu_assert_string_eq("AbCdE", p);
  free(p);
}

MU_TEST(test_tgen_buf_append)
{
  TGenBuf buf;
  tgen_buf_init(&buf);
  mu_assert_int_eq(3, tgen_buf_append(&buf, "abcdef", 3));
  mu_assert_int_eq(6, tgen_buf_append(&buf, "ABCDEF", -1));
  mu_assert_int_eq(0, tgen_buf_append(&buf, "123456", 0));
  mu_assert_string_eq("abcABCDEF", buf.buf);

  mu_assert_int_eq(5, tgen_buf_append_fmt(&buf, "%03d%.2s", 42, "abcdef"));
  mu_assert_string_eq("abcABCDEF042ab", buf.buf);

  tgen_buf_deinit(&buf);
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
  tgen_subs_setn(&subs, "key+trash", 3, "<newkey>", NULL);
  tgen_subs_set_fmt(&subs, "temp", NULL, "%.1fC", 37.1234);

  sub = tgen_subs_get(&subs, "n");
  mu_check(sub);
  mu_assert_string_eq("n", sub->var);
  mu_assert_string_eq("42", sub->repl);

  sub = tgen_subs_getn(&subs, "name", -1);
  mu_check(sub);
  mu_assert_string_eq("name", sub->var);
  mu_assert_string_eq("Adam", sub->repl);

  sub = tgen_subs_getn(&subs, "name", 0);
  mu_check(!sub);

  sub = tgen_subs_getn(&subs, "name", 1);
  mu_check(sub);
  mu_assert_string_eq("n", sub->var);
  mu_assert_string_eq("42", sub->repl);

  sub = tgen_subs_getn(&subs, "name", 2);
  mu_check(!sub);

  sub = tgen_subs_get(&subs, "x");
  mu_check(!sub);

  sub = tgen_subs_get(&subs, "key");
  mu_check(sub);
  mu_assert_string_eq("key", sub->var);
  mu_assert_string_eq("<newkey>", sub->repl);

  sub = tgen_subs_get(&subs, "temp");
  mu_check(sub);
  mu_assert_string_eq("temp", sub->var);
  mu_assert_string_eq("37.1C", sub->repl);

  tgen_subs_deinit(&subs);
}


static int loop(TGenBuf *s, const char *template, int len,
                TGenSubs *subs, void *context)
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
  tgen_subs_set(&subs, "n",     "42",   NULL);
  tgen_subs_set(&subs, "pi",    "3.14", NULL);
  tgen_subs_set(&subs, "name",  "Adam", NULL);
  tgen_subs_set(&subs, "zero",  "0",    NULL);
  tgen_subs_set(&subs, "empty", "",     NULL);
  tgen_subs_set(&subs, "s",     "length is 5.5mm", NULL);
  tgen_subs_set(&subs, "f",     NULL,   tgen_append);
  tgen_subs_set(&subs, "f2",    "XX",   tgen_append);
  tgen_subs_set(&subs, "loop",  NULL,   loop);

  str = tgen("{name} got n={n}!", &subs, NULL);
  mu_assert_string_eq("Adam got n=42!", str);
  free(str);

  str = tgen("simple template", &subs, NULL);
  mu_assert_string_eq("simple template", str);
  free(str);

  str = tgen("{xname} got n={n}!", &subs, NULL);
  mu_check(!str);
  err_clear();

  str = tgen("{name:new template} got n={n}!", &subs, NULL);
  mu_assert_string_eq("Adam got n=42!", str);
  free(str);

  str = tgen("{name:invalid {{n}} got n={n}!", &subs, NULL);
  mu_check(!str);
  err_clear();

  str = tgen("invalid } template", &subs, NULL);
  mu_check(!str);
  err_clear();

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

  str = tgen("pi is {pi%.3s}", &subs, NULL);
  mu_assert_string_eq("pi is 3.1", str);
  free(str);

  str = tgen("pi is {pi%.3u}", &subs, NULL);
  mu_assert_string_eq("pi is 3.1", str);
  free(str);

  str = tgen("pi is {pi%-6.3T}...", &subs, NULL);
  mu_assert_string_eq("pi is 3.1   ...", str);
  free(str);

  str = tgen("The name is {name%c}...", &subs, NULL);
  mu_assert_string_eq("The name is adam...", str);
  free(str);

  str = tgen("The name is {name%C}...", &subs, NULL);
  mu_assert_string_eq("The name is ADAM...", str);
  free(str);

  str = tgen("Answer: {s%u}", &subs, NULL);
  mu_assert_string_eq("Answer: length_is_5.5mm", str);
  free(str);

  str = tgen("Answer: {s%U}", &subs, NULL);
  mu_assert_string_eq("Answer: LENGTH_IS_5.5MM", str);
  free(str);

  str = tgen("Answer: {s%m}", &subs, NULL);
  mu_assert_string_eq("Answer: lengthIs5.5Mm", str);
  free(str);

  str = tgen("Answer: {s%M}", &subs, NULL);
  mu_assert_string_eq("Answer: LengthIs5.5Mm", str);
  free(str);

  str = tgen("func subst {f:YY}", &subs, NULL);
  mu_assert_string_eq("func subst YY", str);
  free(str);

  str = tgen("func subst {f:pi={pi}{}", &subs, NULL);
  mu_assert_string_eq("func subst pi=3.14", str);
  free(str);

  str = tgen("func subst: {f}", &subs, NULL);
  mu_check(!str);
  err_clear();

  str = tgen("func subst {f2:pi={pi} }", &subs, NULL);
  mu_assert_string_eq("func subst pi=3.14 ", str);
  free(str);

  str = tgen("func subst {f2}", &subs, NULL);
  mu_assert_string_eq("func subst XX", str);
  free(str);

  /* test loop function */
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

  /* test variable existences */
  str = tgen("whether 'name' is defined: {name?}", &subs, NULL);
  mu_assert_string_eq("whether 'name' is defined: 1", str);
  free(str);

  str = tgen("whether 'empty' is defined: {empty?}", &subs, NULL);
  mu_assert_string_eq("whether 'empty' is defined: 1", str);
  free(str);

  str = tgen("whether 'xxx' is defined: {xxx?}", &subs, NULL);
  mu_assert_string_eq("whether 'xxx' is defined: 0", str);
  free(str);

  str = tgen("whether var is not empty: {?}", &subs, NULL);
  mu_check(!str);
  err_clear();

  /* test variable assignment */
  str = tgen("exists={x?}, {x=5} exists={x?}, x={x}", &subs, NULL);
  mu_assert_string_eq("exists=0,  exists=1, x=5", str);
  free(str);

  str = tgen("{name=Ewa}name={name}", &subs, NULL);
  mu_assert_string_eq("name=Ewa", str);
  free(str);

  /* test condition */
  str = tgen("{@if:0}a{@elif:}b{@else}pi = {pi}{@endif}...", &subs, NULL);
  mu_assert_string_eq("pi = 3.14...", str);
  free(str);

  str = tgen("{@if:1}a{@elif:}b{@else}pi = {pi}{@endif}...", &subs, NULL);
  mu_assert_string_eq("a...", str);
  free(str);

  err_clear();
  str = tgen("{@if: {empty} }a{@elif:1}b{@else}c{@endif}...", &subs, NULL);
  mu_check(!str);
  err_clear();

  str = tgen("{@if: \"{empty}\" }a{@elif:1}b{@else}c{@endif}...", &subs, NULL);
  mu_assert_string_eq("b...", str);
  free(str);

  str = tgen("{@if: '{empty}' }a{@elif:1}b{@else}c{@endif}...", &subs, NULL);
  mu_assert_string_eq("b...", str);
  free(str);

  str = tgen("{@if: \"\" }true{@else}false{@endif}", &subs, NULL);
  mu_assert_string_eq("false", str);
  free(str);

  str = tgen("{@if: \" \" }true{@else}false{@endif}", &subs, NULL);
  mu_assert_string_eq("true", str);
  free(str);

  str = tgen("{@if: '{pi}' }true{@else}false{@endif}", &subs, NULL);
  mu_assert_string_eq("true", str);
  free(str);

  str = tgen("{@if:'{empty}'}true{@else}false{@endif}", &subs, NULL);
  mu_assert_string_eq("false", str);
  free(str);

  str = tgen("{@if: '{empty}' }true{@else}false{@endif}", &subs, NULL);
  mu_assert_string_eq("false", str);
  free(str);

  str = tgen("{@if:'{name}'}true{@else}false{@endif}", &subs, NULL);
  mu_assert_string_eq("true", str);
  free(str);

  /* test padding */
  str = tgen("pi{@4}is {@12}{pi:templ string}...", &subs, NULL);
  mu_assert_string_eq("pi  is      3.14...", str);
  free(str);

  str = tgen("pi{@4}is\n {@6}{pi:templ string}...", &subs, NULL);
  mu_assert_string_eq("pi  is\n      3.14...", str);
  free(str);

  /* test @error construct */
  str = tgen("bla, bla {@error:My error message...} blu bla", &subs, NULL);
  mu_check(!str);
  mu_assert_string_eq("Error 2027: line 1: My error message...", err_getmsg());

  tgen_subs_deinit(&subs);
}






/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_tgen_escaped_copy);
  MU_RUN_TEST(test_tgen_setcase);
  MU_RUN_TEST(test_tgen_convert_case);
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
