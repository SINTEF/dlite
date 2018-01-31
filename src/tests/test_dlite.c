#include <stdlib.h>

#include "minunit/minunit.h"
#include "dlite.h"


MU_TEST(test_get_uuid)
{
  char buff[37];
  mu_assert_int_eq(4, dlite_get_uuid(buff, NULL));

  mu_assert_int_eq(5, dlite_get_uuid(buff, "abc"));
  mu_assert_string_eq("6cb8e707-0fc5-5f55-88d4-d4fed43e64a8", buff);

  mu_assert_int_eq(5, dlite_get_uuid(buff, "testdata"));
  mu_assert_string_eq("a839938d-1d30-5b2a-af5c-2a23d436abdc", buff);

  mu_assert_int_eq(0, dlite_get_uuid(buff,
                                     "a839938d-1d30-5b2a-af5c-2a23d436abdc"));
  mu_assert_string_eq("a839938d-1d30-5b2a-af5c-2a23d436abdc", buff);
}


MU_TEST(join_split_metadata)
{
  char *uri = "http://www.sintef.no/meta/dlite/0.1/testdata";
  char *name, *version, *namespace, *meta;
  mu_check(dlite_split_meta_uri(uri, &name, &version, &namespace) == 0);
  mu_assert_string_eq("http://www.sintef.no/meta/dlite", namespace);
  mu_assert_string_eq("0.1", version);
  mu_assert_string_eq("testdata", name);

  mu_check((meta = dlite_join_meta_uri(name, version, namespace)));
  mu_assert_string_eq(uri, meta);

  free(name);
  free(version);
  free(namespace);
  free(meta);
}



/***********************************************************************/


MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_get_uuid);
  MU_RUN_TEST(join_split_metadata);
}

int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
