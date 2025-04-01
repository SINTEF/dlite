#include <stdlib.h>

#include "minunit/minunit.h"
#include "utils/err.h"
#include "utils/strtob.h"
#include "utils/fileutils.h"
#include "utils/uuid.h"
#include "boolean.h"
#include "dlite.h"
#include "dlite-behavior.h"


MU_TEST(test_isuuid)
{
  mu_assert_int_eq(1, dlite_isuuid("a58d4302-c9be-416d-a36c-cb25524a5a17"));
  mu_assert_int_eq(1, dlite_isuuid("a58d4302-c9be-416d-a36c-cb25524a5a17+"));
  mu_assert_int_eq(0, dlite_isuuid("58d4302-c9be-416d-a36c-cb25524a5a17"));
  mu_assert_int_eq(0, dlite_isuuid("_a58d4302-c9be-416d-a36c-cb25524a5a17"));
}

MU_TEST(test_idtype)
{
  mu_assert_int_eq(dliteIdRandom, dlite_idtype(NULL));
  mu_assert_int_eq(dliteIdCopy,
                   dlite_idtype("6cb8e707-0fc5-5f55-88d4-d4fed43e64a8"));
  mu_assert_int_eq(dliteIdCopy,
                   dlite_idtype("http://onto-ns.com/meta/0.1/MyEntity/"
                                "6cb8e707-0fc5-5f55-88d4-d4fed43e64a8"));
  mu_assert_int_eq(dliteIdHash,
                   dlite_idtype("http://onto-ns.com/meta/0.1/Alloy/aa6060"));
  mu_assert_int_eq(dliteIdHash, dlite_idtype("aa6060"));
}


#define NS "http://onto-ns.com/meta/0.1/MyDatamodel"
#define UUID "6cb8e707-0fc5-5f55-88d4-d4fed43e64a8"
MU_TEST(test_normalise_id)
{
  char buf[256];

  mu_assert_int_eq(0, dlite_normalise_id(buf, sizeof(buf), NULL, NULL));
  mu_assert_string_eq("", buf);

  mu_assert_int_eq(24+36, dlite_normalise_id(buf, sizeof(buf),
                                             UUID, NULL));
  mu_assert_string_eq("http://onto-ns.com/data/" UUID, buf);

  mu_assert_int_eq(24+6, dlite_normalise_id(buf, sizeof(buf),
                                             "aa6060", NULL));
  mu_assert_string_eq("http://onto-ns.com/data/aa6060", buf);

  mu_assert_int_eq(40+36, dlite_normalise_id(buf, sizeof(buf),
                                             UUID, NS));
  mu_assert_string_eq(NS "/" UUID, buf);

  mu_assert_int_eq(40+6, dlite_normalise_id(buf, sizeof(buf),
                                             "aa6060", NS));
  mu_assert_string_eq(NS "/aa6060", buf);

  mu_assert_int_eq(24+6, dlite_normalise_id(buf, 8, "aa6060", NULL));
  mu_assert_string_eq("http://", buf);
}

MU_TEST(test_normalise_idn)
{
  char buf[256];

  mu_assert_int_eq(0, dlite_normalise_idn(buf, sizeof(buf), NULL, 0, NULL));
  mu_assert_string_eq("", buf);

  mu_assert_int_eq(24+36, dlite_normalise_idn(buf, sizeof(buf),
                                             UUID, sizeof(UUID), NULL));
  mu_assert_string_eq("http://onto-ns.com/data/" UUID, buf);

  mu_assert_int_eq(24+6, dlite_normalise_idn(buf, sizeof(buf),
                                             "aa6060", 6, NULL));
  mu_assert_string_eq("http://onto-ns.com/data/aa6060", buf);

  mu_assert_int_eq(40+36, dlite_normalise_idn(buf, sizeof(buf),
                                             UUID, sizeof(UUID), NS));
  mu_assert_string_eq(NS "/" UUID, buf);

  mu_assert_int_eq(40+6, dlite_normalise_idn(buf, sizeof(buf),
                                             "aa6060", 6, NS));
  mu_assert_string_eq(NS "/aa6060", buf);

  mu_assert_int_eq(24+2, dlite_normalise_idn(buf, sizeof(buf),
                                             "aa6060", 2, NULL));
  mu_assert_string_eq("http://onto-ns.com/data/aa", buf);

  mu_assert_int_eq(24+6, dlite_normalise_idn(buf, 8, "aa6060", 6, NULL));
  mu_assert_string_eq("http://", buf);
}


