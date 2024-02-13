#include <stdlib.h>
#include <string.h>

#include "err.h"
#include "session.h"

#include "minunit/minunit.h"



MU_TEST(test_session)
{
  Session *s1 = session_create("mysession");
  Session *s2 = session_create("mysession");
  err_clear();
  Session *s3 = session_get("mysession");
  Session *s4 = session_get("no-such-session");
  const char *id = session_get_id(s1);
  mu_check(s1);
  mu_check(s2 == NULL);
  mu_check(s3 == s1);
  mu_check(s4 == NULL);
  mu_assert_string_eq("mysession", id);
  session_free(s1);
  err_clear();
}


MU_TEST(test_default)
{
  int stat;
  Session *s1 = session_get_default();
  Session *s2 = session_get_default();
  mu_check(s2 == s1);
  err_clear();

  stat = session_set_default(s1);
  mu_assert_int_eq(0, stat);

  Session *s3 = session_create("new-session");
  stat = session_set_default(s3);
  mu_assert_int_eq(-3, stat);

  session_free(s1);
  session_free(s3);
  err_clear();
}


MU_TEST(test_state)
{
  int stat;
  Session *s = session_get_default();
  char *data = strdup("my state data...");
  stat = session_add_state(s, "data-id", data, free);
  mu_assert_int_eq(0, stat);

  stat = session_add_state(s, "another-id", "static state data", NULL);
  mu_assert_int_eq(0, stat);

  char *data2 = session_get_state(s, "data-id");
  mu_assert_string_eq(data, data2);

  session_free(s);
  err_clear();
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_session);
  MU_RUN_TEST(test_default);
  MU_RUN_TEST(test_state);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
