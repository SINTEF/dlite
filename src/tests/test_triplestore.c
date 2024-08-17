#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "utils/err.h"
#include "utils/session.h"
#include "minunit/minunit.h"
#include "dlite-misc.h"
#include "triplestore.h"


TripleStore *ts;


MU_TEST(test_create)
{
  mu_check((ts = triplestore_create()));
  errno = 0;
}


MU_TEST(test_add)
{
  Triple t[] = {
    {"book", "is-a", "thing", NULL, NULL},
    {"table", "is-a", "thing", NULL, NULL},
    {"table", "is-a", "furniture", NULL, NULL},
    {"book", "is-ontop-of", "table", NULL, NULL},
    {"write", "is-a", "action", NULL, NULL},
    {"walk", "is-a", "action", NULL, NULL},
    {"write", "is-a", "action", NULL, NULL}  /* dublicate */
  };
  size_t n = sizeof(t) / sizeof(t[0]);

  mu_assert_int_eq(0, triplestore_length(ts));
  mu_assert_int_eq(0, triplestore_add_triples(ts, t, n));
  mu_assert_int_eq(6, triplestore_length(ts));

  mu_assert_int_eq(0, triplestore_add_en(ts, "book", "has-title",
                                         "The Infinite Book"));
  mu_assert_int_eq(0, triplestore_add_uri(ts, "book", "has-weight",
                                          "book-weight"));
  mu_assert_int_eq(0, triplestore_add(ts, "book-weight", "has-value",
                                      "0.6", "xsd:double"));
  mu_assert_int_eq(0, triplestore_add(ts, "book-weight", "has-unit",
                                      "kg", "xsd:string"));
  mu_assert_int_eq(10, triplestore_length(ts));
}


MU_TEST(test_next)
{
  TripleState state;
  const Triple *t;

  triplestore_init_state(ts, &state);
  printf("\n");
  while ((t = triplestore_next(&state)))
    printf("  %-11s %-11s %-20s %s\n", t->s, t->p, t->o, t->d);
  triplestore_deinit_state(&state);
}


MU_TEST(test_poll)
{
  TripleState state;
  const Triple *t;

  triplestore_init_state(ts, &state);
  t = triplestore_poll(&state);
  mu_assert_string_eq("book", t->s);

  t = triplestore_next(&state);
  mu_assert_string_eq("book", t->s);

  t = triplestore_poll(&state);
  mu_assert_string_eq("table", t->s);

  triplestore_reset_state(&state);
  t = triplestore_poll(&state);
  mu_assert_string_eq("book", t->s);

  triplestore_deinit_state(&state);
}


MU_TEST(test_find)
{
  int n;
  TripleState state;
  const Triple *t;

  t = triplestore_find_first(ts, NULL, "is-a", "table", NULL);
  mu_check(t == NULL);

  t = triplestore_find_first(ts, NULL, "is-ontop-of", "table", NULL);
  mu_check(t);
  mu_assert_string_eq("book", t->s);
  mu_assert_string_eq("is-ontop-of", t->p);
  mu_assert_string_eq("table", t->o);
  mu_assert_string_eq(NULL, t->d);

  triplestore_init_state(ts, &state);
  n = 0;
  while (triplestore_find(&state, NULL, NULL, NULL, NULL)) n++;
  mu_assert_int_eq(10, n);
  triplestore_deinit_state(&state);

  triplestore_init_state(ts, &state);
  n = 0;
  while (triplestore_find(&state, NULL, "is-a", NULL, NULL)) n++;
  mu_assert_int_eq(5, n);
  triplestore_deinit_state(&state);

  triplestore_init_state(ts, &state);
  n = 0;
  while (triplestore_find(&state, NULL, "is-a", "thing", NULL)) n++;
  mu_assert_int_eq(2, n);
  triplestore_deinit_state(&state);

  /* Count IRIs */
  triplestore_init_state(ts, &state);
  n = 0;
  while (triplestore_find(&state, NULL, NULL, NULL, "")) n++;
  mu_assert_int_eq(7, n);
  triplestore_deinit_state(&state);

  triplestore_init_state(ts, &state);
  n = 0;
  while (triplestore_find(&state, NULL, NULL, NULL, "xsd:double")) n++;
  mu_assert_int_eq(1, n);
  triplestore_deinit_state(&state);

  t = triplestore_find_first(ts, NULL, NULL, NULL, "xsd:double");
  mu_check(t);
  mu_assert_string_eq("book-weight", t->s);
  mu_assert_string_eq("has-value", t->p);
  mu_assert_string_eq("0.6", t->o);
  mu_assert_string_eq("xsd:double", t->d);
}


MU_TEST(test_value)
{
  mu_assert_string_eq("action", triplestore_value(ts, "write", "is-a", NULL,
                                                  NULL, NULL, 0));
  mu_assert_string_eq("thing", triplestore_value(ts, "table", "is-a", NULL,
                                                  NULL, NULL, 1));
  mu_assert_string_eq("kg", triplestore_value(ts, "book-weight", "has-unit",
                                              NULL, NULL, NULL, 0));
  mu_assert_string_eq("book-weight", triplestore_value(ts, NULL, "has-unit",
                                              "kg", NULL, NULL, 0));
  mu_assert_string_eq("some-weight", triplestore_value(ts, NULL, "has-unit",
                                              "µg", NULL, "some-weight", 0));


  /* Check some failures */
 ErrTry:
  // more than one match
  mu_assert_string_eq(NULL, triplestore_value(ts, "table", "is-a", NULL,
                                              NULL, NULL, 0));
  // no match (datatype does not match)
  mu_assert_string_eq(NULL, triplestore_value(ts, "book-weight", "has-unit",
                                              NULL, "xsd:float", NULL, 0));
 ErrCatch(dliteLookupError):  // suppress errors
  break;
 ErrEnd;

 ErrTry:
  // at least 2 of s,p,o must be given
  mu_assert_string_eq(NULL, triplestore_value(ts, NULL, "is-a",
                                              NULL, NULL, NULL, 0));
  mu_assert_string_eq(NULL, triplestore_value(ts, "book", "is-a",
                                              "thing", NULL, NULL, 0));
 ErrCatch(dliteTypeError):  // suppress errors
  break;
 ErrEnd;
}


MU_TEST(test_remove)
{
  mu_assert_int_eq(10, triplestore_length(ts));

  mu_check(!triplestore_remove(ts, NULL, "is-something", NULL, NULL));
  mu_assert_int_eq(10, triplestore_length(ts));

  mu_assert_int_eq(4, triplestore_remove(ts, "book", NULL, NULL, NULL));
  mu_assert_int_eq(6, triplestore_length(ts));
}


MU_TEST(test_clear)
{
  mu_assert_int_eq(6, triplestore_length(ts));
  triplestore_clear(ts);
  mu_assert_int_eq(0, triplestore_length(ts));
}


MU_TEST(test_free)
{
  triplestore_free(ts);

  dlite_finalize();  // for testing memory leaks
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_create);
  MU_RUN_TEST(test_add);
  MU_RUN_TEST(test_next);
  MU_RUN_TEST(test_poll);
  MU_RUN_TEST(test_find);
  MU_RUN_TEST(test_value);
  MU_RUN_TEST(test_remove);
  MU_RUN_TEST(test_clear);
  MU_RUN_TEST(test_free);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