MU_TEST(test_get_uuid)
{
  char buff[37];

  mu_assert_int_eq(dliteIdRandom, dlite_get_uuid(buff, NULL));
  mu_assert_int_eq(1, dlite_isuuid(buff));

  mu_assert_int_eq(dliteIdCopy, dlite_get_uuid(buff,
                   "a839938d-1d30-5b2a-af5c-2a23d436abdc"));
  mu_assert_string_eq("a839938d-1d30-5b2a-af5c-2a23d436abdc", buff);

  mu_assert_int_eq(dliteIdCopy, dlite_get_uuid(buff,
                   "A839938D-1D30-5B2A-AF5C-2A23D436ABDC"));
  mu_assert_string_eq("a839938d-1d30-5b2a-af5c-2a23d436abdc", buff);

  mu_assert_int_eq(dliteIdCopy, dlite_get_uuid(buff,
                   "http://ex.com/a/a839938d-1d30-5b2a-af5c-2a23d436abdc"));
  mu_assert_string_eq("a839938d-1d30-5b2a-af5c-2a23d436abdc", buff);

  mu_assert_int_eq(dliteIdHash, dlite_get_uuid(buff,"http://ex.com/a/b"));
  mu_assert_string_eq("0e188d02-7327-5fa1-832f-78a53ed6e2a1", buff);

  mu_assert_int_eq(dliteIdHash, dlite_get_uuid(buff,DLITE_DATA_NS "/abc"));
  mu_assert_string_eq("8c942973-6c8d-5d6d-8e4e-503ee50d7f84", buff);

  if (dlite_behavior_get("namespacedID")) {
    mu_assert_int_eq(dliteIdHash, dlite_get_uuid(buff, "abc"));
    mu_assert_string_eq("8c942973-6c8d-5d6d-8e4e-503ee50d7f84", buff);
  } else {
    mu_assert_int_eq(dliteIdHash, dlite_get_uuid(buff, "abc"));
    mu_assert_string_eq("6cb8e707-0fc5-5f55-88d4-d4fed43e64a8", buff);

    mu_assert_int_eq(dliteIdHash, dlite_get_uuid(buff, "testdata"));
    mu_assert_string_eq("a839938d-1d30-5b2a-af5c-2a23d436abdc", buff);
  }

}

MU_TEST(test_get_uuidn)
{
  char *id, buff[37];

  mu_assert_int_eq(dliteIdRandom, dlite_get_uuidn(buff, NULL, 0));
  mu_assert_int_eq(dliteIdRandom, dlite_get_uuidn(buff, NULL, 1));
  mu_assert_int_eq(dliteIdRandom, dlite_get_uuidn(buff, "abc", 0));
  mu_assert_int_eq(dliteIdRandom, dlite_get_uuidn(buff, "", 20));

  if (dlite_behavior_get("namespacedID")) {
    mu_assert_int_eq(dliteIdHash, dlite_get_uuidn(buff, "abc", 3));
    mu_assert_string_eq("8c942973-6c8d-5d6d-8e4e-503ee50d7f84", buff);

    mu_assert_int_eq(dliteIdHash, dlite_get_uuidn(buff, "abc", 2));
    mu_assert_string_eq("e7eca1a9-c136-5e00-84ca-10bb61c8ca06", buff);

    mu_assert_int_eq(dliteIdHash, dlite_get_uuidn(buff, "abc", 4));
    mu_assert_string_eq("8f6b6536-03d6-5d86-91c9-87094b1acb9f", buff);

  } else {
    mu_assert_int_eq(dliteIdHash, dlite_get_uuidn(buff, "abc", 3));
    mu_assert_string_eq("6cb8e707-0fc5-5f55-88d4-d4fed43e64a8", buff);

    mu_assert_int_eq(dliteIdHash, dlite_get_uuidn(buff, "abc", 2));
    mu_assert_string_eq("710a586f-e1aa-54ec-93a9-85a85aa0b725", buff);

    mu_assert_int_eq(dliteIdHash, dlite_get_uuidn(buff, "abc", 4));
    mu_assert_string_eq("aa02945d-3cd6-5aec-82f9-0a8f51980d11", buff);
  }

  id = "a839938d-1d30-5b2a-af5c-2a23d436abdc";
  mu_assert_int_eq(dliteIdCopy, dlite_get_uuidn(buff, id, 36));
  mu_assert_string_eq("a839938d-1d30-5b2a-af5c-2a23d436abdc", buff);

  id = "a839938d-1d30-5b2a-af5c-2a23d436abdcXXX";
  mu_assert_int_eq(dliteIdCopy, dlite_get_uuidn(buff, id, 36));
  mu_assert_string_eq("a839938d-1d30-5b2a-af5c-2a23d436abdc", buff);

  id = "A839938D-1D30-5B2A-AF5C-2A23D436ABDC";
  mu_assert_int_eq(dliteIdCopy, dlite_get_uuidn(buff, id, 36));
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

  MU_RUN_TEST(test_isuuid);
  MU_RUN_TEST(test_idtype);
  MU_RUN_TEST(test_normalise_id);
  MU_RUN_TEST(test_normalise_idn);
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
