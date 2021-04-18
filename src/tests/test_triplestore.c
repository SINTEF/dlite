#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "minunit/minunit.h"
#include "triplestore.h"


TripleStore *ts;


MU_TEST(test_create)
{
  mu_check((ts = triplestore_create()));
  errno = 0;
}

MU_TEST(test_triple)
{
  Triple t;
  char *id;
  triple_set(&t, "book", "is-a", "thing", NULL);
  id = triple_get_id(NULL, t.s, t.p, t.o);
  mu_assert_string_eq("e86ddacd5fd2f3f8f46543fc8096eab96a12c440", id);
  triple_clean(&t);
  free(id);
}

MU_TEST(test_add)
{
  Triple t[] = {
    {"book", "is-a", "thing", NULL},
    {"table", "is-a", "thing", NULL},
    {"book", "is-ontop-of", "table", NULL},
    {"write", "is-a", "action", NULL},
    {"walk", "is-a", "action", NULL},
    {"write", "is-a", "action", NULL}  /* dublicate */
  };
  size_t n = sizeof(t) / sizeof(t[0]);

  mu_check(0 == triplestore_length(ts));
  mu_assert_int_eq(0, triplestore_add_triples(ts, t, n));
  mu_check(5 == triplestore_length(ts));

  mu_assert_int_eq(0, triplestore_add(ts, "read", "is-a", "action"));
  mu_check(6 == triplestore_length(ts));

}


MU_TEST(test_next)
{
  TripleState state;
  const Triple *t;

  triplestore_init_state(ts, &state);
  printf("\n");
  while ((t = triplestore_next(&state)))
    printf("  %-11s %-11s %-11s %s\n", t->s, t->p, t->o, t->id);
  triplestore_deinit_state(&state);
}



MU_TEST(test_find)
{
  int n;
  const Triple *t;
  TripleState state;

  t = triplestore_find_first(ts, NULL, "is-a", "table");
  mu_check(t == NULL);

  t = triplestore_find_first(ts, NULL, "is-ontop-of", "table");
  mu_check(t);
  mu_assert_string_eq("book", t->s);
  mu_assert_string_eq("is-ontop-of", t->p);
  mu_assert_string_eq("table", t->o);

  triplestore_init_state(ts, &state);
  n = 0;
  while (triplestore_find(&state, NULL, "is-a", "thing")) n++;
  mu_assert_int_eq(2, n);
  triplestore_deinit_state(&state);

  triplestore_init_state(ts, &state);
  n = 0;
  while (triplestore_find(&state, NULL, "is-a", NULL)) n++;
  mu_assert_int_eq(5, n);
  triplestore_deinit_state(&state);
}


MU_TEST(test_remove)
{
  mu_check(6 == triplestore_length(ts));

  mu_check(!triplestore_remove(ts, NULL, "is-something", NULL));
  mu_check(6 == triplestore_length(ts));

  mu_assert_int_eq(2, triplestore_remove(ts, "book", NULL, NULL));
  mu_check(4 == triplestore_length(ts));
}



MU_TEST(test_free)
{
  triplestore_free(ts);
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_create);
  MU_RUN_TEST(test_triple);
  MU_RUN_TEST(test_add);
  MU_RUN_TEST(test_next);
  MU_RUN_TEST(test_find);
  MU_RUN_TEST(test_remove);
  MU_RUN_TEST(test_free);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
