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

  mu_assert_int_eq(0, dlite_get_uuid(buff,
                                     "A839938D-1D30-5B2A-AF5C-2A23D436ABDC"));
  mu_assert_string_eq("a839938d-1d30-5b2a-af5c-2a23d436abdc", buff);
}

MU_TEST(test_get_uuidn)
{
  char *id, buff[37];

  mu_assert_int_eq(4, dlite_get_uuidn(buff, NULL, 0));
  mu_assert_int_eq(4, dlite_get_uuidn(buff, NULL, 1));
  mu_assert_int_eq(4, dlite_get_uuidn(buff, "abc", 0));
  mu_assert_int_eq(4, dlite_get_uuidn(buff, "", 20));

  mu_assert_int_eq(5, dlite_get_uuidn(buff, "abc", 3));
  mu_assert_string_eq("6cb8e707-0fc5-5f55-88d4-d4fed43e64a8", buff);

  mu_assert_int_eq(5, dlite_get_uuidn(buff, "abc", 2));
  mu_assert_string_eq("710a586f-e1aa-54ec-93a9-85a85aa0b725", buff);

  mu_assert_int_eq(5, dlite_get_uuidn(buff, "abc", 4));
  mu_assert_string_eq("aa02945d-3cd6-5aec-82f9-0a8f51980d11", buff);


  id = "a839938d-1d30-5b2a-af5c-2a23d436abdc";
  mu_assert_int_eq(0, dlite_get_uuidn(buff, id, 36));
  mu_assert_string_eq("a839938d-1d30-5b2a-af5c-2a23d436abdc", buff);

  id = "a839938d-1d30-5b2a-af5c-2a23d436abdcXXX";
  mu_assert_int_eq(0, dlite_get_uuidn(buff, id, 36));
  mu_assert_string_eq("a839938d-1d30-5b2a-af5c-2a23d436abdc", buff);

  id = "A839938D-1D30-5B2A-AF5C-2A23D436ABDC";
  mu_assert_int_eq(0, dlite_get_uuidn(buff, id, 36));
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

  mu_assert_int_eq(0, dlite_option_parse(options, opts, dliteOptStrict));
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

  char options2[] = "name=C;mode=append";
  old = err_set_stream(NULL);
  mu_assert_int_eq(dliteValueError,
                   dlite_option_parse(options2, opts, dliteOptStrict));
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

int _deprecated_call(void) {
  err_clear();
  printf("\n");
  return dlite_deprecation_warning("100.0.1", "my old feature");
}

MU_TEST(test_deprecation_warning)
{
  /* Warning message should only be shown once */
  mu_assert_int_eq(0, _deprecated_call());
  mu_assert_int_eq(0, _deprecated_call());
  mu_assert_int_eq(0, _deprecated_call());

  err_clear();
  printf("\n");
  mu_assert_int_eq(dliteSystemError,
                   dlite_deprecation_warning("0.0.1", "my old feature 2"));

  err_clear();
  printf("\n");
  mu_assert_int_eq(dliteSystemError,
                   dlite_deprecation_warning("0.1.x", "my old feature 3"));
}


/***********************************************************************/


MU_TEST_SUITE(test_suite)
{
  dlite_init();

  MU_RUN_TEST(test_get_uuid);
  MU_RUN_TEST(test_get_uuidn);
  MU_RUN_TEST(test_join_split_metadata);
  MU_RUN_TEST(test_option_parse);
  MU_RUN_TEST(test_join_url);
  MU_RUN_TEST(test_split_url);
  MU_RUN_TEST(test_deprecation_warning);
}

int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
