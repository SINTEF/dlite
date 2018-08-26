#include <stdlib.h>
#include <stdio.h>

#include "minunit/minunit.h"
#include "triplestore.h"


Triplestore *ts;


MU_TEST(test_create)
{
  mu_check((ts = ts_create()));
}

MU_TEST(test_triplet)
{
  Triplet t;
  char *id;
  triplet_set(&t, "book", "is-a", "thing");
  id = triplet_get_id(&t);
  mu_assert_string_eq("e86ddacd5fd2f3f8f46543fc8096eab96a12c440", id);
  triplet_clean(&t);
  free(id);
}

MU_TEST(test_add)
{
  Triplet t[] = {
    {"book", "is-a", "thing"},
    {"table", "is-a", "thing"},
    {"book", "is-ontop-of", "table"},
    {"write", "is-a", "action"},
    {"go", "is-a", "action"},
    {"write", "is-a", "action"}  /* dublicate */
  };
  size_t n = sizeof(t) / sizeof(t[0]);

  mu_assert_int_eq(0, ts_length(ts));
  mu_assert_int_eq(0, ts_add(ts, t, n));
  mu_assert_int_eq(5, ts_length(ts));
}


MU_TEST(test_find)
{
  int n;
  const Triplet *t;
  TripleState state;

  t = ts_find_first(ts, NULL, "is-a", "table");
  mu_check(t == NULL);

  t = ts_find_first(ts, NULL, "is-ontop-of", "table");
  mu_check(t);
  mu_assert_string_eq("book", t->s);
  mu_assert_string_eq("is-ontop-of", t->p);
  mu_assert_string_eq("table", t->o);

  ts_init_state(ts, &state);
  n = 0;
  while (ts_find(ts, &state, NULL, "is-a", "thing")) n++;
  mu_assert_int_eq(2, n);

  ts_init_state(ts, &state);
  n = 0;
  while (ts_find(ts, &state, NULL, "is-a", NULL)) n++;
  mu_assert_int_eq(4, n);
}


MU_TEST(test_free)
{
  ts_free(ts);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_create);
  MU_RUN_TEST(test_triplet);
  MU_RUN_TEST(test_add);
  MU_RUN_TEST(test_find);
  MU_RUN_TEST(test_free);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
