#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "utils/session.h"
#include "minunit/minunit.h"
#include "dlite-misc.h"
#include "triple.h"



MU_TEST(test_set)
{
  Triple t;
  triple_set(&t, "book", "is-a", "thing", NULL, NULL);
  mu_assert_string_eq("book", t.s);
  mu_assert_string_eq("is-a", t.p);
  mu_assert_string_eq("thing", t.o);
  mu_assert_string_eq(NULL, t.d);
  mu_assert_string_eq("e86ddacd5fd2f3f8f46543fc8096eab96a12c440", t.id);

  triple_reset(&t, "subject", "predicate", "object", "datatype", NULL);
  mu_assert_string_eq("subject", t.s);
  mu_assert_string_eq("predicate", t.p);
  mu_assert_string_eq("object", t.o);
  mu_assert_string_eq("datatype", t.d);
  triple_clean(&t);
}


MU_TEST(test_get_id)
{
  char *id;
  Triple t = {"s", "p", "o", "@en", NULL};
  id = triple_get_id(NULL, t.s, t.p, t.o, t.d);
  mu_assert_string_eq("fac793e0cf9731b05a1554d16f834f03bbfe8306", id);
  free(id);
}


MU_TEST(test_copy)
{
  Triple t = {"s", "p", "o", "@en", NULL}, t2, *tp;
  tp = triple_copy(&t2, &t);
  mu_assert_ptr_eq(tp, &t2);
  mu_assert_string_eq("s", t2.s);
  mu_assert_string_eq("p", t2.p);
  mu_assert_string_eq("o", t2.o);
  mu_assert_string_eq("@en", t2.d);
  mu_assert_string_eq(NULL, t2.id);
  triple_clean(&t2);
}


MU_TEST(test_finalize)
{
  dlite_finalize();  // for memory checking
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_set);
  MU_RUN_TEST(test_get_id);
  MU_RUN_TEST(test_copy);
  MU_RUN_TEST(test_finalize);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
