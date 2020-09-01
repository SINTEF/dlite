#include <stdlib.h>

#include "minunit/minunit.h"
#include "utils/err.h"
#include "utils/strtob.h"
#include "utils/fileutils.h"
#include "boolean.h"
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


MU_TEST(test_join_split_metadata)
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


MU_TEST(test_option_parse)
{
  char *options = strdup("name=a;n=3;f=3.14&b=yes#fragment");
  int i;
  FILE *old;
  DLiteOpt opts[] = {
    {'N', "name", "default-name", NULL},
    {'n', "n", "0", NULL},
    {'f', "f", "0.0", NULL},
    {'b', "b", "no", NULL},
    {'x', "x", "0", NULL},
    {0, NULL, NULL, NULL}
  };
  mu_assert_int_eq(0, dlite_option_parse(options, opts, 1));
  for (i=0; opts[i].key; i++) {
    switch (opts[i].c) {
    case 'N':
      mu_assert_string_eq("a", opts[i].value);
      break;
    case 'n':
      mu_assert_int_eq(3, atoi(opts[i].value));
      break;
    case 'f':
      mu_assert_double_eq(3.14, atof(opts[i].value));
      break;
    case 'b':
      mu_assert_int_eq(1, atob(opts[i].value));
      break;
    }
  }
  free(options);

  old = err_set_stream(NULL);
  mu_assert_int_eq(1, dlite_option_parse("name=C;mode=append", opts, 0));
  err_set_stream(old);
}


MU_TEST(test_join_url)
{
  char *url;
  mu_check((url = dlite_join_url("mongodb", "example.com/db",
                                 "mode=append", NULL)));
  mu_assert_string_eq("mongodb://example.com/db?mode=append", url);
  free(url);

  mu_check((url = dlite_join_url("json", "/home/john/file.json", NULL,
                                 "namespace/version/name")));
  mu_assert_string_eq("json:///home/john/file.json#namespace/version/name",
                      url);
  free(url);
}


MU_TEST(test_split_url)
{
  char *driver, *loc, *options, *fragment;
  char url1[] = "mongodb://example.com/db?mode=append";
  char url2[] = "json:///home/john/file.json#ns/ver/name";

  mu_check(0 == dlite_split_url(url1, &driver, &loc, &options, NULL));
  mu_assert_string_eq("mongodb", driver);
  mu_assert_string_eq("example.com/db", loc);
  mu_assert_string_eq("mode=append", options);

  mu_check(0 == dlite_split_url(url2, &driver, &loc, &options, &fragment));
  mu_assert_string_eq("json", driver);
  mu_assert_string_eq("/home/john/file.json", loc);
  mu_assert_string_eq("ns/ver/name", fragment);
  mu_assert_string_eq(NULL, options);
}


/***********************************************************************/


MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_get_uuid);
  MU_RUN_TEST(test_join_split_metadata);
  MU_RUN_TEST(test_option_parse);
  MU_RUN_TEST(test_join_url);
  MU_RUN_TEST(test_split_url);
}

int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
