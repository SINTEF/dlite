#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "minunit/minunit.h"
#include "getuuid.h"


MU_TEST(test_isuuid)
{
  mu_assert_int_eq(1, isuuid("a58d4302-c9be-416d-a36c-cb25524a5a17"));
  mu_assert_int_eq(1, isuuid("a58d4302-c9be-416d-a36c-cb25524a5a17+"));
  mu_assert_int_eq(0, isuuid("58d4302-c9be-416d-a36c-cb25524a5a17"));
  mu_assert_int_eq(0, isuuid("_a58d4302-c9be-416d-a36c-cb25524a5a17"));
}

MU_TEST(test_isinstanceiri)
{
  mu_assert_int_eq(
    1, isinstanceuri("http://onto-ns.com/meta/0.1/Entity/"
                     "a58d4302-c9be-416d-a36c-cb25524a5a17", 0));
  mu_assert_int_eq(
    1, isinstanceuri("http://onto-ns.com/meta/0.1/Entity/"
                     "a58d4302-c9be-416d-a36c-cb25524a5a17#", 0));
  mu_assert_int_eq(
    1, isinstanceuri("http://onto-ns.com/meta/0.1/Entity/"
                     "a58d4302-c9be-416d-a36c-cb25524a5a17/", 0));
  mu_assert_int_eq(  // no slash between URI and UUID
    0, isinstanceuri("http://onto-ns.com/meta/0.1/Entity#"
                     "a58d4302-c9be-416d-a36c-cb25524a5a17", 0));
  mu_assert_int_eq(  // no colon in URI
    0, isinstanceuri("http//onto-ns.com/meta/0.1/Entity/"
                     "a58d4302-c9be-416d-a36c-cb25524a5a17", 0));
  mu_assert_int_eq(  // characters after the hash
    0, isinstanceuri("http://onto-ns.com/meta/0.1/Entity/"
                     "a58d4302-c9be-416d-a36c-cb25524a5a17#x", 0));
}

MU_TEST(test_getuuid)
{
  char buf[UUID_LEN+1];

  mu_assert_int_eq(UUID_RANDOM, getuuid(buf, NULL));
  mu_assert_int_eq(1, isuuid(buf));
  mu_assert_int_eq(UUID_RANDOM, getuuid(buf, ""));
  mu_assert_int_eq(1, isuuid(buf));

  mu_assert_int_eq(UUID_COPY,
                   getuuid(buf, "d683cdda-4987-48a5-9e32-cb37adfe3db0"));
  mu_assert_string_eq("d683cdda-4987-48a5-9e32-cb37adfe3db0", buf);

  char *uri;
  uri = "http://onto-ns.com/meta/0.1/Energy/"
    "d683cdda-4987-48a5-9e32-cb37adfe3db0";
  mu_assert_int_eq(UUID_EXTRACT, getuuid(buf, uri));
  mu_assert_string_eq("d683cdda-4987-48a5-9e32-cb37adfe3db0", buf);
  uri = "http://onto-ns.com/meta/0.1/Energy/"
    "d683cdda-4987-48a5-9e32-cb37adfe3db0#";
  mu_assert_int_eq(UUID_EXTRACT, getuuid(buf, uri));
  mu_assert_string_eq("d683cdda-4987-48a5-9e32-cb37adfe3db0", buf);

  mu_assert_int_eq(UUID_HASH,
                   getuuid(buf, "?683cdda-4987-48a5-9e32-cb37adfe3db0"));
  mu_assert_int_eq(1, isuuid(buf));
  mu_assert_int_eq(UUID_HASH,
                   getuuid(buf, "abc"));
  mu_assert_int_eq(1, isuuid(buf));
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_isuuid);
  MU_RUN_TEST(test_isinstanceiri);
  MU_RUN_TEST(test_getuuid);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
